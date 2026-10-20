#pragma once
// PsyForge – WAV-Analyse: Kick/Bass-One-Shot erkennen, Grob-Einstellung schätzen,
// danach Feinanpassung (Pattern-Search) bis der gerenderte Klang dem Original sehr nahe kommt.
// Ohne JUCE, läuft in einem Hintergrund-Thread.
#include "Presets.h"
#include "Synth.h"
#include <complex>
#include <functional>
#include <atomic>
#include <random>
#include <string>
#include <cstdio>
#include <chrono>

namespace pf
{
enum SoundType { SoundKick = 0, SoundBass = 1, SoundAuto = 2 };

struct Reference
{
    std::vector<float> x;   // mono, normalisiert (Spitze 1), führende Stille entfernt
    double sr = 44100.0;
};

inline Reference makeReference (const std::vector<float>& in, double sr)
{
    Reference r; r.sr = sr;
    float peak = 0.0f;
    for (float s : in) peak = std::max (peak, std::abs (s));
    if (peak <= 1.0e-6f) return r;
    const float thr = peak * 0.003f;   // -50 dB
    size_t start = 0, end = in.size();
    while (start < in.size() && std::abs (in[start]) < thr) ++start;
    while (end > start && std::abs (in[end - 1]) < peak * 0.0005f) --end;   // -66 dB
    // ein paar Samples vor dem Einsatz behalten, aber auf Nulldurchgang achten
    start = start > 4 ? start - 4 : 0;
    const size_t maxLen = (size_t) (sr * 8.0);
    end = std::min (end, start + maxLen);
    r.x.assign (in.begin() + (long) start, in.begin() + (long) end);
    for (auto& s : r.x) s /= peak;
    return r;
}

//==============================================================================
// In-place-FFT (radix 2). Rechnet Real-/Imaginärteil von Hand: std::complex-Multiplikation ist ohne
// -ffast-math sehr langsam (NaN-Prüfungen), das macht hier den grössten Teil der Analysezeit aus.
inline void fft (std::vector<std::complex<float>>& data)
{
    const size_t n = data.size();
    float* a = reinterpret_cast<float*> (data.data());
    for (size_t i = 1, j = 0; i < n; ++i)
    {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) { std::swap (a[2 * i], a[2 * j]); std::swap (a[2 * i + 1], a[2 * j + 1]); }
    }
    for (size_t len = 2; len <= n; len <<= 1)
    {
        const double ang = -2.0 * 3.14159265358979323846 / (double) len;
        const size_t half = len / 2;
        // Drehfaktoren dieser Stufe einmal berechnen
        thread_local std::vector<float> tw;
        tw.resize (half * 2);
        for (size_t j = 0; j < half; ++j) { tw[2 * j] = (float) std::cos (ang * (double) j); tw[2 * j + 1] = (float) std::sin (ang * (double) j); }
        for (size_t i = 0; i < n; i += len)
            for (size_t j = 0; j < half; ++j)
            {
                float* u = a + 2 * (i + j);
                float* v = a + 2 * (i + j + half);
                const float wr = tw[2 * j], wi = tw[2 * j + 1];
                const float vr = v[0] * wr - v[1] * wi, vi = v[0] * wi + v[1] * wr;
                v[0] = u[0] - vr; v[1] = u[1] - vi;
                u[0] += vr; u[1] += vi;
            }
    }
}

//==============================================================================
// Merkmale eines Klangs (für den Vergleich)
struct Features
{
    std::vector<float> env;     // dB relativ zur Spitze, 1-ms-Raster
    std::vector<float> pitch;   // Hz pro 1-ms-Frame (0 = keine Angabe)
    std::vector<float> spec;    // [frame * bands + band] dB relativ zum Maximum
    std::vector<float> head;    // erste 30 ms Wellenform (normalisiert)
    std::vector<double> cross;  // Zeitpunkte der positiven Nulldurchgänge (s)
    std::vector<float> crossAmp;// Hüllkurve (linear) am jeweiligen Nulldurchgang
    int frames = 0, specFrames = 0, bands = 0;
};

struct FeatureSetup
{
    double sr = 44100.0;
    int frames = 0, specFrames = 0, fftSize = 1024, hop = 256, bands = 40, headLen = 0;
    std::vector<int> bandLo, bandHi;
    std::vector<float> window;
    void init (double sampleRate, int lengthSamples)
    {
        sr = sampleRate;
        const int frameLen = std::max (1, (int) std::lround (sr * 0.001));
        frames = std::min (8000, (lengthSamples + frameLen - 1) / frameLen);
        fftSize = sr > 60000.0 ? 2048 : 1024;
        hop = std::max (fftSize / 4, lengthSamples / 120);
        specFrames = std::min (121, (lengthSamples + hop - 1) / hop + 1);
        headLen = std::min (lengthSamples, (int) (sr * 0.03));
        window.resize ((size_t) fftSize);
        for (int i = 0; i < fftSize; ++i) window[(size_t) i] = 0.5f - 0.5f * std::cos (kTwoPi * (float) i / (float) fftSize);
        const double lo = 30.0, hi = std::min (18000.0, sr * 0.47);
        bandLo.clear(); bandHi.clear();
        for (int b = 0; b < bands; ++b)
        {
            const double f0 = lo * std::pow (hi / lo, (double) b / bands);
            const double f1 = lo * std::pow (hi / lo, (double) (b + 1) / bands);
            int i0 = (int) std::floor (f0 * fftSize / sr), i1 = (int) std::ceil (f1 * fftSize / sr);
            i0 = std::max (1, i0); i1 = std::max (i0 + 1, i1);
            bandLo.push_back (i0); bandHi.push_back (std::min (fftSize / 2, i1));
        }
    }
};

inline Features computeFeatures (const float* x, int n, const FeatureSetup& fs, bool withPitch)
{
    Features f;
    const double sr = fs.sr;
    const int frameLen = std::max (1, (int) std::lround (sr * 0.001));
    f.frames = fs.frames;
    f.env.assign ((size_t) fs.frames, -80.0f);
    float peak = 1.0e-9f;
    for (int i = 0; i < n; ++i) peak = std::max (peak, std::abs (x[i]));
    // Hüllkurve über das analytische Signal (Hilbert) – unabhängig von der Phase der Schwingung.
    // Lange Klänge (> 0,6 s: Open Hat, Ride, Riser) über RMS je Millisekunde (viel schneller, dort genau genug).
    std::vector<float> rms ((size_t) fs.frames, 0.0f);
    float maxRms = 1.0e-9f;
    if (n > (int) (sr * 0.6))
    {
        for (int fr = 0; fr < fs.frames; ++fr)
        {
            double acc = 0; int cnt = 0;
            for (int i = std::max (0, fr * frameLen - frameLen); i < (fr + 2) * frameLen && i < n; ++i) { acc += (double) x[i] * x[i]; ++cnt; }
            rms[(size_t) fr] = cnt > 0 ? (float) std::sqrt (acc / cnt * 2.0) : 0.0f;
            maxRms = std::max (maxRms, rms[(size_t) fr]);
        }
    }
    else
    {
        size_t N = 1; while (N < (size_t) n + 2048) N <<= 1;
        std::vector<std::complex<float>> a (N, { 0.0f, 0.0f });
        for (int i = 0; i < n; ++i) a[(size_t) i] = { x[i], 0.0f };
        fft (a);
        for (size_t k = 1; k < N / 2; ++k) a[k] *= 2.0f;
        for (size_t k = N / 2 + 1; k < N; ++k) a[k] = { 0.0f, 0.0f };
        for (auto& v : a) v = std::conj (v);
        fft (a);
        const float inv = 1.0f / (float) N;
        for (int fr = 0; fr < fs.frames; ++fr)
        {
            double acc = 0; int cnt = 0;
            for (int i = fr * frameLen; i < (fr + 1) * frameLen && i < n; ++i) { acc += std::abs (a[(size_t) i]) * inv; ++cnt; }
            rms[(size_t) fr] = cnt > 0 ? (float) (acc / cnt) : 0.0f;
            maxRms = std::max (maxRms, rms[(size_t) fr]);
        }
    }
    for (int fr = 0; fr < fs.frames; ++fr)
        f.env[(size_t) fr] = std::max (-80.0f, 20.0f * std::log10 (rms[(size_t) fr] / maxRms + 1.0e-6f));

    // Tonhöhe über Nulldurchgänge (leicht tiefpassgefiltert)
    if (withPitch)
    {
        f.pitch.assign ((size_t) fs.frames, 0.0f);
        Svf lp1, lp2; lp1.set (5000.0f, 0.6f, (float) sr); lp2.set (5000.0f, 0.6f, (float) sr);
        float prev = 0.0f; double lastCross = -1.0;
        for (int i = 0; i < n; ++i)
        {
            const float y = lp2.lp (lp1.lp (x[i]));
            if (prev < 0.0f && y >= 0.0f)
            {
                const double tc = (i - 1) + (double) (-prev) / (double) (y - prev + 1.0e-12f);
                {
                    const int fr = std::clamp ((int) (tc / frameLen), 0, fs.frames - 1);
                    f.cross.push_back (tc / sr);
                    f.crossAmp.push_back (rms[(size_t) fr] / maxRms);
                }
                if (lastCross >= 0.0)
                {
                    const double period = tc - lastCross;
                    if (period > 0.5)
                    {
                        const float hz = (float) (sr / period);
                        const int fa = std::max (0, (int) (lastCross / frameLen)), fb = std::min (fs.frames - 1, (int) (tc / frameLen));
                        for (int fr = fa; fr <= fb; ++fr) f.pitch[(size_t) fr] = hz;
                    }
                }
                lastCross = tc;
            }
            prev = y;
        }
        for (int fr = 0; fr < fs.frames; ++fr)
            if (f.env[(size_t) fr] < -45.0f) f.pitch[(size_t) fr] = 0.0f;
    }

    // Spektrogramm in log. Bändern
    f.specFrames = fs.specFrames; f.bands = fs.bands;
    f.spec.assign ((size_t) (fs.specFrames * fs.bands), -80.0f);
    std::vector<std::complex<float>> buf ((size_t) fs.fftSize);
    float maxDb = -1.0e9f;
    std::vector<float> raw ((size_t) (fs.specFrames * fs.bands), 0.0f);
    for (int sf = 0; sf < fs.specFrames; ++sf)
    {
        const int start = sf * fs.hop - fs.fftSize / 2;
        for (int i = 0; i < fs.fftSize; ++i)
        {
            const int idx = start + i;
            buf[(size_t) i] = { (idx >= 0 && idx < n) ? x[idx] * fs.window[(size_t) i] : 0.0f, 0.0f };
        }
        fft (buf);
        for (int b = 0; b < fs.bands; ++b)
        {
            double e = 0;
            for (int k = fs.bandLo[(size_t) b]; k < fs.bandHi[(size_t) b]; ++k) e += std::norm (buf[(size_t) k]);
            const float db = 10.0f * std::log10 ((float) e / (float) (fs.bandHi[(size_t) b] - fs.bandLo[(size_t) b]) + 1.0e-12f);
            raw[(size_t) (sf * fs.bands + b)] = db;
            maxDb = std::max (maxDb, db);
        }
    }
    for (size_t i = 0; i < raw.size(); ++i) f.spec[i] = std::max (-80.0f, raw[i] - maxDb);

    f.head.assign ((size_t) fs.headLen, 0.0f);
    for (int i = 0; i < fs.headLen && i < n; ++i) f.head[(size_t) i] = x[i] / peak;
    return f;
}

