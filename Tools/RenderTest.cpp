// PsyForge – Offline-Test ohne DAW/JUCE
//   psyforge_test render <outdir>     rendert alle Werks-Presets (Kick, Bass, Loop) als WAV
//   psyforge_test roundtrip           rendert Presets, analysiert sie wieder und meldet die Übereinstimmung
//   psyforge_test analyze <file.wav>  analysiert eine Datei und gibt die gefundenen Werte aus
#include "../Source/Presets.h"
#include "../Source/Analysis.h"
#include <cstdio>
#include <fstream>
#include <string>
#include <chrono>

using namespace pf;

static void writeWav (const std::string& path, const std::vector<float>& x, int sr)
{
    std::ofstream f (path, std::ios::binary);
    auto w32 = [&] (uint32_t v) { f.write ((const char*) &v, 4); };
    auto w16 = [&] (uint16_t v) { f.write ((const char*) &v, 2); };
    const uint32_t dataBytes = (uint32_t) x.size() * 2;
    f.write ("RIFF", 4); w32 (36 + dataBytes); f.write ("WAVE", 4);
    f.write ("fmt ", 4); w32 (16); w16 (1); w16 (1); w32 ((uint32_t) sr); w32 ((uint32_t) sr * 2); w16 (2); w16 (16);
    f.write ("data", 4); w32 (dataBytes);
    for (float s : x) { const int v = (int) std::lround (std::clamp (s, -1.0f, 1.0f) * 32767.0f); w16 ((uint16_t) (int16_t) v); }
}

static bool readWav (const std::string& path, std::vector<float>& out, int& sr)
{
    std::ifstream f (path, std::ios::binary);
    if (! f) return false;
    std::vector<char> d ((std::istreambuf_iterator<char> (f)), {});
    if (d.size() < 44) return false;
    auto u16 = [&] (size_t o) { return (uint16_t) ((uint8_t) d[o] | ((uint8_t) d[o + 1] << 8)); };
    auto u32 = [&] (size_t o) { return (uint32_t) u16 (o) | ((uint32_t) u16 (o + 2) << 16); };
    size_t pos = 12; int ch = 1, bits = 16, fmt = 1;
    while (pos + 8 <= d.size())
    {
        const std::string id (&d[pos], 4); const uint32_t len = u32 (pos + 4);
        if (id == "fmt ") { fmt = u16 (pos + 8); ch = u16 (pos + 10); sr = (int) u32 (pos + 12); bits = u16 (pos + 22); }
        else if (id == "data")
        {
            const size_t bps = (size_t) bits / 8, n = len / (bps * (size_t) ch);
            out.resize (n);
            for (size_t i = 0; i < n; ++i)
            {
                double acc = 0;
                for (int c = 0; c < ch; ++c)
                {
                    const size_t o = pos + 8 + (i * (size_t) ch + (size_t) c) * bps;
                    if (o + bps > d.size()) break;
                    double v = 0;
                    if (fmt == 3 && bits == 32) { float fv; std::memcpy (&fv, &d[o], 4); v = fv; }
                    else if (bits == 16) v = (int16_t) u16 (o) / 32768.0;
                    else if (bits == 24) { int32_t s = (int32_t) ((uint8_t) d[o] | ((uint8_t) d[o + 1] << 8) | ((int8_t) d[o + 2] << 16)); v = s / 8388608.0; }
                    else if (bits == 32) v = (int32_t) u32 (o) / 2147483648.0;
                    acc += v;
                }
                out[i] = (float) (acc / ch);
            }
            return true;
        }
        pos += 8 + len + (len & 1);
    }
    return false;
}

static void printDiff (const SynthParams& a, const SynthParams& b, const char* prefix)
{
    for (auto& d : paramTable())
    {
        if (std::strncmp (d.id, prefix, 2) != 0 || d.kind != KFloat) continue;
        const float va = a.*(d.field), vb = b.*(d.field);
        std::printf ("    %-10s soll %9.3f  ist %9.3f\n", d.id, va, vb);
    }
}

