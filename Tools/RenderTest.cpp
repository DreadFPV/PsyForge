// PsyForge – Offline-Test ohne DAW/JUCE
//   psyforge_test render <outdir>            rendert alle Werks-Presets aller Instrumente als WAV
//   psyforge_test roundtrip [inst]           Preset verändern -> rendern -> analysieren -> Übereinstimmung
//   psyforge_test analyze <inst> <file.wav>  analysiert eine Datei für ein Instrument (0 = Kick … 9 = Riser)
//   psyforge_test patterns                   prüft alle Patterns (Loop-Grenzen) und Noten pro Instrument
#include "../Source/Presets.h"
#include "../Source/Analysis.h"
#include "../Source/Patterns.h"
#include <random>
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

static void printDiff (const SynthParams& a, const SynthParams& b, int inst)
{
    for (auto& d : paramTable())
    {
        if (! belongsTo (d.id, inst) || d.kind != KFloat) continue;
        std::printf ("    %-10s soll %9.3f  ist %9.3f\n", d.id, a.*(d.field), b.*(d.field));
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
            std::string base = dir + "/" + instrumentName (fp.inst) + "_" + fp.name;
            for (auto& c : base) if (c == ' ') c = '_';
            auto r = renderInstrument (p, fp.inst, sr, 145.0);
            writeWav (base + ".wav", r.audio, (int) sr);
            float pk = 0; for (float s : r.audio) pk = std::max (pk, std::abs (s));
            std::printf ("%-11s %-20s peak %.2f  %5.0f ms\n", instrumentName (fp.inst), fp.name, pk, r.audio.size() * 1000.0 / sr);
        }
        return 0;
    }

    if (mode == "patterns")
    {
        int bad = 0;
        for (int st = 0; st < NumStyles; ++st)
            for (int sec = 0; sec < NumSections; ++sec)
                for (int br = 0; br < NumBassRhythms; ++br)
                    for (int v : { 0, 3, 7 })
                    {
                        const auto pat = makePattern (st, sec, br, v);
                        const auto err = checkLoop (pat);
                        if (err.isNotEmpty()) { ++bad; std::printf ("FEHLER Stil %d Section %d: %s\n", st, sec, err.toRawUTF8()); }
                    }
        const auto pat = makePattern (2, Drop, 0, 0);
        int counts[NumInstruments] {};
        for (auto& e : pat.events) counts[e.voice]++;
        std::printf ("Full-On Drop: ");
        for (int i = 0; i < NumInstruments; ++i) std::printf ("%s %d  ", instrumentName (i), counts[i]);
        const auto mf = toMidiFile (pat, 6, 145.0, -1);
        std::printf ("\nMIDI alle Instrumente: %d Spuren\n%s\n", mf.getNumTracks(), bad == 0 ? "Alle Patterns ok" : "FEHLER");
        return bad;
    }

    if (mode == "roundtrip")
    {
        const int only = argc > 2 ? std::atoi (argv[2]) : -1;
        const int budget = argc > 3 ? std::atoi (argv[3]) : 6000;
        SynthParams cur = defaultParams();
        std::mt19937 rng (42); std::normal_distribution<float> nd (0, 1);
        for (int inst = 0; inst < NumInstruments; ++inst)
        {
            if (only >= 0 && inst != only) continue;
            const FactoryPreset* target = nullptr; int k = 0;
            for (auto& fp : factoryPresets()) if (fp.inst == inst && k++ == 2) { target = &fp; break; }
            SynthParams t = applyPreset (*target, cur); t.inst = (float) inst;
            if (inst >= 2)   // Drums: Ziel zufällig verändern, damit es kein Werks-Preset ist
                for (auto& d : paramTable())
                    if (belongsTo (d.id, inst) && d.kind == KFloat && ! std::strstr (d.id, "_level") && ! std::strstr (d.id, "_vel"))
                        t.*(d.field) = fromNorm (d, toNorm (d, t.*(d.field)) + nd (rng) * 0.12f);
            auto ro = inst == InKick ? renderKick (t, sr, 145.0) : inst == InBass ? renderBass (t, sr, 145.0, (int) t.note) : renderInstrument (t, inst, sr, 145.0);
            Reference ref = makeReference (ro.audio, sr);
            const auto t0 = std::chrono::steady_clock::now();
            AnalysisJob job; job.budget = budget;
            AnalysisResult res = analyzeInstrument (ref, defaultParams(), inst, job);
            const double secs = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
            std::printf ("%-11s (%-18s) grob %5.1f%%  fein %5.1f%%  (%.1f s, %d Renders)\n", instrumentName (inst), target->name,
                         res.initialMatch, res.match, secs, res.evaluations);
            std::fflush (stdout);
            if (only >= 0) printDiff (t, res.params, inst);
        }
        return 0;
    }

    if (mode == "analyze" && argc > 3)
    {
        const int inst = std::atoi (argv[2]);
        std::vector<float> x; int fsr = 44100;
        if (! readWav (argv[3], x, fsr)) { std::printf ("Kann %s nicht lesen\n", argv[3]); return 1; }
        Reference ref = makeReference (x, fsr);
        AnalysisJob job;
        AnalysisResult res = analyzeInstrument (ref, defaultParams(), inst, job);
        std::printf ("%s  grob %.1f%%  fein %.1f%%\n%s\n", instrumentName (inst), res.initialMatch, res.match, res.summary.c_str());
        printDiff (defaultParams(), res.params, inst);
        if (argc > 4)
        {
            auto r = inst == InBass ? renderBassHz (res.params, fsr, res.bassHz, res.gateSec, (float) ref.x.size() / fsr + 0.01f)
                                    : renderInstrument (res.params, inst, fsr, 145.0);
            writeWav (argv[4], r.audio, fsr);
        }
        return 0;
    }
    std::printf ("Aufruf: psyforge_test render <dir> | roundtrip [inst] [budget] | analyze <inst> <wav> [out.wav] | patterns\n");
    return 1;
}