struct LossWeights { float env = 1.0f, pitch = 2.5f, spec = 0.6f, head = 0.3f; };

struct LossParts { float env = 0, pitch = 0, spec = 0, head = 0, total = 0; };

inline LossParts compareFeatures (const Features& t, const Features& c, const LossWeights& w)
{
    LossParts L;
    // Hüllkurve
    {
        double acc = 0, wsum = 0;
        for (int i = 0; i < t.frames; ++i)
        {
            const float a = t.env[(size_t) i], b = i < c.frames ? c.env[(size_t) i] : -80.0f;
            const float m = std::max (a, b);
            if (m < -60.0f) continue;
            const float ww = 0.25f + (m + 60.0f) / 60.0f;          // laute Stellen zählen mehr
            acc += ww * std::min (30.0f, std::abs (std::max (a, -60.0f) - std::max (b, -60.0f))); wsum += ww;
        }
        L.env = wsum > 0 ? (float) (acc / wsum) : 0.0f;
    }
    // Tonhöhe
    if (! t.pitch.empty() && ! c.pitch.empty())
    {
        double acc = 0, wsum = 0;
        for (int i = 0; i < t.frames; ++i)
        {
            const float ft = t.pitch[(size_t) i];
            if (ft <= 0.0f || t.env[(size_t) i] < -30.0f) continue;
            const float fc = i < c.frames ? c.pitch[(size_t) i] : 0.0f;
            const float d = fc > 0.0f ? std::min (12.0f, std::abs (12.0f * std::log2 (fc / ft))) : 12.0f;
            const float ww = std::pow (10.0f, t.env[(size_t) i] / 20.0f);
            acc += d * ww; wsum += ww;
        }
        L.pitch = wsum > 0 ? (float) (acc / wsum) : 0.0f;
    }
    // Spektrum
    {
        double acc = 0, wsum = 0;
        for (int i = 0; i < t.specFrames * t.bands; ++i)
        {
            const float a = t.spec[(size_t) i], b = i < c.specFrames * c.bands ? c.spec[(size_t) i] : -80.0f;
            const float m = std::max (a, b);
            if (m < -60.0f) continue;
            const float x = (m + 60.0f) / 60.0f, ww = x * x;        // leise Bänder zählen weniger
            acc += ww * std::min (30.0f, std::abs (std::max (a, -60.0f) - std::max (b, -60.0f))); wsum += ww;
        }
        L.spec = wsum > 0 ? (float) (acc / wsum) : 0.0f;
    }
    // Wellenform-Anfang (Phase / Anschlag)
    {
        double xy = 0, xx = 0, yy = 0;
        for (size_t i = 0; i < t.head.size(); ++i)
        {
            const float a = t.head[i], b = i < c.head.size() ? c.head[i] : 0.0f;
            xy += a * b; xx += a * a; yy += b * b;
        }
        L.head = (xx > 0 && yy > 0) ? (float) (1.0 - xy / std::sqrt (xx * yy)) : 1.0f;
    }
    L.total = w.env * L.env + w.pitch * L.pitch + w.spec * L.spec + w.head * L.head;
    return L;
}

inline float matchPercent (float loss) { return 100.0f * std::exp (-loss / 10.0f); }

//==============================================================================
// Einfache Messungen am Original
inline float yinPitch (const std::vector<float>& x, double sr, int from, int to)
{
    from = std::max (0, from); to = std::min ((int) x.size(), to);
    const int minLag = std::max (2, (int) (sr / 600.0)), maxLag = (int) (sr / 22.0);
    const int W = to - from - maxLag;
    if (W < (int) (sr * 0.01)) return 0.0f;
    std::vector<double> d ((size_t) maxLag + 2, 0.0);
    for (int tau = 1; tau <= maxLag + 1; ++tau)
    {
        double acc = 0;
        for (int i = from; i < from + W; ++i) { const double v = x[(size_t) i] - x[(size_t) (i + tau)]; acc += v * v; }
        d[(size_t) tau - 1] = acc;
    }
    // kumulativ normalisiert
    std::vector<double> cm ((size_t) maxLag + 1, 1.0);
    double run = 0;
    for (int tau = 1; tau <= maxLag; ++tau)
    {
        run += d[(size_t) tau - 1];
        cm[(size_t) tau] = run > 0 ? d[(size_t) tau - 1] * tau / run : 1.0;
    }
    int best = -1;
    for (int tau = minLag; tau < maxLag; ++tau)
        if (cm[(size_t) tau] < 0.15 && cm[(size_t) tau] <= cm[(size_t) tau + 1]) { best = tau; break; }
    if (best < 0)
    {
        double m = 1e9;
        for (int tau = minLag; tau < maxLag; ++tau) if (cm[(size_t) tau] < m) { m = cm[(size_t) tau]; best = tau; }
    }
    double tauF = best;
    if (best > minLag && best < maxLag - 1)
    {
        const double a = cm[(size_t) best - 1], b = cm[(size_t) best], c = cm[(size_t) best + 1];
        const double den = a - 2 * b + c;
        if (std::abs (den) > 1e-12) tauF = best + 0.5 * (a - c) / den;
    }
    return (float) (sr / tauF);
}

// Betrag der DFT bei einer beliebigen Frequenz (Hann-Fenster über [from, to))
inline float dftMag (const std::vector<float>& x, double sr, int from, int to, float f)
{
    double re = 0, im = 0;
    const int len = std::max (1, to - from);
    const double w = 2.0 * 3.14159265358979 * f / sr;
    double c = 1, s = 0; const double cw = std::cos (w), sw = std::sin (w);
    for (int i = 0; i < len; ++i)
    {
        const double win = 0.5 - 0.5 * std::cos (2.0 * 3.14159265358979 * i / len);
        const double v = x[(size_t) (from + i)] * win;
        re += v * c; im -= v * s;
        const double c2 = c * cw - s * sw; s = s * cw + c * sw; c = c2;
    }
    return (float) std::sqrt (re * re + im * im);
}

// Grundton fein über die normierte Autokorrelation in der Nähe einer Schätzung
// (unabhängig vom Lautstärke-Abfall, funktioniert auch bei kurzen Noten). 0 = nicht gefunden.
inline float nccPitch (const std::vector<float>& x, double sr, float guess, float range = 1.25f)
{
    const int n = (int) x.size();
    const int from = (int) (sr * 0.004);
    const int tMin = std::max (2, (int) (sr / (guess * range))), tMax = (int) (sr / (guess / range));
    if (n - from - tMax <= (int) (sr * 0.015)) return 0.0f;
    std::vector<double> ncc ((size_t) tMax + 2, -1.0);
    for (int tau = tMin; tau <= tMax + 1; ++tau)
    {
        double xy = 0, xx = 0, yy = 0;
        for (int i = from; i + tau < n; ++i)
        {
            const double a = x[(size_t) i], b = x[(size_t) (i + tau)];
            xy += a * b; xx += a * a; yy += b * b;
        }
        ncc[(size_t) tau] = (xx > 0 && yy > 0) ? xy / std::sqrt (xx * yy) : -1.0;
    }
    int best = tMin;
    for (int tau = tMin; tau <= tMax; ++tau) if (ncc[(size_t) tau] > ncc[(size_t) best]) best = tau;
    double tauF = best;
    if (best > tMin && best < tMax)
    {
        const double a = ncc[(size_t) best - 1], b = ncc[(size_t) best], c = ncc[(size_t) best + 1];
        const double den = a - 2 * b + c;
        if (std::abs (den) > 1e-12) tauF = best + 0.5 * (a - c) / den;
    }
    return ncc[(size_t) best] > 0.5 ? (float) (sr / tauF) : 0.0f;
}

// Grundton eines Basses: YIN als Startwert, Oktav-Prüfung über das Spektrum,
// dann exakt über den Phasenverlauf des isolierten Grundtons (analytisches Signal, gewichtete Regression)
inline float estimateBassPitch (const std::vector<float>& x, double sr)
{
    const int n = (int) x.size();
    float y = yinPitch (x, sr, (int) (sr * 0.008), std::min (n, (int) (sr * 0.3)));
    if (y <= 0.0f || n < 512) return y;
    size_t N = 1; while (N < (size_t) n * 2) N <<= 1;
    std::vector<std::complex<float>> X (N, { 0.0f, 0.0f });
    for (int i = 0; i < n; ++i) X[(size_t) i] = { x[(size_t) i], 0.0f };
    fft (X);
    auto bandEnergy = [&] (float f)
    {
        double e = 0;
        const int k0 = (int) (f * 0.85f * N / sr), k1 = (int) (f * 1.15f * N / sr) + 1;
        for (int k = std::max (1, k0); k <= k1 && k < (int) N / 2; ++k) e += std::norm (X[(size_t) k]);
        return e;
    };
    // Oktave tiefer, wenn dort deutlich Energie liegt (YIN springt bei obertonreichen Klängen gern hoch)
    for (int it = 0; it < 2; ++it)
        if (y * 0.5f > 20.0f && bandEnergy (y * 0.5f) > 0.3 * bandEnergy (y)) y *= 0.5f;

    {
        const float f = nccPitch (x, sr, y);
        if (f > 0.0f) return f;
    }

    // nur den Grundton behalten, nur positive Frequenzen -> analytisches Signal
    std::vector<std::complex<float>> Z (N, { 0.0f, 0.0f });
    const int k0 = (int) (y * 0.6f * N / sr), k1 = (int) (y * 1.4f * N / sr) + 1;
    for (int k = std::max (1, k0); k <= k1 && k < (int) N / 2; ++k)
    {
        const float pos = (float) (k - k0) / (float) std::max (1, k1 - k0);
        const float w = 0.5f - 0.5f * std::cos (kTwoPi * pos);     // weiche Bandkanten
        Z[(size_t) k] = X[(size_t) k] * (2.0f * w);
    }
    for (auto& v : Z) v = std::conj (v);
    fft (Z);
    for (auto& v : Z) v = std::conj (v) / (float) N;
    float amax = 0;
    for (int i = 0; i < n; ++i) amax = std::max (amax, std::abs (Z[(size_t) i]));
    // gewichtete lineare Regression der entfalteten Phase
    double sw = 0, st = 0, sp = 0, stt = 0, stp = 0, unwrap = 0, prevPh = 0;
    const int from = (int) (sr * 0.012);
    bool first = true;
    for (int i = 0; i < n; ++i)
    {
        const double ph = std::arg (Z[(size_t) i]);
        if (first) { prevPh = ph; first = false; }
        double d = ph - prevPh;
        while (d > 3.14159265358979) d -= 2 * 3.14159265358979;
        while (d < -3.14159265358979) d += 2 * 3.14159265358979;
        unwrap += d; prevPh = ph;
        const double a = std::abs (Z[(size_t) i]) / amax;
        if (i < from || a < 0.2) continue;
        const double w = a * a, t = (double) i / sr;
        sw += w; st += w * t; sp += w * unwrap; stt += w * t * t; stp += w * t * unwrap;
    }
    const double den = sw * stt - st * st;
    if (sw <= 0 || std::abs (den) < 1e-18) return y;
    const double slope = (sw * stp - st * sp) / den;          // rad/s
    const float f = (float) (slope / (2.0 * 3.14159265358979));
    if (f <= 0 || std::abs (12.0f * std::log2 (f / y)) > 3.0f) return y;
    return f;
}