int main (int argc, char** argv)
{
    const std::string mode = argc > 1 ? argv[1] : "render";
    const double sr = 44100.0;

    if (mode == "render")
    {
        const std::string dir = argc > 2 ? argv[2] : ".";
        SynthParams cur = defaultParams();
        for (auto& fp : factoryPresets())
        {
            SynthParams p = applyPreset (fp, cur);
            std::string base = dir + "/" + fp.name;
            for (auto& c : base) if (c == ' ') c = '_';
            auto k = renderKick (p, sr, p.bpm);
            auto b = renderBass (p, sr, p.bpm, (int) p.note);
            auto l = renderLoop (p, sr, p.bpm, 8, (int) p.note, (int) p.pattern);
            writeWav (base + "_kick.wav", k.audio, (int) sr);
            writeWav (base + "_bass.wav", b.audio, (int) sr);
            writeWav (base + "_loop.wav", l.audio, (int) sr);
            float pk = 0, pb = 0, pl = 0;
            for (float s : k.audio) pk = std::max (pk, std::abs (s));
            for (float s : b.audio) pb = std::max (pb, std::abs (s));
            for (float s : l.audio) pl = std::max (pl, std::abs (s));
            std::printf ("%-20s kick peak %.2f (%.0f ms)  bass peak %.2f  loop peak %.2f  end %.1f Hz\n",
                         fp.name, pk, k.audio.size() * 1000.0 / sr, pb, pl, kickEndHz (p));
        }
        return 0;
    }

    if (mode == "roundtrip")
    {
        SynthParams cur = defaultParams();
        const int only = argc > 2 ? std::atoi (argv[2]) : -1;
        int idx = 0;
        for (auto& fp : factoryPresets())
        {
            const int my = idx++;
            if (only >= 0 && my != only) continue;
            const SynthParams truth = applyPreset (fp, cur);
            for (int kind = 0; kind < 2; ++kind)
            {
                RenderOut ro = kind == 0 ? renderKick (truth, sr, truth.bpm) : renderBass (truth, sr, truth.bpm, (int) truth.note);
                Reference ref = makeReference (ro.audio, sr);
                const auto t0 = std::chrono::steady_clock::now();
                AnalysisJob job;
                job.forceType = kind == 0 ? SoundKick : SoundBass;
                AnalysisResult res = analyzeAndFit (ref, defaultParams(), job);
                const double secs = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
                std::printf ("%-20s %s  erkannt %-4s  grob %5.1f%%  fein %5.1f%%  (%.1f s, %d Renders)\n",
                             fp.name, kind == 0 ? "KICK" : "BASS", res.type == SoundKick ? "Kick" : "Bass",
                             res.initialMatch, res.match, secs, res.evaluations);
                if (only >= 0) printDiff (truth, res.params, kind == 0 ? "k_" : "b_");
            }
        }
        return 0;
    }

    if (mode == "analyze" && argc > 2)
    {
        std::vector<float> x; int fsr = 44100;
        if (! readWav (argv[2], x, fsr)) { std::printf ("Kann %s nicht lesen\n", argv[2]); return 1; }
        Reference ref = makeReference (x, fsr);
        AnalysisJob job;
        AnalysisResult res = analyzeAndFit (ref, defaultParams(), job);
        std::printf ("Typ: %s  grob %.1f%%  fein %.1f%%\n%s\n", res.type == SoundKick ? "Kick" : "Bass",
                     res.initialMatch, res.match, res.summary.c_str());
        printDiff (defaultParams(), res.params, res.type == SoundKick ? "k_" : "b_");
        if (argc > 3)
        {
            auto r = res.type == SoundKick ? renderKick (res.params, fsr, 145.0)
                                           : renderBassHz (res.params, fsr, res.bassHz, res.gateSec, (float) ref.x.size() / fsr + 0.01f);
            writeWav (argv[3], r.audio, fsr);
        }
        return 0;
    }
    std::printf ("Aufruf: psyforge_test render <dir> | roundtrip [index] | analyze <wav> [out.wav]\n");
    return 1;
}
