#include "Patterns.h"
#include <cmath>
#include <algorithm>
#include <random>

namespace pf
{
// Kurznamen der Instrumente (wie in Goadrums)
namespace
{
constexpr int Kick = InKick, Bass = InBass, Snare = InSnare, Clap = InClap, CHat = InCHat, OHat = InOHat,
              Ride = InRide, Perc = InPerc, Zap = InZap, Riser = InRiser;
}

//==============================================================================
namespace
{
enum BassRhythm { BrDefault = 0, BrOffbeat, BrGallop, BrRolling, BrTriplet, BrTriplet16, BrBounce, BrDarkroll };
enum HatMode { HatFull16 = 0, HatOff16, HatOff8 };
enum BackMode { BackSnare = 0, BackClap, BackBoth };

struct StyleDef
{
    const char* name;
    double bpm;
    int bass;
    int hats;
    int back;
    uint16_t percA, percB;          // 16tel-Masken (Bit 0 = erste 16tel)
    int percSemis[4];
    bool flicks;                    // 1/32-Hat-Flicks (dark / hi-tech)
    bool darkNotes;                 // kleine Sekunde als Bass-Variation
    int accentSet = 0;              // Hat-Betonung (Variation)
};

constexpr uint16_t m (std::initializer_list<int> steps)
{
    uint16_t r = 0;
    for (int s : steps) r = (uint16_t) (r | (1u << s));
    return r;
}

const StyleDef styles[NumStyles] =
{
    { "Oldschool Goa",   142, BrRolling,   HatFull16, BackSnare, m ({ 3, 10, 14 }),            m ({ 3, 6, 11, 14 }),       { 2, 0, -2, 3 },  false, false },
    { "Neo-Goa",         145, BrRolling,   HatFull16, BackBoth,  m ({ 2, 7, 11, 15 }),         m ({ 3, 7, 10, 13 }),       { 3, 0, 5, -2 },  false, false },
    { "Full-On",         145, BrTriplet,   HatFull16, BackSnare, m ({ 3, 7, 11 }),             m ({ 3, 6, 9, 13 }),        { 0, 2, 3, 0 },   false, false },
    { "Progressive",     138, BrOffbeat,   HatOff16,  BackClap,  m ({ 7, 15 }),                m ({ 7, 13, 15 }),          { 0, -2, 0, 3 },  false, false },
    { "Psygressive",     140, BrGallop,    HatOff16,  BackClap,  m ({ 3, 11, 14 }),            m ({ 3, 6, 11 }),           { -2, 0, -4, 0 }, false, true  },
    { "Darkpsy / Forest",150, BrDarkroll,  HatFull16, BackSnare, m ({ 1, 5, 6, 9, 13, 14 }),   m ({ 2, 5, 9, 10, 13 }),    { -4, -2, 3, 0 }, true,  true  },
    { "Hi-Tech",         175, BrTriplet16, HatFull16, BackSnare, m ({ 1, 3, 5, 9, 11, 13 }),   m ({ 1, 5, 7, 9, 13, 15 }), { 5, 3, 0, 2 },   true,  false },
    { "Prog BR",         145, BrBounce,    HatOff16,  BackBoth,  m ({ 3, 7, 10, 14 }),         m ({ 3, 6, 10, 15 }),       { 0, 3, 0, -2 },  false, false },
};

//==============================================================================
struct Builder
{
    Pattern p;
    void add (int voice, int bar, int tick, float vel, int len = 12, int note = 0)
    {
        Event e;
        e.voice = voice; e.tick = bar * BarTicks + tick; e.vel = std::clamp (vel, 0.05f, 1.0f);
        e.length = len; e.note = note;
        p.events.push_back (e);
    }
    Pattern finish()
    {
        std::stable_sort (p.events.begin(), p.events.end(), [] (const Event& a, const Event& b) { return a.tick < b.tick; });
        return p;
    }
};

void kicks (Builder& b, int bar, int beatMask = 0xF, float vel = 1.0f)
{
    for (int beat = 0; beat < 4; ++beat)
        if (beatMask & (1 << beat)) b.add (Kick, bar, beat * PPQ, vel);
}

void hats (Builder& b, const StyleDef& s, int bar, int mode, float gain = 1.0f, bool flicks = false)
{
    static const float accents[4][4] = { { 0.45f, 0.32f, 0.78f, 0.38f }, { 0.6f, 0.3f, 0.7f, 0.3f },
                                         { 0.35f, 0.5f, 0.8f, 0.45f },   { 0.5f, 0.25f, 0.85f, 0.55f } };
    const float* accent = accents[juce::jlimit (0, 3, s.accentSet)];
    for (int beat = 0; beat < 4; ++beat)
    {
        const int base = beat * PPQ;
        if (mode == HatFull16)
            for (int i = 0; i < 4; ++i) b.add (CHat, bar, base + i * 24, accent[i] * gain);
        else if (mode == HatOff16)
        {
            b.add (CHat, bar, base + 24, 0.5f * gain);
            b.add (CHat, bar, base + 72, 0.6f * gain);
        }
        else
            b.add (CHat, bar, base + 48, 0.65f * gain);
    }
    if (flicks || s.flicks)
        b.add (CHat, bar, 3 * PPQ + 84, 0.35f * gain);
}

void offbeatOpen (Builder& b, int bar, float vel = 0.75f)
{
    for (int beat = 0; beat < 4; ++beat) b.add (OHat, bar, beat * PPQ + 48, vel);
}

void ride (Builder& b, int bar, bool sixteenths, float gain = 1.0f)
{
    for (int beat = 0; beat < 4; ++beat)
    {
        if (sixteenths)
        {
            b.add (Ride, bar, beat * PPQ + 24, 0.35f * gain);
            b.add (Ride, bar, beat * PPQ + 72, 0.4f * gain);
        }
        b.add (Ride, bar, beat * PPQ + 48, 0.6f * gain);
    }
}

void backbeat (Builder& b, int bar, int mode, float vel = 0.85f, bool firstOnly = false)
{
    for (int beat : { 1, 3 })
    {
        if (firstOnly && beat == 3) continue;
        if (mode == BackSnare || mode == BackBoth) b.add (Snare, bar, beat * PPQ, vel);
        if (mode == BackClap  || mode == BackBoth) b.add (Clap,  bar, beat * PPQ, vel * (mode == BackBoth ? 0.75f : 1.0f));
    }
}

void perc (Builder& b, const StyleDef& s, int bar, bool variantB, float gain = 1.0f)
{
    const uint16_t mask = variantB ? s.percB : s.percA;
    int n = 0;
    for (int step = 0; step < 16; ++step)
        if (mask & (1u << step))
        {
            const float v = (n % 2 == 0 ? 0.7f : 0.55f) * gain;
            b.add (Perc, bar, step * 24, v, 12, s.percSemis[n % 4]);
            ++n;
        }
}

// Bass für einen Takt. noteFor(beat, idx, lastInBeat) liefert Halbtonversatz
template <typename NoteFn>
void bassBar (Builder& b, int bar, int rhythm, float gain, bool darkRollHere, NoteFn noteFor)
{
    for (int beat = 0; beat < 4; ++beat)
    {
        const int base = beat * PPQ;
        struct N { int tick, len; float vel; int extra; };
        N notes[8]; int count = 0;
        auto push = [&] (int t, int l, float v, int ex = 0) { notes[count++] = { t, l, v, ex }; };

        switch (rhythm)
        {
            case BrOffbeat:   push (48, 22, 1.0f); break;
            case BrGallop:    push (48, 20, 0.95f); push (72, 20, 1.0f); break;
            case BrTriplet:   push (32, 26, 0.9f);  push (64, 26, 1.0f); break;
            case BrTriplet16: for (int i = 1; i <= 5; ++i) push (i * 16, 13, i % 2 ? 1.0f : 0.85f); break;
            case BrBounce:    push (48, 20, 1.0f);  push (72, 16, 0.7f, 12); break;
            case BrDarkroll:
                if (darkRollHere && beat == 3)
                    for (int i = 0; i < 6; ++i) push (24 + i * 12, 10, 0.8f + 0.04f * (float) i);
                else { push (24, 20, 0.85f); push (48, 20, 1.0f); push (72, 20, 0.92f); }
                break;
            case BrRolling:
            default:          push (24, 20, 0.85f); push (48, 20, 1.0f); push (72, 20, 0.92f); break;
        }

        for (int i = 0; i < count; ++i)
            b.add (Bass, bar, base + notes[i].tick, notes[i].vel * gain, notes[i].len,
                   notes[i].extra + noteFor (beat, i, i == count - 1));
    }
}
} // namespace

//==============================================================================
juce::StringArray styleNames()
{
    juce::StringArray a;
    for (auto& s : styles) a.add (s.name);
    return a;
}

juce::StringArray sectionNames()
{
    return { "Intro", "Buildup", "Drop", "Offbeat", "Full Energy", "Festival", "Breakdown", "Drop 2", "End" };
}

juce::StringArray bassRhythmNames()
{
    return { "Style-Standard", "Offbeat (K-B-)", "Gallop (K-BB)", "Rolling (KBBB)",
             "Triplet (K-B-B)", "Triplet 16 (Hi-Tech)", "Bounce (BR)", "Darkroll (KBBB+1/32)" };
}

juce::StringArray keyNames()
{
    return { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
}

double styleBpm (int style)       { return styles[juce::jlimit (0, NumStyles - 1, style)].bpm; }
int styleDefaultBass (int style)  { return styles[juce::jlimit (0, NumStyles - 1, style)].bass; }

//==============================================================================
namespace
{
uint16_t mutateMask (uint16_t mask, std::mt19937& rng)
{
    static const int cand[] = { 1, 2, 3, 5, 6, 7, 9, 10, 11, 13, 14, 15 };
    for (int tries = 0; tries < 8; ++tries)
    {
        uint16_t m2 = mask;
        for (int k = 0; k < 2; ++k) m2 = (uint16_t) (m2 ^ (1u << cand[rng() % 12]));
        int bits = 0;
        for (int i = 0; i < 16; ++i) bits += (m2 >> i) & 1;
        if (bits >= 2 && bits <= 7 && m2 != mask) return m2;
    }
    return mask;
}
}

Pattern makePattern (int style, int section, int bassRhythm, int variation)
{
    StyleDef s = styles[juce::jlimit (0, NumStyles - 1, style)];
    const bool varied = variation > 0;
    std::mt19937 grooveRng ((uint32_t) (style * 7919 + variation * 104729 + 17));
    std::mt19937 fillRng ((uint32_t) (style * 7919 + variation * 104729 + section * 131 + 5));
    int fillKind = 0;
    if (varied)
    {
        // gleiche Groove-Variante in allen Sections eines Styles
        s.percA = mutateMask (s.percA, grooveRng);
        s.percB = mutateMask (s.percB, grooveRng);
        static const int pool[] = { -4, -2, 0, 2, 3, 5 };   // nur Perc-Noten (F1–D2), keine Kollision mit anderen Drums
        for (auto& v : s.percSemis) v = pool[grooveRng() % 6];
        s.accentSet = (int) (grooveRng() % 4);
        fillKind = 1 + (int) (fillRng() % 3);
    }
    const int rhythm = bassRhythm <= 0 ? s.bass : juce::jlimit (1, NumBassRhythms - 1, bassRhythm);

    Builder b;
    b.p.bars = 4;

    auto plain = [] (int, int, bool) { return 0; };
    // Oktavsprung auf der letzten Bassnote eines Taktes
    auto octaveEnd = [] (int beat, int, bool last) { return (beat == 3 && last) ? 12 : 0; };
    auto darkVar = [&s] (int beat, int, bool last) { return (s.darkNotes && beat == 3 && last) ? 1 : 0; };

    // Snare-Fill auf dem letzten Beat eines Taktes (16tel oder 32tel), Lautstärke steigend
    auto snareFill = [&b, fillKind] (int bar, bool thirtySeconds, float from, float to)
    {
        auto vel = [from, to] (int i, int n) { return from + (to - from) * (float) i / (float) (n - 1); };
        switch (fillKind)
        {
            case 1:   // 16tel-Triolen
                for (int i = 0; i < 6; ++i) b.add (Snare, bar, 3 * PPQ + i * 16, vel (i, 6), 10);
                break;
            case 2:   // Snare/Clap im Wechsel
                for (int i = 0; i < 4; ++i) b.add (i % 2 ? Clap : Snare, bar, 3 * PPQ + i * 24, vel (i, 4));
                if (thirtySeconds) b.add (Snare, bar, 3 * PPQ + 84, to, 8);
                break;
            case 3:   // Tom-Lauf (Perc) abwärts + Snare
            {
                static const int semis[] = { 5, 3, 0, -4 };
                for (int i = 0; i < 4; ++i) b.add (Perc, bar, 3 * PPQ + i * 24, vel (i, 4), 12, semis[i]);
                b.add (Snare, bar, 3 * PPQ + 72, to);
                break;
            }
            default:
            {
                const int n = thirtySeconds ? 8 : 4, step = thirtySeconds ? 12 : 24;
                for (int i = 0; i < n; ++i)
                    b.add (Snare, bar, 3 * PPQ + i * step, vel (i, n), thirtySeconds ? 8 : 12);
                break;
            }
        }
    };
    // Riser bis exakt zum Loop-Ende
    auto riser = [&b] (int startBar, int startTick, float vel)
    {
        const int from = startBar * BarTicks + startTick;
        b.add (Riser, 0, from, vel, 4 * BarTicks - from);
    };

    switch (section)
    {
        case Intro:
            for (int bar = 0; bar < 4; ++bar)
            {
                kicks (b, bar);
                hats (b, s, bar, s.hats == HatFull16 ? HatOff16 : s.hats, 0.7f);
                if (bar >= 2)
                {
                    offbeatOpen (b, bar, 0.55f);
                    perc (b, s, bar, bar == 3, 0.65f);
                    bassBar (b, bar, rhythm, 0.55f, false, plain);
                }
            }
            riser (3, 0, 0.6f);
            b.add (Zap, 3, 3 * PPQ + 48, 0.6f);
            break;

        case Buildup:
        {
            for (int bar = 0; bar < 4; ++bar)
            {
                if (bar < 2) kicks (b, bar);
                if (bar == 2) kicks (b, bar, 0x1);
                if (bar < 3) hats (b, s, bar, HatOff16, 0.6f + 0.1f * (float) bar);
                if (bar < 2) bassBar (b, bar, rhythm, 0.8f, false, plain);
            }
            // Roll: Viertel → Achtel → 16tel → 32tel, Lautstärke exponentiell steigend,
            // die letzte Achtel bleibt frei (Spannungspause vor dem Drop)
            auto rollVel = [] (int absTick) { const float x = (float) absTick / (float) (4 * BarTicks); return 0.32f + 0.68f * std::pow (x, 1.6f); };
            const int stepPerBar[4] = { PPQ, 48, 24, 12 };
            for (int bar = 0; bar < 4; ++bar)
                for (int t = 0; t < BarTicks; t += stepPerBar[bar])
                {
                    if (bar == 3 && t >= BarTicks - 48) break;
                    const int abs = bar * BarTicks + t;
                    b.add (Snare, bar, t, rollVel (abs), bar == 3 ? 8 : 12);
                    if (bar == 3 && t % 24 == 0) b.add (Clap, bar, t, rollVel (abs) * 0.7f);
                }
            riser (2, 0, 1.0f);
            b.add (Zap, 0, 0, 0.7f);
            break;
        }

        case Drop:
            for (int bar = 0; bar < 4; ++bar)
            {
                kicks (b, bar);
                hats (b, s, bar, s.hats);
                offbeatOpen (b, bar);
                backbeat (b, bar, s.back, 0.85f, bar == 3);
                if (bar % 2 == 1) perc (b, s, bar, false, 0.8f);
                if (bar == 3) bassBar (b, bar, rhythm, 1.0f, true, octaveEnd);
                else          bassBar (b, bar, rhythm, 1.0f, false, darkVar);
            }
            snareFill (3, false, 0.45f, 0.8f);
            b.add (Zap, 0, 0, 1.0f);
            break;

        case Offbeat:
            for (int bar = 0; bar < 4; ++bar)
            {
                kicks (b, bar);
                hats (b, s, bar, HatOff16, 0.6f);
                offbeatOpen (b, bar, 0.9f);
                if (bar == 3) perc (b, s, bar, true, 0.7f);
                bassBar (b, bar, rhythm, 1.0f, bar == 3, plain);
            }
            snareFill (3, false, 0.4f, 0.75f);
            riser (3, 0, 0.55f);
            break;

        case FullEnergy:
            for (int bar = 0; bar < 4; ++bar)
            {
                kicks (b, bar);
                hats (b, s, bar, s.hats, 1.05f);
                offbeatOpen (b, bar, 0.8f);
                ride (b, bar, false);
                perc (b, s, bar, bar % 2 == 1);
                backbeat (b, bar, s.back, 0.85f, bar == 3);
                if (bar == 3) bassBar (b, bar, rhythm, 1.0f, true, octaveEnd);
                else          bassBar (b, bar, rhythm, 1.0f, false, darkVar);
            }
            snareFill (3, false, 0.5f, 0.9f);
            riser (3, 2 * PPQ, 0.5f);
            b.add (Zap, 0, 0, 0.9f);
            break;

        case Festival:
            for (int bar = 0; bar < 4; ++bar)
            {
                kicks (b, bar);
                hats (b, s, bar, HatFull16, 1.15f, true);
                offbeatOpen (b, bar, 0.85f);
                ride (b, bar, true, 1.1f);
                perc (b, s, bar, bar % 2 == 1, 1.1f);
                b.add (Zap, bar, 0, 0.85f);
                backbeat (b, bar, BackBoth, 0.95f, bar == 3);
                if (bar % 2 == 1 && bar != 3) b.add (Clap, bar, 3 * PPQ + 48, 0.6f);
                auto fest = [bar] (int beat, int idx, bool last)
                {
                    if (bar % 2 == 1 && beat == 3) return 12;
                    if (bar == 3 && beat == 2 && last) return 7;
                    juce::ignoreUnused (idx);
                    return 0;
                };
                bassBar (b, bar, rhythm, 1.0f, bar == 3, fest);
            }
            snareFill (3, true, 0.55f, 1.0f);
            riser (3, 0, 0.6f);
            break;

        case Breakdown:
            for (int bar = 0; bar < 4; ++bar)
            {
                hats (b, s, bar, HatOff8, 0.45f);
                if (bar >= 2) offbeatOpen (b, bar, 0.4f);
                if (bar % 2 == 0) perc (b, s, bar, true, 0.5f);
                b.add (Ride, bar, 0, 0.4f);
            }
            b.add (Zap, 0, 0, 0.8f);
            for (int i = 0; i < 8; ++i)  b.add (Snare, 3, i * 24, 0.25f + 0.03f * (float) i);
            for (int i = 0; i < 16; ++i) b.add (Snare, 3, 2 * PPQ + i * 12, 0.5f + 0.03f * (float) i, 8);
            riser (2, 0, 0.9f);
            break;

        case Drop2:
        {
            const int altHats = s.hats == HatFull16 ? HatOff16 : HatFull16;
            for (int bar = 0; bar < 4; ++bar)
            {
                kicks (b, bar);
                hats (b, s, bar, altHats, 1.0f);
                offbeatOpen (b, bar, 0.8f);
                ride (b, bar, false, 0.8f);
                backbeat (b, bar, BackBoth, 0.85f, bar == 3);
                perc (b, s, bar, true, 0.9f);
                auto melo = [bar] (int beat, int idx, bool)
                {
                    static const int line[] = { 12, 10, 7, 5, 3, 7 };
                    if (bar % 2 == 1 && beat == 3) return line[idx % 6];
                    return 0;
                };
                bassBar (b, bar, rhythm, 1.0f, bar == 3, melo);
            }
            snareFill (3, true, 0.5f, 0.95f);
            b.add (Clap, 3, 3 * PPQ, 0.7f);
            b.add (Zap, 0, 0, 1.0f);
            b.add (Zap, 2, 0, 0.7f);
            break;
        }

        case End:
        default:
            for (int bar = 0; bar < 4; ++bar)
            {
                if (bar < 3) kicks (b, bar);
                else         kicks (b, bar, 0x3);
                if (bar < 3) hats (b, s, bar, s.hats, 0.9f - 0.2f * (float) bar);
                if (bar < 2)
                {
                    offbeatOpen (b, bar, 0.6f);
                    bassBar (b, bar, rhythm, 1.0f - 0.15f * (float) bar, false, plain);
                }
            }
            b.add (Zap, 3, 2 * PPQ, 0.5f);
            break;
    }

    // Variation: Ghost-Snares und Open-Hat-Auftakt in den vollen Sections
    if (varied && (section == Drop || section == FullEnergy || section == Festival || section == Drop2))
    {
        static const int ghostSteps[] = { 5, 7, 9, 11 };
        for (int bar : { 1, 3 })
            if (fillRng() % 3 != 0)
                b.add (Snare, bar, ghostSteps[fillRng() % 4] * 24, 0.2f + 0.05f * (float) (fillRng() % 3), 8);
        if (fillRng() % 2 == 0)
            b.add (OHat, 1, 3 * PPQ + 72, 0.5f);
    }

    return b.finish();
}

Pattern makeArrangement (int style, int bassRhythm, int variation)
{
    // typischer Psy-Ablauf, je 4 Takte pro Block
    const std::pair<int, int> plan[] = {
        { Intro, 4 }, { Offbeat, 2 }, { Buildup, 1 }, { Drop, 4 }, { FullEnergy, 4 },
        { Breakdown, 4 }, { Buildup, 1 }, { Drop2, 4 }, { Festival, 4 }, { End, 2 }
    };

    Pattern out; out.bars = 0;
    for (auto [section, repeats] : plan)
    {
        const auto p = makePattern (style, section, bassRhythm, variation);
        for (int r = 0; r < repeats; ++r)
        {
            const int offset = out.bars * BarTicks;
            for (auto e : p.events) { e.tick += offset; out.events.push_back (e); }
            out.bars += p.bars;
        }
    }
    return out;
}

//==============================================================================
int noteForEvent (const Event& e, int key)
{
    if (e.voice == Bass) return BassBasePitch + key + e.note;
    if (e.voice == Perc) return NotePerc + e.note;
    return gmNote (e.voice);
}

//==============================================================================
namespace
{
inline uint32_t hash32 (uint32_t x) noexcept
{
    x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16;
    return x;
}
inline float rand01 (uint32_t seed) noexcept { return (float) (hash32 (seed) & 0xffffff) / 16777216.0f; }

bool swingable (int voice)  { return voice == CHat || voice == Ride || voice == Perc || voice == Snare || voice == Clap; }
bool humanizable (const Event& e)
{
    switch (e.voice)
    {
        case CHat: case OHat: case Ride: case Perc: return true;
        case Snare: case Clap: return e.tick % PPQ != 0;   // Backbeat auf dem Beat bleibt stabil
        default: return false;                              // Kick, Bass, Zap, Riser fest
    }
}
constexpr double kMaxSwingTicks = 12.0;   // Swing 1.0 = halbe 16tel
constexpr double kMaxHumanMs = 6.0;
}

double grooveTick (const Pattern& p, size_t index, long long loop, const Groove& g, float& vel)
{
    const auto& e = p.events[index];
    double t = (double) e.tick;

    // Swing: die geraden 16tel (2. und 4. im Beat) nach hinten
    if (g.swing > 0.0 && swingable (e.voice) && e.tick % 24 == 0 && ((e.tick / 24) % 2) == 1)
        t += juce::jlimit (0.0, 0.6, g.swing) * kMaxSwingTicks;

    if (g.humanize > 0.0 && humanizable (e))
    {
        const uint32_t seed = (uint32_t) index * 0x9E3779B1u ^ (uint32_t) (loop * 0x85EBCA77LL) ^ (uint32_t) e.tick;
        const double ms = g.humanize * kMaxHumanMs * (rand01 (seed) * 2.0 - 1.0);
        double dt = ms / 1000.0 * g.bpm / 60.0 * PPQ;
        if (e.tick < 3) dt = std::abs (dt);                // nie vor den Loop-Start
        t += dt;
        vel = juce::jlimit (0.05f, 1.0f, vel * (1.0f + (float) g.humanize * 0.2f * (rand01 (seed ^ 0xABCDu) * 2.0f - 1.0f)));
    }

    return juce::jlimit (0.0, (double) p.lengthTicks() - 1.0, t);
}

juce::MidiFile toMidiFile (const Pattern& p, int key, double bpm, int inst, const Groove& groove)
{
    constexpr double scale = (double) ExportPPQ / PPQ;
    Groove g = groove; g.bpm = bpm;
    const double loopEnd = (double) p.lengthTicks() * scale;

    auto header = [&] (juce::MidiMessageSequence& seq, const juce::String& name, bool withTempo)
    {
        if (withTempo)
        {
            auto tempo = juce::MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / juce::jmax (40.0, bpm)));
            tempo.setTimeStamp (0); seq.addEvent (tempo);
            auto ts = juce::MidiMessage::timeSignatureMetaEvent (4, 4);
            ts.setTimeStamp (0); seq.addEvent (ts);
        }
        auto nm = juce::MidiMessage::textMetaEvent (3, name);
        nm.setTimeStamp (0); seq.addEvent (nm);
    };
    auto notes = [&] (juce::MidiMessageSequence& seq, int only)
    {
        for (size_t i = 0; i < p.events.size(); ++i)
        {
            const auto& e = p.events[i];
            if (e.voice != only) continue;
            const int note = noteForEvent (e, key);
            if (note < 0 || note > 127) continue;
            float v = e.vel;
            const double on = std::round (grooveTick (p, i, 0, g, v) * scale);
            const double off = juce::jmin (loopEnd, on + juce::jmax (1.0, e.length * scale - 1.0));
            const auto vel = (juce::uint8) juce::jlimit (1, 127, (int) std::round (v * 127.0f));
            seq.addEvent (juce::MidiMessage::noteOn (1, note, vel), on);
            seq.addEvent (juce::MidiMessage::noteOff (1, note), off);
        }
    };
    auto finish = [&] (juce::MidiMessageSequence& seq)
    {
        auto end = juce::MidiMessage::endOfTrack();
        end.setTimeStamp (loopEnd);
        seq.addEvent (end);
        seq.sort();
        seq.updateMatchedPairs();
    };