// Grundton präzisieren: Bandpass um die Schätzung, mittlere Periode über die Nulldurchgänge
inline float refinePitch (const std::vector<float>& x, double sr, float f0)
{
    if (f0 <= 0.0f) return f0;
    for (int pass = 0; pass < 2; ++pass)
    {
        Svf bp1, bp2; bp1.set (f0, 2.0f, (float) sr); bp2.set (f0, 2.0f, (float) sr);
        float peak = 0; for (float v : x) peak = std::max (peak, std::abs (v));
        std::vector<double> cr;
        float prev = 0, env = 0;
        const int from = (int) (sr * 0.012), to = (int) (x.size() * 0.85);
        for (int i = 0; i < (int) x.size(); ++i)
        {
            const float y = bp2.bp (bp1.bp (x[(size_t) i]));
            env = std::max (std::abs (x[(size_t) i]), env * 0.999f);
            if (i > from && i < to && prev < 0 && y >= 0 && env > peak * 0.05f)
                cr.push_back ((i - 1) + (double) (-prev) / (double) (y - prev + 1e-12f));
            prev = y;
        }
        if (cr.size() < 3) return f0;
        const double period = (cr.back() - cr.front()) / (double) (cr.size() - 1);
        const float f = (float) (sr / period);
        if (std::abs (12.0f * std::log2 (f / f0)) > 2.0f) return f0;   // Ausreisser: Schätzung behalten
        f0 = f;
    }
    return f0;
}

inline float medianOf (std::vector<float> v)
{
    if (v.empty()) return 0.0f;
    std::nth_element (v.begin(), v.begin() + (long) v.size() / 2, v.end());
    return v[v.size() / 2];
}

// Anteil der Energie oberhalb von 2,5 x Grundton in einem Abschnitt
inline float harmonicRatio (const std::vector<float>& x, double sr, int from, int len, float f0)
{
    int N = 1; while (N < len) N <<= 1;
    N = std::max (N, 4096);
    std::vector<std::complex<float>> buf ((size_t) N, { 0.0f, 0.0f });
    for (int i = 0; i < len && from + i < (int) x.size(); ++i)
    {
        const float w = 0.5f - 0.5f * std::cos (kTwoPi * (float) i / (float) len);
        buf[(size_t) i] = { x[(size_t) (from + i)] * w, 0.0f };
    }
    fft (buf);
    double hi = 0, all = 0;
    for (int k = 1; k < N / 2; ++k)
    {
        const double e = std::norm (buf[(size_t) k]);
        const double hz = k * sr / N;
        all += e; if (hz > f0 * 2.5) hi += e;
    }
    return all > 0 ? (float) (hi / all) : 0.0f;
}

inline std::vector<float> harmonicAmps (const std::vector<float>& x, double sr, int from, int len, float f0, int count)
{
    int N = 1; while (N < len) N <<= 1;
    N = std::max (N, 8192);
    std::vector<std::complex<float>> buf ((size_t) N, { 0.0f, 0.0f });
    for (int i = 0; i < len && from + i < (int) x.size(); ++i)
    {
        const float w = 0.5f - 0.5f * std::cos (kTwoPi * (float) i / (float) len);
        buf[(size_t) i] = { x[(size_t) (from + i)] * w, 0.0f };
    }
    fft (buf);
    std::vector<float> h;
    for (int n = 1; n <= count; ++n)
    {
        const double bin = f0 * n * N / sr;
        float m = 0;
        for (int k = (int) bin - 2; k <= (int) bin + 2; ++k)
            if (k > 0 && k < N / 2) m = std::max (m, std::abs (buf[(size_t) k]));
        h.push_back (m);
    }
    return h;
}

//==============================================================================
struct AnalysisJob
{
    int forceType = SoundAuto;
    std::atomic<bool>* cancel = nullptr;
    std::function<void (float progress, float match)> progress;
    int budget = 6000;           // max. Renders
    bool fine = true;            // Feinanpassung aktiv
    double maxSeconds = 45.0;    // Zeitlimit der Feinanpassung (lange Klänge wie Ride/Riser)
    std::function<void (const char* stage, const SynthParams&, float gate)> stageHook;   // nur für Tests
};

struct AnalysisResult
{
    bool ok = false;
    SoundType type = SoundKick;
    SynthParams params;
    float initialMatch = 0, match = 0;
    int evaluations = 0;
    float bassHz = 0, gateSec = 0;
    int detectedNote = -1; float detectedCents = 0;
    float kickEnd = 0, measuredHz = 0;
    int inst = InKick;
    std::string summary;
};

