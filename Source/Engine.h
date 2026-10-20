#pragma once
// PsyForge – Klang-Engine für Psy-Kick und Rolling-Bass (header-only, ohne JUCE)
//
// Kick:  Sinus mit 3-stufigem Pitch-Sweep (Click -> Punch -> Grundton), Phasen-Reset,
//        Fishtail-Hüllkurve, Click-Layer, Knock-Oberton, Sättigung, EQ (Low-Cut, 200-Hz-Loch, High-Cut)
// Bass:  Saw/Square (+Unison) + Sub, 12/24-dB-Tiefpass mit Filter-Hüllkurve, Pluck, Drive,
//        Ducking durch die Kick, EQ inkl. Notch auf dem Kick-Grundton
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <vector>
#include <array>

namespace pf
{
constexpr float kPi = 3.14159265358979f;
constexpr float kTwoPi = 6.28318530717959f;

inline float noteHz (double midi, double a4) { return (float) (a4 * std::pow (2.0, (midi - 69.0) / 12.0)); }
inline double hzToMidi (double hz, double a4) { return 69.0 + 12.0 * std::log2 (std::max (1.0e-3, hz) / a4); }

enum SnapMode { SnapOff = 0, SnapRoot, SnapFifth, SnapScale };

// Frequenz auf die nächste passende Note der Tonart einrasten (key 0 = C, Moll-Tonleiter)
inline float snapToKey (float hz, int key, double a4, int mode)
{
    if (mode == SnapOff) return hz;
    static const int minor[] = { 0, 2, 3, 5, 7, 8, 10 };
    static const int fifth[] = { 0, 7 };
    const int* degrees = (mode == SnapScale) ? &minor[0] : &fifth[0];
    const int count = mode == SnapScale ? 7 : (mode == SnapFifth ? 2 : 1);
    const double midi = hzToMidi (hz, a4);
    double best = midi, bestDist = 1e9;
    for (int oct = -1; oct <= 11; ++oct)
        for (int d = 0; d < count; ++d)
        {
            const double cand = oct * 12 + key + degrees[d];
            if (std::abs (cand - midi) < bestDist) { bestDist = std::abs (cand - midi); best = cand; }
        }
    return noteHz (best, a4);
}

inline const char* noteName (int midi)
{
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return names[((midi % 12) + 12) % 12];
}
inline int noteOctave (int midi) { return midi / 12 - 1; }   // C1 = MIDI 24

//==============================================================================
struct Noise
{
    uint32_t s = 0x12345678u;
    explicit Noise (uint32_t seed = 0x12345678u) : s (seed ? seed : 1u) {}
    inline float next() noexcept
    {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        return (float) ((int32_t) s) * (1.0f / 2147483648.0f);
    }
};

// Cytomic TPT State-Variable-Filter
struct Svf
{
    float ic1 = 0, ic2 = 0, a1 = 0, a2 = 0, a3 = 0, k = 1;
    void reset() noexcept { ic1 = ic2 = 0; }
    void set (float fc, float q, float sr) noexcept
    {
        fc = std::clamp (fc, 10.0f, sr * 0.45f);
        const float g = std::tan (kPi * fc / sr);
        k = 1.0f / std::max (0.3f, q);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    inline void process (float v0, float& lp, float& bp, float& hp) noexcept
    {
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        lp = v2; bp = v1; hp = v0 - k * v1 - v2;
    }
    inline float lp (float x) noexcept { float l, b, h; process (x, l, b, h); return l; }
    inline float bp (float x) noexcept { float l, b, h; process (x, l, b, h); return b; }
};

// RBJ-Biquad (Transposed Direct Form II)
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    bool bypass = true;
    void reset() noexcept { z1 = z2 = 0; }
    void setNone() noexcept { bypass = true; }
    void setLP (float f, float q, float sr) noexcept { design (0, f, q, 0, sr); }
    void setHP (float f, float q, float sr) noexcept { design (1, f, q, 0, sr); }
    void setPeak (float f, float q, float db, float sr) noexcept { design (2, f, q, db, sr); }
    void design (int type, float f, float q, float db, float sr) noexcept
    {
        bypass = false;
        f = std::clamp (f, 5.0f, sr * 0.49f);
        const float w = kTwoPi * f / sr, cw = std::cos (w), sw = std::sin (w);
        const float alpha = sw / (2.0f * std::max (0.1f, q));
        float B0, B1, B2, A0, A1, A2;
        if (type == 0)      { B0 = (1 - cw) * 0.5f; B1 = 1 - cw; B2 = B0; A0 = 1 + alpha; A1 = -2 * cw; A2 = 1 - alpha; }
        else if (type == 1) { B0 = (1 + cw) * 0.5f; B1 = -(1 + cw); B2 = B0; A0 = 1 + alpha; A1 = -2 * cw; A2 = 1 - alpha; }
        else
        {
            const float A = std::pow (10.0f, db / 40.0f);
            B0 = 1 + alpha * A; B1 = -2 * cw; B2 = 1 - alpha * A;
            A0 = 1 + alpha / A; A1 = -2 * cw; A2 = 1 - alpha / A;
        }
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }
    inline float process (float x) noexcept
    {
        if (bypass) return x;
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
};

inline float polyBlep (float t, float dt) noexcept
{
    if (t < dt)        { t /= dt;            return t + t - t * t - 1.0f; }
    if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
    return 0.0f;
}

inline float driveShape (float x, float drive, int type) noexcept
{
    if (drive <= 0.0005f) return x;
    const float g = 1.0f + drive * 9.0f;
    switch (type)
    {
        case 1:  return std::clamp (x * g, -1.0f, 1.0f) * (1.0f - 0.15f * drive);           // Hard
        case 2:  return std::sin (std::clamp (x * g, -4.0f, 4.0f) * kPi * 0.5f) * 0.95f;   // Fold
        case 3:  return (std::tanh (x * g + 0.3f) - 0.291313f) / (std::tanh (g + 0.3f) - 0.291313f); // Asym (Röhre)
        default: return std::tanh (x * g) / std::tanh (g);                                  // Tanh
    }
}

//==============================================================================
// Alle Klangwerte (Klarwerte). Auswahl-Parameter werden als Index gespeichert.
struct SynthParams
{
    // Global
    float key = 6, a4 = 440, master = 0.8f, note = 30, bpm = 145, limiter = 1, inst = 0, midiFilter = 0;
    // Pattern / Sequencer
    float style = 2, section = 2, bassRhy = 0, variation = 0, swing = 0, humanize = 0.3f, seqOn = 0, source = 0;

