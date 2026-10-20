#pragma once
// PsyForge – Stimmen für Snare, Clap, Hats/Ride (Metall), Perc, Zap, Riser (ohne JUCE)
// Alle Stimmen sind deterministisch (Rauschen startet bei jedem Anschlag mit demselben Seed),
// damit Anzeige, Export und Analyse immer exakt gleich klingen.
#include "Engine.h"

namespace pf
{
enum Instrument { InKick = 0, InBass, InSnare, InClap, InCHat, InOHat, InRide, InPerc, InZap, InRiser, NumInstruments };

inline const char* instrumentName (int i)
{
    static const char* n[] = { "Kick", "Bass", "Snare", "Clap", "Closed Hat", "Open Hat", "Ride", "Perc", "Zap", "Riser" };
    return n[std::clamp (i, 0, NumInstruments - 1)];
}
// ID-Präfix der Klangregler je Instrument
inline const char* instrumentPrefix (int i)
{
    static const char* n[] = { "k_", "b_", "s_", "c_", "h_", "o_", "r_", "p_", "z_", "x_" };
    return n[std::clamp (i, 0, NumInstruments - 1)];
}

inline float velGain (float vel, float sens) { return 1.0f - sens + sens * std::clamp (vel, 0.0f, 1.0f); }
inline float msTau (float ms) { return std::max (0.00005f, ms * 0.001f / 2.302585f); }   // Zeit bis -20 dB -> Zeitkonstante

// Gemeinsame Basis: Retrigger ohne Knacksen
struct DrumBase
{
    float sr = 44100.0f, t = 0.0f, vel = 1.0f;
    bool active = false;
    float lastOut = 0.0f, fade = 0.0f, lastAmp = 0.0f; int fadeLeft = 0, fadeLen = 64;
    void prepareBase (float s) { sr = s; active = false; t = 0; fadeLen = std::max (16, (int) (sr * 0.0015f)); }
    void beginTrigger (float v) { fade = lastOut; fadeLeft = fadeLen; active = true; t = 0.0f; vel = v; }
    inline float declick() noexcept { return fadeLeft > 0 ? fade * (float) fadeLeft-- / (float) fadeLen : 0.0f; }
};

//==============================================================================
struct SnareVoice : DrumBase
{
    double ph1 = 0, ph2 = 0; Noise noise { 0x51u }; Svf hp, bp, snapHp; Biquad lowCut, highCut;
    float f1 = 190, f2 = 330, maxLen = 1.0f;
    void prepare (float s) { prepareBase (s); }
    void update (const SynthParams& p)
    {
        f1 = p.sTone1; f2 = p.sTone2;
        if (p.sKeyLock > 0.5f) { f1 = snapToKey (f1, (int) p.key, p.a4, SnapScale); f2 = snapToKey (f2, (int) p.key, p.a4, SnapScale); }
        if (p.sLowCut > 20.5f) lowCut.setHP (p.sLowCut, 0.707f, sr); else lowCut.setNone();
        if (p.sHighCut < 19900.0f) highCut.setLP (p.sHighCut, 0.707f, sr); else highCut.setNone();
        maxLen = std::max (p.sBodyDecay, p.sNoiseDecay + p.sNoiseAttack) * 0.0035f + 0.02f;
    }
    void trigger (float v, const SynthParams& p)
    {
        beginTrigger (velGain (v, p.sVelSens));
        ph1 = ph2 = 0.0; noise = Noise (0x51u); hp.reset(); bp.reset(); snapHp.reset();
        hp.set (p.sNoiseHp, 0.7f, sr); bp.set (p.sNoiseBp, p.sNoiseQ, sr); snapHp.set (p.sSnapHz, 1.0f, sr);
    }
    inline float tick (const SynthParams& p) noexcept
    {
        float out = declick();
        lastAmp = 0.0f;
        if (active)
        {
            const float drop = std::pow (2.0f, p.sPitchDrop * std::exp (-t / std::max (0.0003f, p.sPitchTime * 0.001f)) / 12.0f);
            ph1 += f1 * drop / sr; ph1 -= std::floor (ph1);
            ph2 += f2 * drop / sr; ph2 -= std::floor (ph2);
            const float bodyEnv = std::exp (-t / msTau (p.sBodyDecay));
            const float body = (std::sin ((float) ph1 * kTwoPi) * (1.0f - p.sToneMix) + std::sin ((float) ph2 * kTwoPi) * p.sToneMix) * bodyEnv * p.sBodyLevel;

            const float n = noise.next();
            float l, b, h; hp.process (n, l, b, h);
            const float nf = h * 0.6f + bp.bp (n) * 0.9f;
            const float att = p.sNoiseAttack > 0.01f ? std::min (1.0f, t / (p.sNoiseAttack * 0.001f)) : 1.0f;
            const float noiseEnv = att * std::exp (-t / msTau (p.sNoiseDecay));
            const float nz = nf * noiseEnv * p.sNoiseLevel;

            float snap = 0.0f;
            if (p.sSnap > 0.0f && t < 0.01f)
            {
                snap = (snapHp.bp (n) * 1.5f + std::sin (kTwoPi * p.sSnapHz * t) * 0.5f) * std::exp (-t / 0.0012f) * p.sSnap;
            }
            float s = (body + nz + snap) * 0.7f;
            s = driveShape (s, p.sDrive, (int) p.sDriveType);
            s *= vel;
            lastAmp = std::max (bodyEnv * p.sBodyLevel, noiseEnv * p.sNoiseLevel);
            lastOut = s; out += s;
            t += 1.0f / sr;
            if (t > maxLen) { active = false; lastOut = 0.0f; }
        }
        else lastOut = 0.0f;
        out = highCut.process (lowCut.process (out));
        return out * p.sLevel;
    }
};

//==============================================================================
struct ClapVoice : DrumBase
{
    Noise noise { 0xC1A9u }; Svf bp, hp, lp;
    float starts[8] {}; int bursts = 3; float maxLen = 1.0f;
    void prepare (float s) { prepareBase (s); }
    void update (const SynthParams&) {}
    void trigger (float v, const SynthParams& p)
    {
        beginTrigger (velGain (v, p.cVelSens));
        noise = Noise (0xC1A9u); bp.reset(); hp.reset(); lp.reset();
        bp.set (p.cBpHz, p.cBpQ, sr); hp.set (p.cHpHz, 0.7f, sr); lp.set (p.cLpHz, 0.7f, sr);
        bursts = std::clamp ((int) std::lround (p.cBursts), 1, 8);
        Noise jit (0x77u);
        float at = 0.0f;
        for (int i = 0; i < bursts; ++i)
        {
            starts[i] = at;
            at += p.cSpacing * 0.001f * (1.0f + p.cJitter * 0.5f * jit.next());
        }
        maxLen = starts[bursts - 1] + std::max (p.cTailDecay, p.cBurstDecay) * 0.0035f + 0.02f;
    }
    inline float tick (const SynthParams& p) noexcept
    {
        float out = declick();
        lastAmp = 0.0f;
        if (active)
        {
            float e = 0.0f;
            const float tauB = msTau (p.cBurstDecay);
            for (int i = 0; i < bursts; ++i)
                if (t >= starts[i])
                {
                    const float a = i == bursts - 1 ? 1.0f : 0.85f - 0.05f * (float) i;
                    e = std::max (e, a * std::exp (-(t - starts[i]) / tauB));
                }
            const float last = starts[bursts - 1];
            if (t >= last) e = std::max (e, p.cTailLevel * std::exp (-(t - last) / msTau (p.cTailDecay)));

            float l, b, h;
            hp.process (bp.bp (noise.next()) * 2.5f, l, b, h);
            float s = lp.lp (h) * e;
            s = driveShape (s, p.cDrive, 0) * vel;
            lastAmp = e;
            lastOut = s; out += s;
            t += 1.0f / sr;
            if (t > maxLen) { active = false; lastOut = 0.0f; }
        }
        else lastOut = 0.0f;
        return out * p.cLevel;
    }
};

//==============================================================================
// Metall (808-artig: 6 Rechteck-Oszillatoren + Rauschen) für Closed Hat, Open Hat, Ride
struct MetalP { float level, freq, inharm, noise, bpHz, bpQ, hpHz, attack, decay, bell, bellHz, drive, highCut, velSens; };

inline MetalP metalParams (const SynthParams& p, int inst)
{
#define PF_METAL(x) MetalP { p.x##Level, p.x##Freq, p.x##Inharm, p.x##Noise, p.x##BpHz, p.x##BpQ, p.x##HpHz, p.x##Attack, p.x##Decay, \
                             p.x##Bell, p.x##BellHz, p.x##Drive, p.x##HighCut, p.x##VelSens }
    switch (inst)
    {
        case InOHat: return PF_METAL (o);
        case InRide: return PF_METAL (r);
        default:     return PF_METAL (h);
    }
#undef PF_METAL
}

struct MetalVoice : DrumBase
{
    float ph[6] {}; double bellPh = 0; Noise noise { 0x77u }; Svf bp, hp1, hp2; Biquad highCut;
    float maxLen = 1.0f, choke = 0.0f;
    void prepare (float s) { prepareBase (s); }
    void update (const MetalP& m)
    {
        if (m.highCut < 19900.0f) highCut.setLP (m.highCut, 0.707f, sr); else highCut.setNone();
        maxLen = m.decay * 0.0045f + m.attack * 0.001f + 0.01f;
    }
    void trigger (float v, const MetalP& m)
    {
        beginTrigger (velGain (v, m.velSens));
        for (int i = 0; i < 6; ++i) ph[i] = 0.1f * (float) i;
        bellPh = 0; noise = Noise (0x77u); bp.reset(); hp1.reset(); hp2.reset(); choke = 0.0f;
        bp.set (m.bpHz, m.bpQ, sr); hp1.set (m.hpHz, 0.7f, sr); hp2.set (m.hpHz * 0.85f, 0.7f, sr);
    }
    void doChoke() { if (active) choke = 1.0f; }
    inline float tick (const MetalP& m) noexcept
    {
        static constexpr float ratios[6] = { 1.0f, 1.4827f, 1.8003f, 2.5460f, 2.6303f, 3.8968f };
        float out = declick();
        lastAmp = 0.0f;
        if (active)
        {
            float sq = 0.0f;
            const float spread = 0.4f + 1.2f * m.inharm;
            for (int i = 0; i < 6; ++i)
            {
                const float r = 1.0f + (ratios[i] - 1.0f) * spread;
                ph[i] += m.freq * r / sr;
                if (ph[i] >= 1.0f) ph[i] -= 1.0f;
                sq += ph[i] < 0.5f ? 1.0f : -1.0f;
            }
            float x = sq * (1.0f / 6.0f) * (1.0f - m.noise) + noise.next() * m.noise;
            float l, b, h, s;
            hp1.process (bp.bp (x) * 1.6f + x * 0.3f, l, b, h);
            hp2.process (h, l, b, s);

            float e = m.attack > 0.01f ? std::min (1.0f, t / (m.attack * 0.001f)) : 1.0f;
            e *= std::exp (-t / msTau (m.decay));
            if (choke > 0.0f) { choke *= 0.995f; e *= choke; if (choke < 0.001f) active = false; }
            s *= e * 2.2f;
            if (m.bell > 0.0f)
            {
                bellPh += m.bellHz / sr; bellPh -= std::floor (bellPh);
                const float bph = (float) bellPh;
                const float bell = (std::sin (bph * kTwoPi) + 0.5f * std::sin (bph * 2.76f * kTwoPi)) * std::exp (-t / msTau (m.decay * 1.4f));
                s += bell * m.bell * 0.6f;
            }
            s = driveShape (s, m.drive, 0) * vel;
            lastAmp = e;
            lastOut = s; out += s;
            t += 1.0f / sr;
            if (t > maxLen) { active = false; lastOut = 0.0f; }
        }
        else lastOut = 0.0f;
        return highCut.process (out) * m.level;
    }
};

//==============================================================================
struct PercVoice : DrumBase
{
    double ph = 0, fmPh = 0; Noise noise { 0x9Cu }; Svf lp; float f0 = 220, maxLen = 1.0f;
    void prepare (float s) { prepareBase (s); }
    void update (const SynthParams& p) { maxLen = p.pDecay * 0.0045f + p.pAttack * 0.001f + 0.01f; }
    // semis: Note relativ zu A1 (45)
    void trigger (float v, int semis, const SynthParams& p)
    {
        beginTrigger (velGain (v, p.pVelSens));
        float f = p.pTune * std::pow (2.0f, (float) semis / 12.0f);
        if (p.pSnap > 0.5f) f = snapToKey (f, (int) p.key, p.a4, (int) p.pSnap);
        f0 = f; ph = 0.0; fmPh = 0.0; noise = Noise (0x9Cu); lp.reset();
        lp.set (p.pLpHz, 0.6f + p.pReso * 6.0f, sr);
    }
    inline float tick (const SynthParams& p) noexcept
    {
        float out = declick();
        lastAmp = 0.0f;
        if (active)
        {
            const float f = f0 * std::pow (2.0f, p.pPitchEnv * std::exp (-t / std::max (0.0003f, p.pPitchTime * 0.001f)) / 12.0f);
            float mod = 0.0f;
            if (p.pFm > 0.0f)
            {
                fmPh += f * p.pFmRatio / sr; fmPh -= std::floor (fmPh);
                mod = std::sin ((float) fmPh * kTwoPi) * p.pFm * 3.0f * std::exp (-t / 0.06f);
            }
            ph += f / sr; ph -= std::floor (ph);
            float phx = (float) ph + mod / kTwoPi; phx -= std::floor (phx);
            const float sn = std::sin (phx * kTwoPi);
            const float tri = phx < 0.25f ? 4.0f * phx : (phx < 0.75f ? 2.0f - 4.0f * phx : 4.0f * phx - 4.0f);
            const float sq = std::tanh (sn * 6.0f);
            float o = p.pWave < 0.5f ? sn + (tri - sn) * (p.pWave * 2.0f) : tri + (sq - tri) * ((p.pWave - 0.5f) * 2.0f);
            o += noise.next() * p.pNoise * std::exp (-t / 0.02f);
            o = lp.lp (o);
            float e = p.pAttack > 0.01f ? std::min (1.0f, t / (p.pAttack * 0.001f)) : 1.0f;
            e *= std::exp (-t / msTau (p.pDecay));
            float s = driveShape (o * e, p.pDrive, 0) * vel;
            lastAmp = e;
            lastOut = s; out += s;
            t += 1.0f / sr;
            if (t > maxLen) { active = false; lastOut = 0.0f; }
        }
        else lastOut = 0.0f;
        return out * p.pLevel;
    }
};

//==============================================================================
struct ZapVoice : DrumBase
{
    double ph = 0; Svf lp; unsigned ctr = 0; float pitchMul = 1.0f, len = 0.35f, lastFreq = 0.0f;
    void prepare (float s) { prepareBase (s); }
    void update (const SynthParams& p) { len = std::max (0.01f, p.zLength * 0.001f); }
    void trigger (float v, int semis, const SynthParams& p)
    {
        beginTrigger (velGain (v, p.zVelSens));
        pitchMul = std::pow (2.0f, (float) semis / 12.0f);
        ph = 0.0; lp.reset(); ctr = 0;
    }
    inline float freqAt (const SynthParams& p, float tt) const noexcept
    {
        const float sweep = std::max (0.002f, p.zSweep * 0.001f);
        const float u = std::min (1.0f, tt / sweep);
        // Kurve: 0 = linear in Hz, 1 = stark exponentiell
        const float k = 1.0f + p.zCurve * 8.0f;
        const float shape = (1.0f - std::exp (-k * u)) / (1.0f - std::exp (-k));
        const float lo = std::log (std::max (5.0f, p.zEndHz)), hi = std::log (std::max (5.0f, p.zStartHz));
        return std::exp (hi + (lo - hi) * shape) * pitchMul;
    }
    inline float tick (const SynthParams& p) noexcept
    {
        float out = declick();
        lastAmp = 0.0f; lastFreq = 0.0f;
        if (active)
        {
            const float f = freqAt (p, t);
            ph += f / sr; ph -= std::floor (ph);
            const float pp = (float) ph;
            const float dt = std::min (0.5f, f / sr);
            const float saw = (2.0f * pp - 1.0f) - polyBlep (pp, dt);
            float o = std::sin (pp * kTwoPi) * (1.0f - p.zWave) + saw * p.zWave;
            if ((ctr++ & 15) == 0) lp.set (std::min (sr * 0.45f, f * p.zLpRatio), 0.6f + p.zReso * 8.0f, sr);
            o = lp.lp (o);
            float e = std::min (1.0f, t / 0.0005f);
            const float fadeStart = len * 0.6f;
            if (t > fadeStart) { const float x = (t - fadeStart) / (len - fadeStart); e *= std::max (0.0f, 1.0f - x) * std::max (0.0f, 1.0f - x); }
            float s = driveShape (o * e, p.zDrive, 0) * vel;
            lastAmp = e; lastFreq = f;
            lastOut = s; out += s;
            t += 1.0f / sr;
            if (t >= len) { active = false; lastOut = 0.0f; }
        }
        else lastOut = 0.0f;
        return out * p.zLevel;
    }
};

//==============================================================================
struct RiserVoice : DrumBase
{
    double ph = 0; Noise noise { 0x5EEDu }; Svf bp; unsigned ctr = 0; float dur = 2.0f, relStart = 0, relT = 0; bool releasing = false; float lastFreq = 0.0f;
    void prepare (float s) { prepareBase (s); }
    void update (const SynthParams&) {}
    // durSec: Länge des Sweeps (aus Note-Länge oder Regler)
    void trigger (float v, float durSec)
    {
        beginTrigger (velGain (v, 0.5f));
        dur = std::max (0.05f, durSec); releasing = false; relT = 0; ph = 0; noise = Noise (0x5EEDu); bp.reset(); ctr = 0;
    }
    void release() { if (active && ! releasing) { releasing = true; relT = 0; relStart = std::min (1.0f, t / dur); } }
    inline float tick (const SynthParams& p) noexcept
    {
        float out = declick();
        lastAmp = 0.0f; lastFreq = 0.0f;
        if (active)
        {
            const float u = std::min (1.0f, t / dur);
            const float k = 0.3f + p.xCurve * 3.0f;
            const float shape = std::pow (u, k);
            const float f = std::exp (std::log (p.xStartHz) + (std::log (p.xEndHz) - std::log (p.xStartHz)) * shape);
            ph += f / sr; ph -= std::floor (ph);
            const float pp = (float) ph;
            const float tone = (std::sin (pp * kTwoPi) * 0.6f + ((2.0f * pp - 1.0f) - polyBlep (pp, std::min (0.5f, f / sr))) * 0.4f);
            if ((ctr++ & 15) == 0) bp.set (f * 1.5f, 0.7f + p.xReso * 6.0f, sr);
            const float nz = bp.bp (noise.next()) * 2.0f;
            float e = std::pow (u, 0.5f + p.xCurve);
            if (u >= 1.0f && ! releasing) release();
            if (releasing)
            {
                e = std::pow (relStart, 0.5f + p.xCurve) * std::exp (-relT / msTau (p.xRelease));
                relT += 1.0f / sr;
                if (relT > p.xRelease * 0.0035f + 0.01f) active = false;
            }
            float s = (nz * p.xNoise + tone * p.xTone) * e;
            s = driveShape (s, p.xDrive, 0) * vel;
            lastAmp = e; lastFreq = f;
            lastOut = s; out += s;
            t += 1.0f / sr;
            if (! active) lastOut = 0.0f;
        }
        else lastOut = 0.0f;
        return out * p.xLevel;
    }
};
} // namespace pf