namespace detail
{
struct Var { float SynthParams::* field; const PDef* def; };   // field == nullptr -> Gate (Bass)

using Objective = std::function<float (const SynthParams&, float gate)>;

inline float getNorm (const Var& v, const SynthParams& p, float gate)
{
    return v.field ? toNorm (*v.def, p.*(v.field)) : std::clamp ((gate - 0.005f) / 0.995f, 0.0f, 1.0f);
}
inline void setNorm (const Var& v, SynthParams& p, float& gate, float u)
{
    if (v.field) p.*(v.field) = fromNorm (*v.def, u);
    else gate = 0.005f + std::clamp (u, 0.0f, 1.0f) * 0.995f;
}

// Pattern-Search in normierten Koordinaten. Gibt den besten Wert zurück, p/gate werden ersetzt.
inline float patternSearch (SynthParams& p, float& gate, const std::vector<Var>& vars, const Objective& obj,
                            float step0, float stepMin, int maxCalls, const std::function<bool()>& stop, unsigned seed = 1234u)
{
    int calls = 0;
    auto eval = [&] (const SynthParams& q, float g) { ++calls; return obj (q, g); };
    float best = eval (p, gate);
    float step = step0;
    std::mt19937 rng (seed + (unsigned) vars.size());
    std::vector<size_t> order (vars.size());
    for (size_t i = 0; i < order.size(); ++i) order[i] = i;
    while (step >= stepMin && calls < maxCalls && ! stop())
    {
        bool improved = false;
        std::shuffle (order.begin(), order.end(), rng);
        for (size_t oi : order)
        {
            const Var& v = vars[oi];
            const float u0 = getNorm (v, p, gate);
            for (int sgn : { +1, -1 })
            {
                const float u = std::clamp (u0 + (float) sgn * step, 0.0f, 1.0f);
                if (std::abs (u - u0) < 1e-6f) continue;
                SynthParams q = p; float g = gate;
                setNorm (v, q, g, u);
                const float L = eval (q, g);
                if (L < best - 1e-6f)
                {
                    best = L; p = q; gate = g; improved = true;
                    float stepX = step;
                    for (int k = 0; k < 5; ++k)          // weiter in dieselbe Richtung
                    {
                        stepX *= 2.0f;
                        const float u2 = std::clamp (getNorm (v, p, gate) + (float) sgn * stepX, 0.0f, 1.0f);
                        SynthParams q2 = p; float g2 = gate;
                        setNorm (v, q2, g2, u2);
                        const float L2 = eval (q2, g2);
                        if (L2 < best - 1e-6f) { best = L2; p = q2; gate = g2; } else break;
                    }
                    break;
                }
            }
            if (calls >= maxCalls || stop()) break;
        }
        if (! improved) step *= 0.5f;
    }
    return best;
}

// sep-CMA-ES (diagonale Kovarianz) – robust gegen lokale Minima, für die Feinanpassung
inline float cmaSearch (SynthParams& p, float& gate, const std::vector<Var>& vars, const Objective& obj,
                        float sigma0, int maxCalls, const std::function<bool()>& stop, unsigned seed = 4242u)
{
    const int n = (int) vars.size();
    if (n == 0) return obj (p, gate);
    std::mt19937 rng (seed);
    std::normal_distribution<double> nd (0.0, 1.0);
    const int lambda = 4 + (int) std::floor (3.0 * std::log ((double) n)) + 4;
    const int mu = lambda / 2;
    std::vector<double> w ((size_t) mu);
    double wsum = 0, w2 = 0;
    for (int i = 0; i < mu; ++i) { w[(size_t) i] = std::log (mu + 0.5) - std::log (i + 1.0); wsum += w[(size_t) i]; }
    for (auto& v : w) { v /= wsum; w2 += v * v; }
    const double mueff = 1.0 / w2;
    const double cs = (mueff + 2.0) / (n + mueff + 5.0);
    const double ds = 1.0 + 2.0 * std::max (0.0, std::sqrt ((mueff - 1.0) / (n + 1.0)) - 1.0) + cs;
    const double cc = (4.0 + mueff / n) / (n + 4.0 + 2.0 * mueff / n);
    double c1 = 2.0 / ((n + 1.3) * (n + 1.3) + mueff);
    double cmu = std::min (1.0 - c1, 2.0 * (mueff - 2.0 + 1.0 / mueff) / ((n + 2.0) * (n + 2.0) + mueff));
    c1 *= (n + 2.0) / 3.0; cmu = std::min (1.0 - c1, cmu * (n + 2.0) / 3.0);
    const double chiN = std::sqrt ((double) n) * (1.0 - 1.0 / (4.0 * n) + 1.0 / (21.0 * n * n));

    std::vector<double> m ((size_t) n), C ((size_t) n, 1.0), ps ((size_t) n, 0.0), pc ((size_t) n, 0.0);
    for (int i = 0; i < n; ++i) m[(size_t) i] = getNorm (vars[(size_t) i], p, gate);
    double sigma = sigma0;
    float best = obj (p, gate); int calls = 1;
    SynthParams bestP = p; float bestG = gate;

    struct Ind { std::vector<double> y, x; float f; };
    std::vector<Ind> pop ((size_t) lambda);
    int gen = 0;
    while (calls + lambda <= maxCalls && ! stop() && sigma > 1e-4)
    {
        for (auto& ind : pop)
        {
            ind.y.assign ((size_t) n, 0.0); ind.x.assign ((size_t) n, 0.0);
            SynthParams q = p; float g = gate;
            for (int i = 0; i < n; ++i)
            {
                ind.y[(size_t) i] = std::sqrt (C[(size_t) i]) * nd (rng);
                double x = m[(size_t) i] + sigma * ind.y[(size_t) i];
                if (x < 0.0) x = -x;                                     // an den Grenzen spiegeln
                if (x > 1.0) x = 2.0 - x;
                x = std::clamp (x, 0.0, 1.0);
                ind.x[(size_t) i] = x;
                ind.y[(size_t) i] = (x - m[(size_t) i]) / sigma;
                setNorm (vars[(size_t) i], q, g, (float) x);
            }
            ind.f = obj (q, g); ++calls;
            if (ind.f < best) { best = ind.f; bestP = q; bestG = g; }
        }
        std::sort (pop.begin(), pop.end(), [] (const Ind& a, const Ind& b) { return a.f < b.f; });
        std::vector<double> mOld = m, yw ((size_t) n, 0.0);
        for (int i = 0; i < n; ++i)
        {
            double acc = 0;
            for (int k = 0; k < mu; ++k) acc += w[(size_t) k] * pop[(size_t) k].y[(size_t) i];
            yw[(size_t) i] = acc;
            m[(size_t) i] = std::clamp (mOld[(size_t) i] + sigma * acc, 0.0, 1.0);
        }
        double psn = 0;
        for (int i = 0; i < n; ++i)
        {
            ps[(size_t) i] = (1 - cs) * ps[(size_t) i] + std::sqrt (cs * (2 - cs) * mueff) * yw[(size_t) i] / std::sqrt (C[(size_t) i]);
            psn += ps[(size_t) i] * ps[(size_t) i];
        }
        psn = std::sqrt (psn);
        ++gen;
        const bool hs = psn / std::sqrt (1.0 - std::pow (1.0 - cs, 2.0 * gen)) < (1.4 + 2.0 / (n + 1.0)) * chiN;
        for (int i = 0; i < n; ++i)
        {
            pc[(size_t) i] = (1 - cc) * pc[(size_t) i] + (hs ? std::sqrt (cc * (2 - cc) * mueff) * yw[(size_t) i] : 0.0);
            double rank = 0;
            for (int k = 0; k < mu; ++k) rank += w[(size_t) k] * pop[(size_t) k].y[(size_t) i] * pop[(size_t) k].y[(size_t) i];
            C[(size_t) i] = (1 - c1 - cmu) * C[(size_t) i]
                          + c1 * (pc[(size_t) i] * pc[(size_t) i] + (hs ? 0.0 : cc * (2 - cc) * C[(size_t) i]))
                          + cmu * rank;
            C[(size_t) i] = std::clamp (C[(size_t) i], 1e-8, 4.0);
        }
        sigma *= std::exp ((cs / ds) * (psn / chiN - 1.0));
        sigma = std::min (sigma, 0.5);
    }
    p = bestP; gate = bestG;
    return best;
}

inline std::vector<Var> vars (std::initializer_list<const char*> ids)
{
    std::vector<Var> v;
    for (auto* id : ids)
        if (std::string (id) == "gate") v.push_back ({ nullptr, nullptr });
        else if (auto* d = findParam (id)) v.push_back ({ d->field, d });
    return v;
}

// Hüllkurve (linear, 1-ms-Raster) des Originals
inline std::vector<float> linearEnv (const Reference& r)
{
    const int fl = std::max (1, (int) std::lround (r.sr * 0.001));
    std::vector<float> e;
    for (size_t i = 0; i < r.x.size(); i += (size_t) fl)
    {
        float m = 0;
        for (size_t j = i; j < i + (size_t) fl && j < r.x.size(); ++j) m = std::max (m, std::abs (r.x[j]));
        e.push_back (m);
    }
    std::vector<float> s (e.size());
    for (size_t i = 0; i < e.size(); ++i)
    {
        float m = 0;
        for (size_t j = i; j < std::min (e.size(), i + 6); ++j) m = std::max (m, e[j]);
        s[i] = m;
    }
    return s;
}

// Kick-Tonhöhenverlauf direkt gegen die gemessenen Nulldurchgänge prüfen (ohne Rendern, sehr schnell):
// zwischen zwei Nulldurchgängen muss das Modell genau eine Schwingung machen.
inline float kickPitchCurveError (const SynthParams& p, const Features& t, double sr)
{
    if (t.cross.size() < 3) return 0.0f;
    SynthParams q = p; q.kSync = 0; q.kSnap = SnapOff; q.kFollow = 0;
    const KickShape k = makeKickShape (q, 145.0);
    const double dt = 2.0 / sr;
    double phase = 0.0, time = 0.0;
    std::vector<double> ph (t.cross.size());
    size_t ci = 0;
    while (ci < t.cross.size())
    {
        const double target = t.cross[ci];
        while (time + dt <= target)
        {
            phase += kickFreqAt (k, (float) (time + dt * 0.5)) * dt;
            time += dt;
        }
        const double rest = target - time;
        ph[ci] = phase + kickFreqAt (k, (float) (time + rest * 0.5)) * rest;
        ++ci;
    }
    double acc = 0, wsum = 0;
    for (size_t i = 0; i + 1 < ph.size(); ++i)
    {
        const double cyc = ph[i + 1] - ph[i];
        const double w = t.crossAmp[i];
        if (w < 0.03) continue;                               // unter -30 dB ignorieren
        const double err = std::min (12.0, std::abs (12.0 * std::log2 (std::max (0.02, cyc))));
        acc += err * w; wsum += w;
    }
    return wsum > 0 ? (float) (acc / wsum) : 0.0f;
}

// Kick-Amplitude (Modell ohne Drive/Click) gegen die gemessene Hüllkurve
inline float kickEnvCurveError (const SynthParams& p, const Features& t)
{
    SynthParams q = p; q.kSync = 0; q.kSnap = SnapOff; q.kFollow = 0;
    const KickShape k = makeKickShape (q, 145.0);
    std::vector<float> m ((size_t) t.frames);
    float mx = 1e-9f;
    const float dNorm = std::max (1e-6f, std::abs (driveShape (1.0f, k.drive, k.driveType)));
    for (int i = 0; i < t.frames; ++i)
    {
        const float a = kickAmpAt (k, (float) ((i + 0.5) * 0.001));
        m[(size_t) i] = std::abs (driveShape (a, k.drive, k.driveType)) / dNorm;   // Sättigung staucht die Hüllkurve
        mx = std::max (mx, m[(size_t) i]);
    }
    double acc = 0; int cnt = 0;
    for (int i = 0; i < t.frames; ++i)
    {
        const float a = t.env[(size_t) i], b = std::max (-80.0f, 20.0f * std::log10 (m[(size_t) i] / mx + 1e-6f));
        if (std::max (a, b) < -50.0f) continue;
        acc += std::min (30.0f, std::abs (std::max (a, -50.0f) - std::max (b, -50.0f))); ++cnt;
    }
    return cnt ? (float) (acc / cnt) : 0.0f;
}

// Bass-Amplitude (ADSR-Modell) gegen die gemessene Hüllkurve
inline float bassEnvCurveError (const SynthParams& p, float gate, const std::vector<float>& envDb)
{
    const BassShape b = makeBassShape (p);
    struct { int frames; const std::vector<float>& env; } t { (int) envDb.size(), envDb };
    std::vector<float> m ((size_t) t.frames);
    float mx = 1e-9f, relStart = 0;
    for (int i = 0; i < t.frames; ++i)
    {
        const float tt = (float) ((i + 0.5) * 0.001);
        auto gated = [&] (float x) { return x < b.attack ? x / std::max (1e-6f, b.attack)
                                                         : b.sustain + (1.0f - b.sustain) * std::exp (-(x - b.attack) / b.tauDecay); };
        float e;
        if (tt < gate) e = gated (tt);
        else { relStart = gated (gate); e = relStart * std::exp (-(tt - gate) / b.tauRelease); }
        m[(size_t) i] = e; mx = std::max (mx, e);
    }
    double acc = 0; int cnt = 0;
    for (int i = 0; i < t.frames; ++i)
    {
        const float a = t.env[(size_t) i], bb = std::max (-80.0f, 20.0f * std::log10 (m[(size_t) i] / mx + 1e-6f));
        if (std::max (a, bb) < -50.0f) continue;
        acc += std::min (30.0f, std::abs (std::max (a, -50.0f) - std::max (bb, -50.0f))); ++cnt;
    }
    return cnt ? (float) (acc / cnt) : 0.0f;
}

// Hüllkurve nur des Grundtons (Bandpass) – zeigt die Lautstärke-Hüllkurve ohne den Filter-'Plock'
inline std::vector<float> fundamentalEnvDb (const Reference& r, float f0, int frames)
{
    Svf a, b; a.set (f0, 1.5f, (float) r.sr); b.set (f0, 1.5f, (float) r.sr);
    const int fl = std::max (1, (int) std::lround (r.sr * 0.001));
    std::vector<float> y (r.x.size());
    for (size_t i = 0; i < r.x.size(); ++i) y[i] = b.bp (a.bp (r.x[i]));
    // Hilbert-Betrag
    size_t N = 1; while (N < y.size() + 2048) N <<= 1;
    std::vector<std::complex<float>> c (N, { 0.0f, 0.0f });
    for (size_t i = 0; i < y.size(); ++i) c[i] = { y[i], 0.0f };
    fft (c);
    for (size_t k = 1; k < N / 2; ++k) c[k] *= 2.0f;
    for (size_t k = N / 2 + 1; k < N; ++k) c[k] = { 0.0f, 0.0f };
    for (auto& v : c) v = std::conj (v);
    fft (c);
    std::vector<float> e ((size_t) frames, 0.0f);
    float mx = 1e-9f;
    for (int fr = 0; fr < frames; ++fr)
    {
        double acc = 0; int cnt = 0;
        for (int i = fr * fl; i < (fr + 1) * fl && i < (int) y.size(); ++i) { acc += std::abs (c[(size_t) i]); ++cnt; }
        e[(size_t) fr] = cnt ? (float) (acc / cnt) : 0.0f;
        mx = std::max (mx, e[(size_t) fr]);
    }
    for (auto& v : e) v = std::max (-80.0f, 20.0f * std::log10 (v / mx + 1e-6f));
    return e;
}

struct Fitter
{
    const Reference& ref;
    SoundType type;
    FeatureSetup fs;
    Features target;
    LossWeights w;
    float bassHz = 55, targetPitch = 0;
    int evals = 0, budget = 6000;
    std::atomic<bool>* cancel = nullptr;
    std::function<void (float, float)> progress;
    float bestLoss = 1e9f;
    SynthParams bestParams; float bestGate = 0.1f;