    // Kick – Pitch
    float kLevel = 0.9f, kStartHz = 2600, kPunchHz = 190, kEndHz = 48, kClickTime = 1.2f, kSweepTime = 16, kDrift = 1.0f,
          kSnap = SnapRoot, kShape = 0, kStartPhase = 0, kKnock = 0.25f, kFollow = 0;
    // Kick – Amp
    float kAttack = 0.1f, kHold = 6, kDecay = 110, kLength = 207, kSync = 2, kFade = 16,
          kFishDepth = 0.3f, kFishPos = 12, kFishWidth = 8, kVelSens = 0.3f;
    // Kick – Click
    float kClickLevel = 0.35f, kClickHz = 4500, kClickDecay = 2.0f, kClickTone = 0.3f;
    // Kick – Drive / EQ
    float kDrive = 0.3f, kDriveType = 0, kTransient = 0.6f;
    float kLowCut = 20, kHoleHz = 200, kHoleDb = 4, kHoleQ = 1.2f, kHighCut = 16000;

    // Bass – Oszillatoren
    float bLevel = 0.75f, bOctave = 2, bFine = 0, bSaw = 1.0f, bSquare = 0.0f, bPw = 0.5f, bDetune = 0, bSub = 0.35f, bSubOct = 0, bSubInv = 0,
          bRetrig = 1, bStartPhase = 0.5f;
    // Bass – Amp
    float bAttack = 0.5f, bDecay = 75, bSustain = 0.25f, bRelease = 12, bVelSens = 0.4f, bOffset = 0;
    // Bass – Filter
    float bCutoff = 420, bReso = 0.2f, bFenv = 3.0f, bFattack = 0, bFdecay = 45, bKeytrack = 0.5f, bSlope = 1;
    // Bass – Pluck / Drive / Ducking / EQ
    float bPluck = 0, bPluckDecay = 8, bDrive = 0.2f, bDriveType = 0, bDuck = 0.6f, bDuckRelease = 60, bDuckSrc = 1;
    float bLowCut = 30, bNotchDb = 0, bHighCut = 9000;

