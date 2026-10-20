#pragma once
// PsyForge – ein Instrument pro Instanz: Kick, Bass, Snare, Clap, Hats, Ride, Perc, Zap oder Riser
#include "Drums.h"

namespace pf
{
// GM-nahe Drum-Noten (wie Goadrums)
constexpr int NoteKick = 36, NoteSnare = 38, NoteClap = 39, NoteCHat = 42, NoteOHat = 46, NoteRide = 51,
              NotePerc = 45, NoteZap = 49, NoteRiser = 54;

inline int gmNote (int inst)
{
    switch (inst)
    {
        case InKick:  return NoteKick;
        case InSnare: return NoteSnare;
        case InClap:  return NoteClap;
        case InCHat:  return NoteCHat;
        case InOHat:  return NoteOHat;
        case InRide:  return NoteRide;
        case InPerc:  return NotePerc;
        case InZap:   return NoteZap;
        case InRiser: return NoteRiser;
        default:      return -1;   // Bass: echte Tonhöhe
    }
}

// Gehört diese Note im Drum-Map-Modus zu diesem Instrument?
inline bool gmAccepts (int inst, int note)
{
    switch (inst)
    {
        case InKick:  return note == 35 || note == 36;
        case InSnare: return note == 37 || note == 38 || note == 40;
        case InClap:  return note == 39;
        case InCHat:  return note == 42 || note == 44;
        case InOHat:  return note == 46;
        case InRide:  return note == 51 || note == 53 || note == 59;
        case InPerc:  return note == 41 || note == 43 || note == 45 || note == 47 || note == 48 || note == 50;
        case InZap:   return note == 49 || note == 52 || note == 55 || note == 57;
        case InRiser: return note == 54;
        default:      return true;
    }
}

inline bool isTonal (int inst) { return inst == InBass || inst == InPerc; }

//==============================================================================
class Synth
{
public:
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        kick.prepare (sr); bass.prepare (sr); snare.prepare (sr); clap.prepare (sr); metal.prepare (sr);
        perc.prepare (sr); zap.prepare (sr); riser.prepare (sr);
        pendingCount = 0; duck = 0.0f;
        setParams (params, bpm);
    }

    void setParams (const SynthParams& p, double hostBpm)
    {
        params = p; bpm = std::max (40.0, hostBpm);
        inst = std::clamp ((int) p.inst, 0, NumInstruments - 1);
        switch (inst)
        {
            case InKick:  ks = makeKickShape (p, bpm, followHz); kick.updateEq (p); break;
            case InBass:  bs = makeBassShape (p); bass.updateEq (p, kickEndHz (p)); break;
            case InSnare: snare.update (p); break;
            case InClap:  clap.update (p); break;
            case InCHat: case InOHat: case InRide: mp = metalParams (p, inst); metal.update (mp); break;
            case InPerc:  perc.update (p); break;
            case InZap:   zap.update (p); break;
            case InRiser: riser.update (p); break;
            default: break;
        }
        duckTau = msTau (p.bDuckRelease);
    }

    int instrument() const { return inst; }

    // gateSamples: bekannte Notenlänge (Sequencer), -1 = unbekannt (Live-MIDI)
    void noteOn (int note, float vel, int gateSamples = -1)
    {
        int delay = 0;
        if (inst == InBass) delay = (int) (params.bOffset * 0.001f * sr);
        if (delay > 0)
        {
            if (pendingCount < (int) pending.size()) pending[(size_t) pendingCount++] = { note, vel, delay, gateSamples, true };
            return;
        }
        start (note, vel, gateSamples);
    }

    void noteOff (int note)
    {
        for (int i = 0; i < pendingCount; ++i)
            if (pending[(size_t) i].note == note && pending[(size_t) i].on)
            {
                if (pendingCount < (int) pending.size())
                    pending[(size_t) pendingCount++] = { note, 0.0f, pending[(size_t) i].delay + 1, -1, false };
                return;
            }
        stop (note);
    }

    void allOff()
    {
        pendingCount = 0;
        bass.noteOff();
        riser.release();
    }

    // Bass-Ducking: eine Kick hat vor 'samplesAgo' Samples angeschlagen
    void duckTrigger (int samplesAgo)
    {
        const float d = std::exp (-(float) std::max (0, samplesAgo) / (duckTau * sr));
        duck = std::max (duck, d);
    }

    inline float tick() noexcept
    {
        if (pendingCount > 0)
        {
            for (int i = 0; i < pendingCount; )
            {
                auto& e = pending[(size_t) i];
                if (--e.delay <= 0)
                {
                    if (e.on) start (e.note, e.vel, e.gate); else stop (e.note);
                    pending[(size_t) i] = pending[(size_t) --pendingCount];
                }
                else ++i;
            }
        }
        if (gateLeft > 0 && --gateLeft == 0) stop (gateNote);

        float s = 0.0f;
        lastFreq = 0.0f;
        switch (inst)
        {
            case InKick:  s = kick.tick (ks); lastAmp = kick.lastAmp; lastFreq = kick.lastFreq; break;
            case InBass:
                s = bass.tick (bs); lastAmp = bass.lastAmp;
                if (params.bDuck > 0.0f && duck > 0.0001f)
                {
                    s *= 1.0f - params.bDuck * duck;
                    duck *= std::exp (-1.0f / (duckTau * sr));
                }
                break;
            case InSnare: s = snare.tick (params); lastAmp = snare.lastAmp; break;
            case InClap:  s = clap.tick (params); lastAmp = clap.lastAmp; break;
            case InCHat: case InOHat: case InRide: s = metal.tick (mp); lastAmp = metal.lastAmp; break;
            case InPerc:  s = perc.tick (params); lastAmp = perc.lastAmp; break;
            case InZap:   s = zap.tick (params); lastAmp = zap.lastAmp; lastFreq = zap.lastFreq; break;
            case InRiser: s = riser.tick (params); lastAmp = riser.lastAmp; lastFreq = riser.lastFreq; break;
            default: break;
        }
        return s;
    }