    Fitter (const Reference& r, SoundType t) : ref (r), type (t)
    {
        fs.init (r.sr, (int) r.x.size());
        if (t == SoundBass)
        {
            // Bass: ganze Wellenform vergleichen (Phasen-Reset -> jede Note hat dieselbe Form; erfasst auch die Tonhöhe genau)
            fs.headLen = std::min ((int) r.x.size(), (int) (r.sr * 0.3));
            w.pitch = 0.0f; w.spec = 0.8f; w.head = 1.0f;
        }
        target = computeFeatures (r.x.data(), (int) r.x.size(), fs, true);
    }

    bool cancelled() const { return cancel != nullptr && cancel->load(); }
    std::function<bool()> stopFn() { return [this] { return cancelled() || evals >= budget; }; }

    std::vector<float> render (const SynthParams& p, float gate) const
    {
        if (type == SoundKick)
        {
            SynthParams q = p; q.kSync = 0; q.kSnap = SnapOff; q.kFollow = 0;
            return renderKick (q, ref.sr, 145.0, 1.0f, 0.0f).audio;
        }
        return renderBassHz (p, ref.sr, bassHz * std::pow (2.0f, p.bFine / 1200.0f), gate,
                             (float) ref.x.size() / (float) ref.sr + 0.005f).audio;
    }

    LossParts loss (const SynthParams& p, float gate)
    {
        ++evals;
        const Reference y = makeReference (render (p, gate), ref.sr);   // gleiche Aufbereitung wie das Original
        const Features c = computeFeatures (y.x.data(), (int) y.x.size(), fs, false);
        LossParts L = compareFeatures (target, c, w);
        if (type == SoundKick)
        {
            // Tonhöhe phasenunabhängig über die Nulldurchgänge des Originals bewerten
            L.total -= w.pitch * L.pitch;
            L.pitch = kickPitchCurveError (p, target, ref.sr);
            L.total += w.pitch * L.pitch;
        }
        if (L.total < bestLoss) { bestLoss = L.total; bestParams = p; bestGate = gate; }
        if (progress && (evals % 25) == 0)
            progress (std::min (0.99f, (float) evals / (float) std::max (1, budget)), matchPercent (bestLoss));
        return L;
    }

    Objective objective (int part)
    {
        return [this, part] (const SynthParams& p, float gate)
        {
            const LossParts L = loss (p, gate);
            switch (part)
            {
                case 1:  return w.pitch * L.pitch + 0.3f * L.head + 0.1f * L.env;
                case 2:  return L.env + 0.2f * L.spec;
                default: return L.total;
            }
        };
    }

    float renderSearch (SynthParams& p, float& gate, const std::vector<Var>& vars, int part, float step0, float stepMin, int maxEvals)
    {
        return patternSearch (p, gate, vars, objective (part), step0, stepMin, maxEvals, stopFn(), (unsigned) evals);
    }

    float renderCma (SynthParams& p, float& gate, const std::vector<Var>& vars, float sigma, int maxEvals)
    {
        const int start = evals;
        return cmaSearch (p, gate, vars, objective (0), sigma, maxEvals,
                          [this, start, maxEvals] { return cancelled() || evals >= budget || evals - start >= maxEvals; }, 4242u + (unsigned) evals);
    }

    // diskrete Auswahl (Drive-Typ usw.) durchprobieren
    void tryChoices (SynthParams& p, float gate, float SynthParams::* field, int count)
    {
        float best = loss (p, gate).total; float bestV = p.*field;
        for (int i = 0; i < count; ++i)
        {
            if (i == (int) std::lround (bestV)) continue;
            SynthParams q = p; q.*field = (float) i;
            const float L = loss (q, gate).total;
            if (L < best) { best = L; bestV = (float) i; }
        }
        p.*field = bestV;
    }

    // Neustarts mit zufälliger Störung, solange Budget übrig ist
    void polish (SynthParams& p, float& gate, const std::vector<Var>& vars)
    {
        std::mt19937 rng (777u);
        std::normal_distribution<float> nd (0.0f, 1.0f);
        float best = loss (p, gate).total;
        int round = 0;
        while (! cancelled() && evals < budget - 50)
        {
            SynthParams q = p; float g = gate;
            const float amp = (round % 3 == 0) ? 0.08f : 0.03f;
            for (auto& v : vars) setNorm (v, q, g, getNorm (v, q, g) + nd (rng) * amp);
            const float L = patternSearch (q, g, vars, objective (0), 0.03f, 0.0015f, std::max (50, (budget - evals) / 3), stopFn(), 99u + (unsigned) round);
            if (L < best) { best = L; p = q; gate = g; }
            ++round;
        }
    }
};
} // namespace detail

//==============================================================================
inline SoundType detectType (const Reference& r)
{
    // Grundton im hinteren Teil, dann stark tiefpassgefilterte Nulldurchgänge: Kick = deutlicher Pitch-Fall, Bass = konstant
    const int n = (int) r.x.size();
    const float fl = yinPitch (r.x, r.sr, n / 3, n);
    if (fl <= 0.0f) return SoundKick;
    Svf a, b; a.set (fl * 2.5f, 0.6f, (float) r.sr); b.set (fl * 2.5f, 0.6f, (float) r.sr);
    std::vector<double> cr;
    float prev = 0;
    for (int i = 0; i < n; ++i)
    {
        const float y = b.lp (a.lp (r.x[(size_t) i]));
        if (prev < 0 && y >= 0) cr.push_back ((double) i / r.sr);
        prev = y;
    }
    // Frequenz zwischen 3 und 15 ms
    std::vector<float> early;
    for (size_t i = 0; i + 1 < cr.size(); ++i)
        if (cr[i] > 0.003 && cr[i] < 0.015) early.push_back ((float) (1.0 / (cr[i + 1] - cr[i])));
    const float fe = medianOf (early);
    const float ratio = fe > 0 ? fe / fl : 1.0f;
    if (ratio > 1.7f) return SoundKick;
    if (ratio < 1.25f) return SoundBass;
    // Grenzfall: Sinus-artig (wenig Obertöne) spricht für Kick
    return harmonicRatio (r.x, r.sr, n / 3, std::max (256, n / 3), fl) < 0.1f ? SoundKick : SoundBass;
}