    // Snare
    float sLevel = 0.8f, sTone1 = 190, sTone2 = 330, sToneMix = 0.35f, sBodyLevel = 0.7f, sBodyDecay = 70, sPitchDrop = 7, sPitchTime = 10,
          sNoiseLevel = 0.7f, sNoiseHp = 1500, sNoiseBp = 5000, sNoiseQ = 0.8f, sNoiseAttack = 0.5f, sNoiseDecay = 140,
          sSnap = 0.3f, sSnapHz = 3000, sDrive = 0.3f, sDriveType = 0, sLowCut = 80, sHighCut = 16000, sVelSens = 0.5f, sKeyLock = 0;
    // Clap
    float cLevel = 0.8f, cBursts = 3, cSpacing = 10, cJitter = 0.2f, cBurstDecay = 3.5f, cTailLevel = 0.7f, cTailDecay = 180,
          cBpHz = 1400, cBpQ = 1.3f, cHpHz = 600, cLpHz = 12000, cDrive = 0.2f, cVelSens = 0.5f;
    // Metall-Stimmen: Closed Hat (h), Open Hat (o), Ride (r)
    float hLevel = 0.6f, hFreq = 360, hInharm = 0.5f, hNoise = 0.35f, hBpHz = 9000, hBpQ = 0.8f, hHpHz = 7000, hAttack = 0.3f, hDecay = 45,
          hBell = 0, hBellHz = 800, hDrive = 0.1f, hHighCut = 18000, hVelSens = 0.6f;
    float oLevel = 0.55f, oFreq = 340, oInharm = 0.5f, oNoise = 0.35f, oBpHz = 8500, oBpQ = 0.8f, oHpHz = 6000, oAttack = 0.5f, oDecay = 260,
          oBell = 0, oBellHz = 800, oDrive = 0.1f, oHighCut = 18000, oVelSens = 0.5f;
    float rLevel = 0.45f, rFreq = 420, rInharm = 0.6f, rNoise = 0.25f, rBpHz = 6000, rBpQ = 0.7f, rHpHz = 3500, rAttack = 1, rDecay = 700,
          rBell = 0.35f, rBellHz = 900, rDrive = 0.05f, rHighCut = 16000, rVelSens = 0.5f;
    // Perc (tonal, Note A1 = Grundton)
    float pLevel = 0.7f, pTune = 220, pWave = 0, pPitchEnv = 12, pPitchTime = 12, pFm = 0, pFmRatio = 1.5f, pAttack = 0.2f, pDecay = 120,
          pNoise = 0, pLpHz = 8000, pReso = 0.2f, pDrive = 0.3f, pSnap = 3, pVelSens = 0.5f;
    // Zap
    float zLevel = 0.6f, zStartHz = 3000, zEndHz = 60, zSweep = 250, zCurve = 0.6f, zWave = 0, zLpRatio = 4, zReso = 0.3f,
          zLength = 350, zDrive = 0.2f, zVelSens = 0.5f;
    // Riser
    float xLevel = 0.55f, xLength = 8, xNoise = 0.7f, xTone = 0.4f, xStartHz = 200, xEndHz = 6000, xCurve = 0.5f, xReso = 0.4f,
          xRelease = 300, xDrive = 0.1f;
};

// Länge der Kick (Sync)
inline float syncBeats (int sync)
{
    static const float beats[] = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };
    return beats[std::clamp (sync, 0, 4)];
}

//==============================================================================
// Vorberechnete Kick-Form (aus Parametern, pro Block)
struct KickShape
{
    float startHz, punchHz, endHz, tauC, tauS, drift, len, fade, attack, hold, tauDecay;
    float fishDepth, fishPos, fishSigma, clickLevel, clickHz, tauClick, clickTone, knock, shape, startPhase;
    float drive, transient, level, velSens;
    int driveType;
};

inline float kickEndHz (const SynthParams& p, float followHz = 0.0f)
{
    float f = p.kEndHz;
    if (p.kFollow > 0.5f && followHz > 0.0f) f = followHz;
    else f = snapToKey (f, (int) p.key, p.a4, (int) p.kSnap);
    return f;
}

inline KickShape makeKickShape (const SynthParams& p, double bpm, float followHz = 0.0f)
{
    KickShape k {};
    k.endHz = kickEndHz (p, followHz);
    // Start/Punch skalieren mit, wenn der Grundton der Note folgt
    const float scale = (p.kFollow > 0.5f && followHz > 0.0f) ? followHz / std::max (1.0f, p.kEndHz) : 1.0f;
    k.startHz = p.kStartHz * scale;
    k.punchHz = p.kPunchHz * scale;
    k.tauC = std::max (0.00005f, p.kClickTime * 0.001f);
    k.tauS = std::max (0.0003f, p.kSweepTime * 0.001f);
    k.drift = p.kDrift;
    const int sync = (int) p.kSync;
    k.len = sync > 0 ? (float) (syncBeats (sync) * 60.0 / std::max (40.0, bpm)) : p.kLength * 0.001f;
    k.len = std::max (0.01f, k.len);
    k.fade = std::min (k.len * 0.9f, std::max (0.0005f, p.kFade * 0.001f));
    k.attack = p.kAttack * 0.001f;
    k.hold = p.kHold * 0.001f;
    k.tauDecay = std::max (0.001f, p.kDecay * 0.001f / 2.302585f);   // Decay = Zeit bis -20 dB
    k.fishDepth = std::clamp (p.kFishDepth, 0.0f, 0.98f);
    k.fishPos = p.kFishPos * 0.001f;
    k.fishSigma = std::max (0.0003f, p.kFishWidth * 0.0005f);
    k.clickLevel = p.kClickLevel;
    k.clickHz = p.kClickHz;
    k.tauClick = std::max (0.0001f, p.kClickDecay * 0.001f);
    k.clickTone = p.kClickTone;
    k.knock = p.kKnock;
    k.shape = p.kShape;
    k.startPhase = p.kStartPhase;
    k.drive = p.kDrive;
    k.driveType = (int) p.kDriveType;
    k.transient = p.kTransient;
    k.level = p.kLevel;
    k.velSens = p.kVelSens;
    return k;
}

inline float kickFreqAt (const KickShape& k, float t) noexcept
{
    const float driftMul = std::pow (2.0f, -k.drift * std::min (1.0f, t / k.len) / 12.0f);
    const float f = k.endHz * driftMul + (k.punchHz - k.endHz) * std::exp (-t / k.tauS)
                                       + (k.startHz - k.punchHz) * std::exp (-t / k.tauC);
    return std::max (5.0f, f);
}

inline float kickAmpAt (const KickShape& k, float t) noexcept
{
    if (t >= k.len || t < 0.0f) return 0.0f;
    float a = k.attack > 0.00001f ? std::min (1.0f, t / k.attack) : 1.0f;
    a *= t < k.hold ? 1.0f : std::exp (-(t - k.hold) / k.tauDecay);
    if (k.fishDepth > 0.0f)
    {
        const float z = (t - k.fishPos) / k.fishSigma;
        a *= 1.0f - k.fishDepth * std::exp (-0.5f * z * z);
    }
    const float fadeStart = k.len - k.fade;
    if (t > fadeStart)
    {
        const float x = (t - fadeStart) / k.fade;       // 0..1
        a *= 0.5f + 0.5f * std::cos (x * kPi);
    }
    return a;
}

//==============================================================================
struct KickVoice
{
    float sr = 44100.0f, t = 0.0f, vel = 1.0f;
    bool active = false;
    double phase = 0;
    Noise noise { 0xA5A5A5u };
    Svf clickBp;
    Biquad lowCut, hole, highCut;
    float lastOut = 0.0f, fade = 0.0f; int fadeLeft = 0, fadeLen = 96;
    float lastFreq = 0.0f, lastAmp = 0.0f;