    bool isActive() const
    {
        if (pendingCount > 0) return true;
        switch (inst)
        {
            case InKick:  return kick.active;
            case InBass:  return bass.active;
            case InSnare: return snare.active;
            case InClap:  return clap.active;
            case InCHat: case InOHat: case InRide: return metal.active;
            case InPerc:  return perc.active;
            case InZap:   return zap.active;
            case InRiser: return riser.active;
            default:      return false;
        }
    }

    float lastAmp = 0.0f, lastFreq = 0.0f;
    const KickShape& kickShape() const { return ks; }

private:
    void start (int note, float vel, int gate)
    {
        switch (inst)
        {
            case InKick:
                if (params.kFollow > 0.5f) { followHz = noteHz (note, params.a4); ks = makeKickShape (params, bpm, followHz); }
                kick.trigger (vel, ks);
                break;
            case InBass:  bass.noteOn (bassNoteHz (params, note), vel, bs); currentBass = note; break;
            case InSnare: snare.trigger (vel, params); break;
            case InClap:  clap.trigger (vel, params); break;
            case InCHat: case InOHat: case InRide: metal.trigger (vel, mp); break;
            case InPerc:  perc.trigger (vel, note - NotePerc, params); break;
            case InZap:   zap.trigger (vel, 0, params); break;
            case InRiser:
            {
                const float dur = gate > 0 ? (float) gate / sr : params.xLength * (float) (60.0 / bpm);
                riser.trigger (vel, dur);
                break;
            }
            default: break;
        }
        // bekannte Notenlänge: Note-Off selbst auslösen (nur Bass/Riser brauchen es)
        if (gate > 0 && inst == InBass) { gateLeft = gate; gateNote = note; }
    }
    void stop (int note)
    {
        if (inst == InBass && note == currentBass) bass.noteOff();
        if (inst == InRiser) riser.release();
    }

    struct Pending { int note; float vel; int delay; int gate; bool on; };

    SynthParams params;
    KickShape ks {}; BassShape bs {}; MetalP mp {};
    KickVoice kick; BassVoice bass; SnareVoice snare; ClapVoice clap; MetalVoice metal; PercVoice perc; ZapVoice zap; RiserVoice riser;
    float sr = 44100.0f, duck = 0.0f, duckTau = 0.02f, followHz = 0.0f;
    double bpm = 145.0;
    int inst = InKick, currentBass = -1, gateLeft = 0, gateNote = -1;
    std::array<Pending, 64> pending {};
    int pendingCount = 0;
};

//==============================================================================
// Note, mit der ein Instrument in Anzeige/Export/Analyse gespielt wird
inline int previewNote (const SynthParams& p, int inst)
{
    if (inst == InBass) return (int) p.note;
    if (inst == InKick && p.kFollow > 0.5f) return (int) std::lround (hzToMidi (p.kEndHz, p.a4));
    return gmNote (inst);
}

// Ein Anschlag des Instruments (Kick und Bass über die genaueren Spezial-Renderer)
inline RenderOut renderInstrument (const SynthParams& p, int inst, double sr, double bpm, float vel = 1.0f, float gateSec = -1.0f, float maxSec = 8.0f)
{
    if (inst == InKick) return renderKick (p, sr, bpm, vel, 0.01f);
    if (inst == InBass)
    {
        if (gateSec > 0) return renderBassHz (p, sr, bassNoteHz (p, (int) p.note), gateSec, gateSec + 0.25f, vel);
        return renderBass (p, sr, bpm, (int) p.note, vel);
    }
    RenderOut r; r.sr = sr;
    SynthParams q = p; q.inst = (float) inst;
    Synth s; s.prepare (sr); s.setParams (q, bpm);
    s.noteOn (previewNote (q, inst), vel, gateSec > 0 ? (int) (gateSec * sr) : -1);
    const int maxN = (int) (maxSec * sr);
    r.audio.reserve ((size_t) (sr * 0.5));
    for (int i = 0; i < maxN; ++i)
    {
        const float x = s.tick();
        r.audio.push_back (x); r.amp.push_back (s.lastAmp); r.freq.push_back (s.lastFreq);
        if (! s.isActive() && i > 32) break;
    }
    // Stille am Ende abschneiden
    float peak = 1e-9f; for (float v : r.audio) peak = std::max (peak, std::abs (v));
    size_t end = r.audio.size();
    while (end > 16 && std::abs (r.audio[end - 1]) < peak * 0.0003f) --end;
    end = std::min (r.audio.size(), end + (size_t) (sr * 0.003));
    r.audio.resize (end); r.amp.resize (end); r.freq.resize (end);
    return r;
}
} // namespace pf