//==============================================================================
inline void guessKick (const Reference& r, SynthParams& p, AnalysisResult& res)
{
    FeatureSetup fs; fs.init (r.sr, (int) r.x.size());
    const Features f = computeFeatures (r.x.data(), (int) r.x.size(), fs, true);
    const auto env = detail::linearEnv (r);
    const int frames = (int) env.size();
    const double lenMs = (double) r.x.size() * 1000.0 / r.sr;

    p.kSync = 0; p.kSnap = SnapOff; p.kFollow = 0;
    p.kLength = (float) std::clamp (lenMs, 30.0, 1500.0);
    p.kFade = (float) std::clamp (lenMs * 0.12, 2.0, 60.0);

    int lastLoud = 0;
    for (int i = 0; i < f.frames; ++i) if (f.env[(size_t) i] > -30.0f) lastLoud = i;

    // Grundton: Median im hinteren Teil
    std::vector<float> tail;
    for (int i = std::max (10, (int) (lastLoud * 0.45)); i <= (int) (lastLoud * 0.9); ++i)
        if (f.pitch[(size_t) i] > 0) tail.push_back (f.pitch[(size_t) i]);
    float endHz = medianOf (tail);
    if (endHz <= 0) endHz = 50;
    // Drift: Unterschied Mitte zu Ende
    std::vector<float> mid, last;
    for (int i = std::max (10, (int) (lastLoud * 0.3)); i <= (int) (lastLoud * 0.45); ++i) if (f.pitch[(size_t) i] > 0) mid.push_back (f.pitch[(size_t) i]);
    for (int i = (int) (lastLoud * 0.8); i <= lastLoud; ++i) if (f.pitch[(size_t) i] > 0) last.push_back (f.pitch[(size_t) i]);
    const float fm = medianOf (mid), fla = medianOf (last);
    float drift = 0.5f;
    if (fm > 0 && fla > 0) drift = std::clamp (12.0f * std::log2 (fm / fla) * 1.6f, 0.0f, 8.0f);
    p.kEndHz = std::clamp (endHz * std::pow (2.0f, drift * 0.35f / 12.0f), 20.0f, 200.0f);
    p.kDrift = drift;

    // Punch: Tonhöhe bei ~4 ms, Start: Maximum in den ersten 1,5 ms
    auto pitchAt = [&] (int ms) { for (int i = ms; i < std::min (f.frames, ms + 3); ++i) if (f.pitch[(size_t) i] > 0) return f.pitch[(size_t) i]; return 0.0f; };
    float punch = pitchAt (3);
    if (punch <= endHz * 1.2f) punch = endHz * 4.0f;
    p.kPunchHz = std::clamp (punch, 60.0f, 1200.0f);
    float start = 0; for (int i = 0; i < 2 && i < f.frames; ++i) start = std::max (start, f.pitch[(size_t) i]);
    p.kStartHz = std::clamp (std::max (start, punch * 3.0f), 200.0f, 12000.0f);
    // Sweep-Zeit: wann ist (f - end) auf 37 % von (punch - end) gefallen?
    float sweep = 18;
    for (int i = 3; i < f.frames; ++i)
    {
        const float fr = f.pitch[(size_t) i];
        if (fr > 0 && fr - endHz <= 0.37f * (punch - endHz)) { sweep = (float) (i - 3) + 1.5f; break; }
    }
    p.kSweepTime = std::clamp (sweep * 1.3f, 2.0f, 120.0f);
    p.kClickTime = 1.0f;

    // Hüllkurve
    float pk = 0; int pkAt = 0;
    for (int i = 0; i < frames; ++i) if (env[(size_t) i] > pk) { pk = env[(size_t) i]; pkAt = i; }
    p.kAttack = (float) std::clamp (pkAt * 0.3, 0.0, 5.0);
    int hold = pkAt;
    while (hold < frames - 1 && env[(size_t) hold] > pk * 0.89f) ++hold;      // -1 dB
    p.kHold = (float) std::clamp (hold - pkAt, 0, 60);
    int dec = hold;
    while (dec < frames - 1 && env[(size_t) dec] > pk * 0.1f) ++dec;          // -20 dB
    p.kDecay = (float) std::clamp ((dec - hold) * 1.0, 10.0, 800.0);

    // Fishtail: Einschnürung zwischen 3 und 40 ms, danach wieder lauter
    p.kFishDepth = 0.0f;
    float bestDip = 0;
    for (int i = 3; i < std::min (frames - 5, 40); ++i)
    {
        float after = 0;
        for (int j = i + 1; j < std::min (frames, i + 30); ++j) after = std::max (after, env[(size_t) j]);
        const float before = env[(size_t) std::max (0, i - 3)];
        const float ref = std::min (before, after);
        if (env[(size_t) i] < ref * 0.92f && ref - env[(size_t) i] > bestDip)
        {
            bestDip = ref - env[(size_t) i];
            p.kFishDepth = std::clamp (1.0f - env[(size_t) i] / ref, 0.0f, 0.9f);
            p.kFishPos = (float) i;
        }
    }
    if (p.kFishDepth > 0.0f) p.kFishWidth = std::clamp (p.kFishPos * 0.6f, 2.0f, 30.0f);
    else { p.kFishDepth = 0.15f; p.kFishPos = 12; p.kFishWidth = 8; }

    // Click: Hochton-Energie in den ersten 3 ms
    {
        const int n = std::min ((int) r.x.size(), (int) (r.sr * 0.003));
        Svf hp; hp.set (2500.0f, 0.7f, (float) r.sr);
        double eh = 0, ea = 0;
        for (int i = 0; i < n; ++i) { float l, b, h; hp.process (r.x[(size_t) i], l, b, h); eh += h * h; ea += r.x[(size_t) i] * r.x[(size_t) i]; }
        const float ratio = ea > 0 ? (float) (eh / ea) : 0.0f;
        p.kClickLevel = std::clamp (std::sqrt (ratio) * 1.6f, 0.0f, 1.2f);
        p.kClickHz = 4500; p.kClickDecay = 1.5f; p.kClickTone = 0.3f;
    }

    // Sättigung: Anteil der Samples nahe der Spitze im Körper (Sinus ~0,4, Rechteck ~1)
    {
        const int a = (int) (r.sr * 0.006), b = std::max (a + 10, (int) (r.x.size() / 2));
        int cnt = 0, hi = 0; float lp = 0;
        for (int i = a; i < b && i < (int) r.x.size(); ++i) lp = std::max (lp, std::abs (r.x[(size_t) i]));
        for (int i = a; i < b && i < (int) r.x.size(); ++i)
        {
            const float v = std::abs (r.x[(size_t) i]);
            if (v > lp * 0.15f) { ++cnt; if (v > lp * 0.8f) ++hi; }
        }
        const float frac = cnt ? (float) hi / (float) cnt : 0.4f;
        p.kDrive = std::clamp ((frac - 0.42f) / 0.45f, 0.0f, 1.0f);
        p.kTransient = 0.5f;
    }
    p.kShape = 0; p.kKnock = 0.15f; p.kStartPhase = 0;
    p.kLowCut = 15; p.kHoleDb = 0; p.kHighCut = 20000; p.kVelSens = 0.3f;

    res.kickEnd = endHz;
}

inline void guessBass (const Reference& r, SynthParams& p, AnalysisResult& res)
{
    const int n = (int) r.x.size();
    const float f0 = estimateBassPitch (r.x, r.sr);
    res.bassHz = f0 > 20.0f ? f0 : 55.0f;
    const double midi = hzToMidi (res.bassHz, p.a4);
    res.detectedNote = (int) std::lround (midi);
    res.detectedCents = (float) ((midi - res.detectedNote) * 100.0);

    const auto env = detail::linearEnv (r);
    const int frames = (int) env.size();
    float pk = 0; int pkAt = 0;
    for (int i = 0; i < frames; ++i) if (env[(size_t) i] > pk) { pk = env[(size_t) i]; pkAt = i; }
    p.bAttack = (float) std::clamp (pkAt * 0.5, 0.0, 30.0);
    int dec = pkAt;
    while (dec < frames - 1 && env[(size_t) dec] > pk * 0.5f) ++dec;
    // Sustain: Pegel bei 70 % der Länge
    const float sus = env[(size_t) std::clamp ((int) (frames * 0.7), 0, frames - 1)] / std::max (1e-6f, pk);
    p.bSustain = std::clamp (sus, 0.0f, 1.0f);
    p.bDecay = (float) std::clamp ((dec - pkAt) * 2.5, 5.0, 600.0);
    // Note-Ende: wo die Hüllkurve deutlich steiler abfällt (Release schneller als Decay)
    {
        FeatureSetup fs; fs.init (r.sr, n);
        const Features f = computeFeatures (r.x.data(), n, fs, false);
        const int F = f.frames;
        int bestI = (int) (F * 0.6); float bestD = 0.0f;
        for (int i = std::max (pkAt + 5, 8); i < F - 6; ++i)
        {
            if (f.env[(size_t) i] < -45.0f) break;
            const float before = (f.env[(size_t) i] - f.env[(size_t) (i - 5)]) / 5.0f;    // dB/ms
            const float after  = (f.env[(size_t) (i + 5)] - f.env[(size_t) i]) / 5.0f;
            const float d = before - after;                                             // > 0: wird steiler
            if (d > bestD) { bestD = d; bestI = i; }
        }
        res.gateSec = (float) std::clamp ((bestI - 1) / 1000.0, 0.01, 2.0);
        // Release: Zeit von dort bis -20 dB darunter
        int rel = bestI;
        while (rel < F - 1 && f.env[(size_t) rel] > f.env[(size_t) bestI] - 20.0f) ++rel;
        p.bRelease = (float) std::clamp ((double) (rel - bestI), 2.0, 200.0);
        // Sustain/Decay neu schätzen: Pegel kurz vor dem Note-Ende
        const float atGate = std::pow (10.0f, f.env[(size_t) std::max (0, bestI - 1)] / 20.0f);
        p.bSustain = std::clamp (atGate * 0.5f, 0.0f, 1.0f);
    }

    // Wellenform aus Obertönen: Saw hat alle (1/n), Square nur ungerade
    const int from = (int) (r.sr * 0.01), len = std::min (n - from, (int) (r.sr * 0.06));
    if (len > 256)
    {
        const auto h = harmonicAmps (r.x, r.sr, from, len, res.bassHz, 8);
        const float even = h[1] + h[3] + h[5], odd = h[2] + h[4] + h[6];
        const float sq = odd > 0 ? std::clamp (1.0f - even / (odd + 1e-6f), 0.0f, 1.0f) : 0.0f;
        p.bSquare = sq; p.bSaw = 1.0f - sq * 0.8f;
        // Sub: Grundton deutlich stärker als bei Saw üblich (H1/H2 = 2)
        const float r12 = h[1] > 0 ? h[0] / h[1] : 4.0f;
        p.bSub = std::clamp ((r12 - 2.0f) * 0.15f, 0.0f, 1.0f);
        // Cutoff: wo die Obertöne gegenüber 1/n um ~12 dB eingebrochen sind
        float cut = res.bassHz * 8.0f;
        for (int k = 1; k < (int) h.size(); ++k)
            if (h[(size_t) k] * (float) (k + 1) < h[0] * 0.25f) { cut = res.bassHz * (float) (k + 1); break; }
        p.bCutoff = std::clamp (cut, 40.0f, 8000.0f);
    }
    p.bFenv = 2.5f; p.bFdecay = 40; p.bFattack = 0; p.bReso = 0.2f;
    p.bPluck = 0; p.bDrive = 0.1f; p.bDetune = 0; p.bStartPhase = 0.5f; p.bRetrig = 1;
    p.bLowCut = 15; p.bHighCut = 20000; p.bNotchDb = 0; p.bOctave = 2; p.bFine = 0;
    p.bVelSens = 0.4f; p.bKeytrack = 0.0f;
}