    void prepare (float sampleRate)
    {
        sr = sampleRate; active = false; t = 0;
        clickBp.reset(); lowCut.reset(); hole.reset(); highCut.reset();
        fadeLen = std::max (16, (int) (sr * 0.002f));
    }
    void updateEq (const SynthParams& p)
    {
        if (p.kLowCut > 10.5f) lowCut.setHP (p.kLowCut, 0.707f, sr); else lowCut.setNone();
        if (p.kHoleDb > 0.05f) hole.setPeak (p.kHoleHz, p.kHoleQ, -p.kHoleDb, sr); else hole.setNone();
        if (p.kHighCut < 19900.0f) highCut.setLP (p.kHighCut, 0.707f, sr); else highCut.setNone();
    }
    void trigger (float v, const KickShape& k)
    {
        fade = lastOut; fadeLeft = fadeLen;
        active = true; t = 0.0f;
        vel = 1.0f - k.velSens + k.velSens * std::clamp (v, 0.0f, 1.0f);
        phase = k.startPhase;
        clickBp.reset();
    }
    inline float tick (const KickShape& k) noexcept
    {
        float out = 0.0f;
        if (fadeLeft > 0) out += fade * (float) fadeLeft-- / (float) fadeLen;
        lastFreq = 0; lastAmp = 0;
        if (active)
        {
            const float f = kickFreqAt (k, t);
            phase += f / sr;
            phase -= std::floor (phase);
            const float ph = (float) phase;

            const float sn = std::sin (ph * kTwoPi);
            float osc = sn;
            if (k.shape > 0.0f)
            {
                const float tri = (ph < 0.25f ? 4.0f * ph : (ph < 0.75f ? 2.0f - 4.0f * ph : 4.0f * ph - 4.0f));
                const float sat = std::tanh (sn * 3.0f) / std::tanh (3.0f);
                osc = k.shape < 0.5f ? sn + (tri - sn) * (k.shape * 2.0f)
                                     : tri + (sat - tri) * ((k.shape - 0.5f) * 2.0f);
            }
            if (k.knock > 0.0f)
                osc += k.knock * 0.6f * std::sin (ph * 2.0f * kTwoPi) * std::exp (-t / 0.015f);

            const float amp = kickAmpAt (k, t);
            float s = osc * amp;

            if (k.clickLevel > 0.0f && t < k.tauClick * 8.0f)
            {
                clickBp.set (k.clickHz, 1.1f, sr);
                const float n = clickBp.bp (noise.next()) * 2.2f;
                const float tone = std::sin (kTwoPi * k.clickHz * t);
                const float ce = std::exp (-t / k.tauClick);
                s += (n * (1.0f - k.clickTone) + tone * k.clickTone) * ce * k.clickLevel;
            }

            if (k.drive > 0.0005f)
            {
                const float d = driveShape (s, k.drive, k.driveType);
                const float tw = k.transient * std::exp (-t / 0.004f);
                s = s * tw + d * (1.0f - tw);
            }

            s *= vel;
            lastFreq = f; lastAmp = amp;
            lastOut = s;
            out += s;
            t += 1.0f / sr;
            if (t >= k.len) { active = false; lastOut = 0.0f; }
        }
        else lastOut = 0.0f;

        out = lowCut.process (out);
        out = hole.process (out);
        out = highCut.process (out);
        return out * k.level;
    }
};

//==============================================================================
struct BassShape
{
    float saw, square, pw, detune, sub, subMul, subSign, startPhase, attack, tauDecay, sustain, tauRelease, velSens;
    float cutoff, q, fenv, fattack, tauF, keytrack, pluck, tauPluck, drive, level;
    int slope, driveType;
    bool retrig;
};

inline BassShape makeBassShape (const SynthParams& p)
{
    BassShape b {};
    b.saw = p.bSaw; b.square = p.bSquare; b.pw = std::clamp (p.bPw, 0.05f, 0.95f);
    b.detune = p.bDetune; b.sub = p.bSub; b.subMul = p.bSubOct > 0.5f ? 0.5f : 1.0f; b.subSign = p.bSubInv > 0.5f ? 1.0f : -1.0f;
    b.startPhase = p.bStartPhase;
    b.attack = p.bAttack * 0.001f;
    b.tauDecay = std::max (0.0005f, p.bDecay * 0.001f / 2.302585f);
    b.sustain = p.bSustain;
    b.tauRelease = std::max (0.0002f, p.bRelease * 0.001f / 2.302585f);
    b.velSens = p.bVelSens;
    b.cutoff = p.bCutoff; b.q = 0.55f + p.bReso * 9.0f; b.fenv = p.bFenv;
    b.fattack = p.bFattack * 0.001f;
    b.tauF = std::max (0.0005f, p.bFdecay * 0.001f / 2.302585f);
    b.keytrack = p.bKeytrack;
    b.pluck = p.bPluck; b.tauPluck = std::max (0.0002f, p.bPluckDecay * 0.001f / 2.302585f);
    b.drive = p.bDrive; b.driveType = (int) p.bDriveType;
    b.level = p.bLevel; b.slope = (int) p.bSlope; b.retrig = p.bRetrig > 0.5f;
    return b;
}

inline float bassNoteHz (const SynthParams& p, int midiNote)
{
    const int oct = (int) p.bOctave - 2;             // Auswahl 0..4 = -2..+2 Oktaven
    return noteHz (midiNote + 12 * oct + p.bFine / 100.0, p.a4);
}

struct BassVoice
{
    float sr = 44100.0f;
    bool active = false, gate = false;
    double ph1 = 0, ph2 = 0, phSub = 0;
    float t = 0, tRel = 0, relStart = 0, env = 0, vel = 1, freq = 55;
    Svf f1, f2;
    Biquad lowCut, notch, highCut;
    float lastOut = 0, fade = 0; int fadeLeft = 0, fadeLen = 48;
    float lastAmp = 0;