    juce::MidiFile file;
    file.setTicksPerQuarterNote (ExportPPQ);
    if (inst >= 0)
    {
        juce::MidiMessageSequence seq;
        header (seq, juce::String ("PsyForge ") + instrumentName (inst), true);
        notes (seq, inst);
        finish (seq);
        file.addTrack (seq);
        return file;
    }
    // alle Instrumente: Spur 1 = Tempo, danach eine Spur pro Instrument (Cubase legt dafür je eine MIDI-Spur an)
    {
        juce::MidiMessageSequence seq;
        header (seq, "PsyForge", true);
        finish (seq);
        file.addTrack (seq);
    }
    static const int order[] = { Kick, Bass, Snare, Clap, CHat, OHat, Ride, Perc, Zap, Riser };
    for (int v : order)
    {
        bool any = false;
        for (auto& e : p.events) if (e.voice == v) { any = true; break; }
        if (! any) continue;
        juce::MidiMessageSequence seq;
        header (seq, juce::String ("PsyForge ") + instrumentName (v), false);
        notes (seq, v);
        finish (seq);
        file.addTrack (seq);
    }
    return file;
}

juce::String checkLoop (const Pattern& p)
{
    const int L = p.lengthTicks();
    if (L % BarTicks != 0) return "Laenge ist kein ganzer Takt";
    // schlimmster Fall: voller Swing + volle Humanize bei langsamstem Tempo (60 BPM) → max. ca. 7,2 + 0,6 Ticks
    const double maxShift = 0.6 * kMaxSwingTicks + kMaxHumanMs / 1000.0 * 200.0 / 60.0 * PPQ;
    for (const auto& e : p.events)
    {
        if (e.tick < 0) return "Event vor Loop-Start";
        if (e.tick + e.length > L) return "Note ragt ueber das Loop-Ende (Voice " + juce::String (e.voice) + ", Tick " + juce::String (e.tick) + ")";
        if (e.voice != Riser && (double) e.tick + maxShift >= (double) L) return "Groove koennte Event ueber das Loop-Ende schieben";
    }
    return {};
}

} // namespace pf