//==============================================================================
inline AnalysisResult analyzeAndFit (const Reference& ref, const SynthParams& base, const AnalysisJob& job)
{
    AnalysisResult res;
    res.params = base;
    if (ref.x.size() < (size_t) (ref.sr * 0.01)) { res.summary = "Datei zu kurz oder still."; return res; }

    res.type = job.forceType == SoundAuto ? detectType (ref) : (SoundType) job.forceType;
    SynthParams p = base;
    if (res.type == SoundKick) guessKick (ref, p, res); else guessBass (ref, p, res);

    detail::Fitter F (ref, res.type);
    F.cancel = job.cancel; F.progress = job.progress; F.budget = job.budget;
    F.bassHz = res.bassHz;
    float gate = res.gateSec;
    res.initialMatch = matchPercent (F.loss (p, gate).total);
    auto noStop = [] { return false; };

    if (job.fine)
    {
        const int B = job.budget;
        if (res.type == SoundKick)
        {
            const auto pitchVars = detail::vars ({ "k_start", "k_punch", "k_end", "k_clickt", "k_sweep", "k_drift" });
            const auto ampVars   = detail::vars ({ "k_attack", "k_hold", "k_decay", "k_length", "k_fade", "k_fdepth", "k_fpos", "k_fwidth" });
            const auto toneVars  = detail::vars ({ "k_shape", "k_knock", "k_clvl", "k_clhz", "k_cldec", "k_cltone", "k_drive", "k_trans",
                                                   "k_phase", "k_holedb", "k_holehz", "k_holeq", "k_lowcut", "k_highcut" });

            // 1) Tonhöhenverlauf direkt an die Nulldurchgänge anpassen (schnell, mehrere Startpunkte)
            const detail::Objective pitchObj = [&] (const SynthParams& q, float) { return detail::kickPitchCurveError (q, F.target, ref.sr); };
            struct Cand { SynthParams p; float L; };
            std::vector<Cand> cands;
            for (float sw : { 4.0f, 8.0f, 16.0f, 32.0f, 64.0f })
                for (float pm : { 2.5f, 4.0f, 7.0f })
                    for (float ct : { 0.3f, 1.0f, 3.0f })
                    {
                        SynthParams q = p;
                        q.kSweepTime = sw; q.kPunchHz = std::clamp (p.kEndHz * pm, 60.0f, 1200.0f); q.kClickTime = ct;
                        q.kStartHz = std::clamp (q.kPunchHz * 8.0f, 200.0f, 12000.0f);
                        cands.push_back ({ q, pitchObj (q, gate) });
                    }
            cands.push_back ({ p, pitchObj (p, gate) });
            std::sort (cands.begin(), cands.end(), [] (const Cand& a, const Cand& b) { return a.L < b.L; });
            float bestPitch = 1e9f;
            for (size_t i = 0; i < std::min<size_t> (5, cands.size()); ++i)
            {
                SynthParams q = cands[i].p; float g = gate;
                const float L = detail::patternSearch (q, g, pitchVars, pitchObj, 0.1f, 0.0005f, 1500, noStop, 11u + (unsigned) i);
                if (L < bestPitch) { bestPitch = L; p = q; }
            }

            if (job.stageHook) job.stageHook ("pitch", p, gate);
            // 2) Lautstärkeverlauf (Modell) an die Hüllkurve anpassen
            const detail::Objective envObj = [&] (const SynthParams& q, float) { return detail::kickEnvCurveError (q, F.target); };
            {
                float bestEnv = 1e9f; SynthParams bestP = p;
                for (float fd : { 0.0f, p.kFishDepth, 0.5f })
                {
                    SynthParams q = p; float g = gate; q.kFishDepth = fd;
                    auto ampDrive = ampVars; ampDrive.push_back (detail::vars ({ "k_drive" })[0]);
                    const float L = detail::patternSearch (q, g, ampDrive, envObj, 0.1f, 0.0005f, 2500, noStop, 21u);
                    if (L < bestEnv) { bestEnv = L; bestP = q; }
                }
                p = bestP;
            }

            if (job.stageHook) job.stageHook ("amp", p, gate);
            // 3) mit echtem Rendern: Klangfarbe, dann alles zusammen, dann Neustarts
            std::vector<detail::Var> all = pitchVars;
            all.insert (all.end(), ampVars.begin(), ampVars.end());
            all.insert (all.end(), toneVars.begin(), toneVars.end());
            F.renderSearch (p, gate, toneVars, 0, 0.08f, 0.004f, B / 8);
            F.tryChoices (p, gate, &SynthParams::kDriveType, 4);
            F.renderCma (p, gate, all, 0.08f, (int) (B * 0.55f));
            F.tryChoices (p, gate, &SynthParams::kDriveType, 4);
            F.renderSearch (p, gate, all, 0, 0.02f, 0.0007f, B);
        }
        else
        {
            const auto envVars  = detail::vars ({ "b_attack", "b_decay", "b_sustain", "b_release", "gate" });
            const auto toneVars = detail::vars ({ "b_saw", "b_square", "b_pw", "b_sub", "b_cutoff", "b_reso", "b_fenv", "b_fatt", "b_fdec",
                                                  "b_drive", "b_pluck", "b_pldec", "b_detune", "b_phase", "b_lowcut", "b_highcut" });
            // Tonhöhe: auf die nächste Note in der eingestellten Stimmung (A4) setzen. Die Messung ist durch den
            // Filter-Sweep um bis zu ~30 ct verschoben, Psy-Bässe sind aber praktisch immer auf eine Note gestimmt.
            const float measuredHz = estimateBassPitch (ref.x, ref.sr);
            int note = (int) std::lround (hzToMidi (measuredHz > 0 ? measuredHz : 55.0f, p.a4));
            F.bassHz = noteHz (note, p.a4);
            res.measuredHz = measuredHz;
            if (job.stageHook) job.stageHook ("guess", p, gate);
            // Startpunkt: eigene Schätzung oder ein Werks-Bass (mit Note-Länge/Tonhöhe des Originals), je nachdem was näher dran ist
            {
                float bestL = F.loss (p, gate).total;
                SynthParams bestP = p;
                for (auto& fp : factoryPresets())
                {
                    SynthParams q = applyPreset (fp, p);
                    for (auto& d : paramTable())
                        if (std::strncmp (d.id, "b_", 2) != 0) q.*(d.field) = p.*(d.field);
                    q.bFine = p.bFine; q.bOctave = 2; q.bKeytrack = 0.0f; q.bLevel = p.bLevel;
                    q.bLowCut = 15; q.bNotchDb = 0;
                    const float L = F.loss (q, gate).total;
                    if (L < bestL) { bestL = L; bestP = q; }
                }
                p = bestP;
            }
            F.renderSearch (p, gate, envVars, 2, 0.1f, 0.002f, B / 10);
            if (job.stageHook) job.stageHook ("env", p, gate);

            std::vector<detail::Var> all = envVars;
            all.insert (all.end(), toneVars.begin(), toneVars.end());
            // erst die wichtigsten Regler, dann alle
            const auto core = detail::vars ({ "b_sub", "b_cutoff", "b_fenv", "b_fdec", "b_decay", "b_sustain", "b_release", "gate", "b_attack", "b_drive", "b_reso" });
            F.renderCma (p, gate, core, 0.15f, (int) (B * 0.3f));
            F.tryChoices (p, gate, &SynthParams::bSlope, 2);
            F.tryChoices (p, gate, &SynthParams::bSubOct, 2);
            F.tryChoices (p, gate, &SynthParams::bSubInv, 2);
            F.tryChoices (p, gate, &SynthParams::bDriveType, 4);
            // Nachbarnoten prüfen (falls die Messung daneben lag)
            {
                float bestL = F.loss (p, gate).total; int bestNote = note;
                for (int dn : { -1, 1 })
                {
                    const float keep = F.bassHz;
                    F.bassHz = noteHz (note + dn, p.a4);
                    const float L = F.loss (p, gate).total;
                    if (L < bestL - 0.3f) { bestL = L; bestNote = note + dn; }
                    F.bassHz = keep;
                }
                note = bestNote; F.bassHz = noteHz (note, p.a4);
                F.bestLoss = 1e9f;   // Bestwert neu sammeln (Tonhöhe könnte sich geändert haben)
            }
            F.renderCma (p, gate, core, 0.06f, (int) (B * 0.15f));
            F.renderCma (p, gate, all, 0.05f, (int) (B * 0.3f));
            F.renderSearch (p, gate, all, 0, 0.02f, 0.0007f, B);
        }
        // bestes je gefundenes Ergebnis verwenden
        if (F.bestLoss < F.loss (p, gate).total) { p = F.bestParams; gate = F.bestGate; }

        // 4) Phase ausrichten, damit auch die Wellenform deckungsgleich ist
        if (! F.cancelled())
        {
            float SynthParams::* ph = res.type == SoundKick ? &SynthParams::kStartPhase : &SynthParams::bStartPhase;
            const detail::Objective phaseObj = [&] (const SynthParams& q, float g) { const auto L = F.loss (q, g); return L.total + 3.0f * L.head; };
            float best = phaseObj (p, gate), bestV = p.*ph;
            for (int i = 0; i < 32; ++i)
            {
                SynthParams q = p; q.*ph = (float) i / 32.0f;
                const float L = phaseObj (q, gate);
                if (L < best) { best = L; bestV = q.*ph; }
            }
            p.*ph = bestV;
            const auto phv = res.type == SoundKick ? detail::vars ({ "k_phase", "k_start", "k_clickt", "k_punch" })
                                                   : detail::vars ({ "b_phase", "b_fine" });
            detail::patternSearch (p, gate, phv, phaseObj, 0.02f, 0.0005f, 400, noStop, 55u);
        }
    }

    res.gateSec = gate;
    res.match = matchPercent (F.loss (p, gate).total);
    res.evaluations = F.evals;

    // Ergebnis für das Plugin aufbereiten
    char buf[512];
    if (res.type == SoundKick)
    {
        SynthParams q = p; q.kSnap = SnapOff; q.kSync = 0; q.kFollow = 0;
        const KickShape k = makeKickShape (q, 145.0);
        const double midi = hzToMidi (k.endHz, p.a4);
        const int nn = (int) std::lround (midi);
        std::snprintf (buf, sizeof buf, "Kick · Grundton %.1f Hz (≈ %s%d %+d ct) · Punch %.0f Hz · Länge %.0f ms · Drive %.0f %%",
                       k.endHz, noteName (nn), noteOctave (nn), (int) std::lround ((midi - nn) * 100.0),
                       p.kPunchHz, p.kLength, p.kDrive * 100.0f);
        p = q;
    }
    else
    {
        // Bass spielt im Plugin MIDI-Noten -> Feinstimmung nur bei echter Verstimmung übernehmen
        res.bassHz = F.bassHz;
        p.bFine = 0.0f;
        {
            const double midi = hzToMidi (res.bassHz, p.a4);
            res.detectedNote = (int) std::lround (midi);
            res.detectedCents = (float) ((midi - res.detectedNote) * 100.0);
        }
        const float cents = res.detectedCents;
        std::snprintf (buf, sizeof buf, "Bass · Note %s%d (%.1f Hz, gemessen ~%.1f Hz) · Cutoff %.0f Hz · Decay %.0f ms · Note-Länge %.0f ms",
                       noteName (res.detectedNote), noteOctave (res.detectedNote), res.bassHz, res.measuredHz,
                       p.bCutoff, p.bDecay, res.gateSec * 1000.0f);
        (void) cents;
        p.bFine = std::abs (cents) < 8.0f ? 0.0f : std::clamp (cents, -100.0f, 100.0f);
        p.bOctave = 2;
        p.bKeytrack = 0.0f;
    }
    res.summary = buf;
    res.params = p;
    res.ok = true;
    if (job.progress) job.progress (1.0f, res.match);
    return res;
}
//==============================================================================
// Analyse für Snare, Clap, Hats, Ride, Perc, Zap, Riser: Hüllkurve + Spektrum (+ Tonhöhe bei tonalen Klängen)
namespace detail
{
inline float decayToDb (const std::vector<float>& envDb, float db, int from = 0)
{
    int pk = from; float mx = -200.0f;
    for (int i = from; i < (int) envDb.size(); ++i) if (envDb[(size_t) i] > mx) { mx = envDb[(size_t) i]; pk = i; }
    int i = pk;
    while (i < (int) envDb.size() - 1 && envDb[(size_t) i] > mx + db) ++i;
    return (float) (i - pk);   // ms
}

inline float spectralCentroid (const std::vector<float>& x, double sr, int from, int len)
{
    int N = 1; while (N < len) N <<= 1;
    N = std::max (N, 2048);
    std::vector<std::complex<float>> buf ((size_t) N, { 0.0f, 0.0f });
    for (int i = 0; i < len && from + i < (int) x.size(); ++i)
    {
        const float w = 0.5f - 0.5f * std::cos (kTwoPi * (float) i / (float) len);
        buf[(size_t) i] = { x[(size_t) (from + i)] * w, 0.0f };
    }
    fft (buf);
    double num = 0, den = 0;
    for (int k = 1; k < N / 2; ++k) { const double e = std::norm (buf[(size_t) k]); num += e * k * sr / N; den += e; }
    return den > 0 ? (float) (num / den) : 1000.0f;
}

inline void setIf (SynthParams& p, const char* id, float v)
{
    if (auto* d = findParam (id)) p.*(d->field) = std::clamp (v, d->min, d->max);
}

// Grobe Schätzung aus Messwerten
inline void guessDrum (const Reference& r, int inst, SynthParams& p)
{
    FeatureSetup fs; fs.init (r.sr, (int) r.x.size());
    const Features f = computeFeatures (r.x.data(), (int) r.x.size(), fs, true);
    const float dec = decayToDb (f.env, -20.0f);
    const float lenMs = (float) (r.x.size() * 1000.0 / r.sr);
    const int n = (int) r.x.size();
    const float cent = spectralCentroid (r.x, r.sr, 0, std::min (n, (int) (r.sr * 0.08)));
    switch (inst)
    {
        case InSnare:
            setIf (p, "s_ndec", dec * 1.1f); setIf (p, "s_bdec", dec * 0.5f);
            setIf (p, "s_nbp", cent * 1.2f);
            {
                const float f0 = yinPitch (r.x, r.sr, (int) (r.sr * 0.004), std::min (n, (int) (r.sr * 0.05)));
                if (f0 > 90 && f0 < 500) setIf (p, "s_tone1", f0);
            }
            break;
        case InClap:
            setIf (p, "c_tdec", dec); setIf (p, "c_bp", cent * 0.6f);
            break;
        case InCHat: setIf (p, "h_dec", dec); setIf (p, "h_bp", cent); setIf (p, "h_hp", cent * 0.7f); break;
        case InOHat: setIf (p, "o_dec", dec); setIf (p, "o_bp", cent); setIf (p, "o_hp", cent * 0.7f); break;
        case InRide: setIf (p, "r_dec", dec); setIf (p, "r_bp", cent); setIf (p, "r_hp", cent * 0.6f); break;
        case InPerc:
        {
            setIf (p, "p_dec", dec);
            const float f0 = yinPitch (r.x, r.sr, std::min (n - 1, (int) (r.sr * 0.01)), n);
            if (f0 > 30) setIf (p, "p_tune", f0);
            setIf (p, "p_snap", 0);
            break;
        }
        case InZap:
        {
            setIf (p, "z_len", lenMs);
            std::vector<float> early, late;
            for (int i = 1; i < std::min (f.frames, 6); ++i) if (f.pitch[(size_t) i] > 0) early.push_back (f.pitch[(size_t) i]);
            for (int i = (int) (lenMs * 0.6f); i < (int) (lenMs * 0.9f) && i < f.frames; ++i) if (f.pitch[(size_t) i] > 0) late.push_back (f.pitch[(size_t) i]);
            if (! early.empty()) setIf (p, "z_start", medianOf (early) * 1.3f);
            if (! late.empty())  setIf (p, "z_end", medianOf (late) * 0.9f);
            setIf (p, "z_sweep", lenMs * 0.6f);
            break;
        }
        case InRiser:
        {
            int pk = 0; float mx = -200;
            for (int i = 0; i < f.frames; ++i) if (f.env[(size_t) i] > mx) { mx = f.env[(size_t) i]; pk = i; }
            setIf (p, "x_len", (float) pk / 1000.0f * 145.0f / 60.0f);
            setIf (p, "x_rel", std::max (10.0f, lenMs - (float) pk));
            break;
        }
        default: break;
    }
}
} // namespace detail