    void prepare (float sampleRate)
    {
        sr = sampleRate; active = false; gate = false;
        f1.reset(); f2.reset(); lowCut.reset(); notch.reset(); highCut.reset();
        fadeLen = std::max (8, (int) (sr * 0.001f));
    }
    void updateEq (const SynthParams& p, float kickHz)
    {
        if (p.bLowCut > 10.5f) lowCut.setHP (p.bLowCut, 0.707f, sr); else lowCut.setNone();
        if (p.bNotchDb > 0.05f) notch.setPeak (kickHz, 4.0f, -p.bNotchDb, sr); else notch.setNone();
        if (p.bHighCut < 19900.0f) highCut.setLP (p.bHighCut, 0.707f, sr); else highCut.setNone();
    }
    void noteOn (float hz, float v, const BassShape& b)
    {
        fade = lastOut; fadeLeft = fadeLen;
        freq = hz;
        vel = 1.0f - b.velSens + b.velSens * std::clamp (v, 0.0f, 1.0f);
        if (b.retrig || ! active)
        {
            ph1 = ph2 = b.startPhase; phSub = b.startPhase;
            f1.reset(); f2.reset();
        }
        active = true; gate = true; t = 0; tRel = 0; env = 0;
    }
    void noteOff()
    {
        if (! gate) return;
        gate = false; relStart = env; tRel = 0;
    }
    inline float tick (const BassShape& b) noexcept
    {
        float out = 0.0f;
        if (fadeLeft > 0) out += fade * (float) fadeLeft-- / (float) fadeLen;
        lastAmp = 0;
        if (active)
        {
            // Hüllkurve
            float e;
            if (gate)
            {
                if (t < b.attack) e = t / b.attack;
                else e = b.sustain + (1.0f - b.sustain) * std::exp (-(t - b.attack) / b.tauDecay);
            }
            else
            {
                e = relStart * std::exp (-tRel / b.tauRelease);
                tRel += 1.0f / sr;
            }
            env = e;

            const float pitchMul = b.pluck > 0.0f ? std::pow (2.0f, b.pluck * std::exp (-t / b.tauPluck) / 12.0f) : 1.0f;
            const float f = freq * pitchMul;
            const float detMul = std::pow (2.0f, b.detune / 1200.0f);

            auto oscAt = [&] (double& ph, float fr) -> float
            {
                const float dt = std::min (0.5f, fr / sr);
                const float p = (float) ph;
                float o = 0.0f;
                if (b.saw > 0.0f) o += b.saw * ((2.0f * p - 1.0f) - polyBlep (p, dt));
                if (b.square > 0.0f)
                {
                    // Rechteck gegenphasig zum Saw-Grundton definiert -> beide addieren sich im Grundton
                    float sq = p < b.pw ? -1.0f : 1.0f;
                    sq -= polyBlep (p, dt);
                    float p2 = p - b.pw; if (p2 < 0.0f) p2 += 1.0f;
                    sq += polyBlep (p2, dt);
                    o += b.square * sq;
                }
                ph += fr / sr; ph -= std::floor (ph);
                return o;
            };

            float top;
            if (b.detune > 0.05f) top = 0.5f * (oscAt (ph1, f / std::sqrt (detMul)) + oscAt (ph2, f * std::sqrt (detMul)));
            else top = oscAt (ph1, f);

            // Filter
            float fe = b.fattack > 0.00001f && t < b.fattack ? t / b.fattack
                                                              : std::exp (-(t - b.fattack) / b.tauF);
            const float key = std::pow (std::max (10.0f, freq) / 55.0f, b.keytrack);
            const float cut = std::clamp (b.cutoff * key * std::pow (2.0f, b.fenv * fe), 20.0f, sr * 0.45f);
            if (b.slope > 0)
            {
                f1.set (cut, 0.707f, sr); f2.set (cut, b.q, sr);
                top = f2.lp (f1.lp (top));
            }
            else { f2.set (cut, b.q, sr); top = f2.lp (top); }

            top = driveShape (top, b.drive, b.driveType);

            float sub = 0.0f;
            if (b.sub > 0.0f)
            {
                sub = b.subSign * std::sin ((float) phSub * kTwoPi) * b.sub;     // Standard: phasengleich mit dem Saw-Grundton
                phSub += f * b.subMul / sr; phSub -= std::floor (phSub);
            }

            float s = (top * 0.8f + sub) * e * vel;
            lastAmp = e;
            lastOut = s;
            out += s;
            t += 1.0f / sr;
            if (! gate && e < 0.0001f) { active = false; lastOut = 0.0f; }
        }
        else lastOut = 0.0f;

        out = lowCut.process (out);
        out = notch.process (out);
        out = highCut.process (out);
        return out * b.level;
    }
};

//==============================================================================
// Offline-Render (Anzeige, Export, Analyse)
struct RenderOut
{
    std::vector<float> audio, freq, amp;
    double sr = 44100.0;
};

inline RenderOut renderKick (const SynthParams& p, double sr, double bpm, float vel = 1.0f, float extraSec = 0.02f)
{
    RenderOut r; r.sr = sr;
    KickVoice v; v.prepare ((float) sr); v.updateEq (p);
    const KickShape k = makeKickShape (p, bpm);
    v.trigger (vel, k);
    const int n = (int) ((k.len + extraSec) * sr) + 1;
    r.audio.resize ((size_t) n); r.freq.resize ((size_t) n); r.amp.resize ((size_t) n);
    for (int i = 0; i < n; ++i)
    {
        r.audio[(size_t) i] = v.tick (k);
        r.freq[(size_t) i] = v.lastFreq;
        r.amp[(size_t) i] = v.lastAmp;
    }
    return r;
}

// eine Bassnote: gate = Note-Länge (s), total = Gesamtlänge (s)
inline RenderOut renderBassHz (const SynthParams& p, double sr, float hz, float gateSec, float totalSec, float vel = 1.0f)
{
    RenderOut r; r.sr = sr;
    BassVoice v; v.prepare ((float) sr);
    v.updateEq (p, kickEndHz (p));
    const BassShape b = makeBassShape (p);
    v.noteOn (hz, vel, b);
    const int n = std::max (1, (int) (totalSec * sr));
    const int gateN = (int) (gateSec * sr);
    r.audio.resize ((size_t) n); r.amp.resize ((size_t) n);
    for (int i = 0; i < n; ++i)
    {
        if (i == gateN) v.noteOff();
        r.audio[(size_t) i] = v.tick (b);
        r.amp[(size_t) i] = v.lastAmp;
    }
    return r;
}

inline RenderOut renderBass (const SynthParams& p, double sr, double bpm, int note, float vel = 1.0f)
{
    const float sixteenth = (float) (15.0 / std::max (40.0, bpm));
    return renderBassHz (p, sr, bassNoteHz (p, note), sixteenth * 0.7f, sixteenth * 1.6f, vel);
}

} // namespace pf