inline AnalysisResult analyzeDrumAndFit (const Reference& ref, const SynthParams& base, int inst, const AnalysisJob& job)
{
    AnalysisResult res;
    res.params = base; res.inst = inst; res.type = SoundKick;
    if (ref.x.size() < (size_t) (ref.sr * 0.005)) { res.summary = "Datei zu kurz oder still."; return res; }

    const bool tonal = inst == InPerc || inst == InZap;
    FeatureSetup fs; fs.init (ref.sr, (int) ref.x.size());
    const Features target = computeFeatures (ref.x.data(), (int) ref.x.size(), fs, tonal);
    LossWeights w; w.env = 1.0f; w.spec = 1.0f; w.pitch = tonal ? 2.0f : 0.0f; w.head = tonal ? 0.3f : 0.0f;
    const float maxSec = (float) (ref.x.size() / ref.sr) * 1.5f + 0.5f;

    int evals = 0;
    float bestLoss = 1e9f;
    const auto t0 = std::chrono::steady_clock::now();
    auto elapsed = [&] { return std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count(); };
    auto cancelled = [&] { return (job.cancel != nullptr && job.cancel->load()) || elapsed() > job.maxSeconds; };
    auto loss = [&] (const SynthParams& q) -> float
    {
        ++evals;
        SynthParams r = q; r.inst = (float) inst;
        const auto out = renderInstrument (r, inst, ref.sr, 145.0, 1.0f, -1.0f, maxSec);
        const Reference y = makeReference (out.audio, ref.sr);
        if (y.x.empty()) return 1e6f;
        const Features c = computeFeatures (y.x.data(), (int) y.x.size(), fs, tonal);
        const float L = compareFeatures (target, c, w).total;
        if (L < bestLoss) bestLoss = L;
        if (job.progress && evals % 20 == 0)
            job.progress (std::min (0.99f, std::max ((float) evals / (float) job.budget, (float) (elapsed() / job.maxSeconds))), matchPercent (bestLoss));
        return L;
    };
    const detail::Objective obj = [&] (const SynthParams& q, float) { return loss (q); };

    // Regler dieses Instruments (ohne Level/Velocity)
    std::vector<detail::Var> vars;
    for (auto& d : paramTable())
    {
        if (! belongsTo (d.id, inst) || d.kind != KFloat) continue;
        const std::string id = d.id;
        if (id.find ("_level") != std::string::npos || id.find ("_vel") != std::string::npos) continue;
        vars.push_back ({ d.field, &d });
    }
    std::vector<float SynthParams::*> choices;   // Auswahl-Regler (z. B. Drive-Typ) einzeln durchprobieren
    for (auto& d : paramTable())
        if (belongsTo (d.id, inst) && (d.kind == KChoice || d.kind == KBool) && std::string (d.id) != "p_snap" && std::string (d.id) != "s_klock")
            choices.push_back (d.field);
    // Tonhöhe exakt übernehmen: Einrasten auf die Tonart während der Analyse aus
    auto unlock = [] (SynthParams& q) { q.pSnap = 0; q.sKeyLock = 0; };

    // Startpunkte: eigene Schätzung + alle Werks-Presets dieses Instruments
    struct Cand { SynthParams p; float L; };
    std::vector<Cand> cands;
    {
        SynthParams g = applyPreset (FactoryPreset { "", "", {}, inst }, base);
        unlock (g);
        detail::guessDrum (ref, inst, g);
        cands.push_back ({ g, loss (g) });
        res.initialMatch = matchPercent (cands.back().L);
        for (auto& fp : factoryPresets())
            if (fp.inst == inst)
            {
                SynthParams q = applyPreset (fp, base);
                unlock (q);
                SynthParams q2 = q; detail::guessDrum (ref, inst, q2);
                cands.push_back ({ q, loss (q) });
                cands.push_back ({ q2, loss (q2) });
            }
    }
    std::sort (cands.begin(), cands.end(), [] (const Cand& a, const Cand& b) { return a.L < b.L; });
    res.initialMatch = std::max (res.initialMatch, matchPercent (cands.front().L));

    SynthParams best = cands.front().p; float bestL = cands.front().L;
    if (job.fine && ! vars.empty())
    {
        const int B = job.budget;
        auto stop = [&] { return cancelled() || evals >= B; };
        const int starts = std::min<int> (2, (int) cands.size());
        for (int s = 0; s < starts && ! stop(); ++s)
        {
            SynthParams q = cands[(size_t) s].p; float g = 0.0f;
            const int cap = evals + (int) (B * 0.35f);
            const float L = detail::cmaSearch (q, g, vars, obj, 0.12f, cap - evals, [&] { return stop() || evals >= cap; }, 900u + (unsigned) s);
            if (L < bestL) { bestL = L; best = q; }
        }
        for (auto f : choices)
        {
            if (stop()) break;
            const float keep = best.*f;
            for (int v = 0; v < 4; ++v)
            {
                SynthParams q = best; q.*f = (float) v;
                bool valid = false;
                for (auto& d : paramTable()) if (d.field == f && (float) v <= d.max) valid = true;
                if (! valid || v == (int) std::lround (keep)) continue;
                const float L = loss (q);
                if (L < bestL) { bestL = L; best = q; }
            }
        }
        float g = 0.0f;
        const float L = detail::patternSearch (best, g, vars, obj, 0.03f, 0.001f, std::max (50, B - evals), stop, 77u);
        bestL = std::min (bestL, L);
    }

    res.params = best;
    res.match = matchPercent (bestL);
    res.evaluations = evals;
    char buf[256];
    std::snprintf (buf, sizeof buf, "%s · Länge %.0f ms · %d Varianten getestet", instrumentName (inst), ref.x.size() * 1000.0 / ref.sr, evals);
    res.summary = buf;
    res.ok = true;
    if (job.progress) job.progress (1.0f, res.match);
    return res;
}

// Einstieg: passendes Verfahren für das Instrument der Instanz
inline AnalysisResult analyzeInstrument (const Reference& ref, const SynthParams& base, int inst, AnalysisJob job)
{
    if (inst == InKick || inst == InBass)
    {
        job.forceType = inst == InKick ? SoundKick : SoundBass;
        auto r = analyzeAndFit (ref, base, job);
        r.inst = inst;
        return r;
    }
    return analyzeDrumAndFit (ref, base, inst, job);
}
} // namespace pf
