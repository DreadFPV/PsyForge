#pragma once
// PsyForge – Song-Patterns (aus Goadrums übernommen): 8 Stile, 9 Sections, Bass-Rhythmen, Variation, Groove
#include <vector>
#include <juce_audio_basics/juce_audio_basics.h>
#include "Synth.h"

namespace pf
{
constexpr int PPQ = 96;              // Ticks pro Viertel
constexpr int BarTicks = PPQ * 4;
constexpr int BassBasePitch = 24;    // C1 + Tonart
constexpr int ExportPPQ = 960;

struct Event
{
    int tick = 0, length = 12, voice = InKick;
    int note = 0;       // Perc: Halbtöne relativ zu A1; Bass: Halbtöne relativ zum Grundton
    float vel = 1.0f;
};

struct Pattern
{
    int bars = 4;
    std::vector<Event> events;   // nach Tick sortiert
    int lengthTicks() const { return bars * BarTicks; }
};

enum Section { Intro = 0, Buildup, Drop, Offbeat, FullEnergy, Festival, Breakdown, Drop2, End, NumSections };
constexpr int NumStyles = 8;
constexpr int NumBassRhythms = 8;     // 0 = Style-Standard

juce::StringArray styleNames();
juce::StringArray sectionNames();
juce::StringArray bassRhythmNames();
juce::StringArray keyNames();
double styleBpm (int style);
int styleDefaultBass (int style);   // 1..7

Pattern makePattern (int style, int section, int bassRhythm, int variation = 0);
Pattern makeArrangement (int style, int bassRhythm, int variation = 0);

// MIDI-Note eines Events (Drums: GM-Note, Perc: A1 + Halbtöne, Bass: echte Tonhöhe)
int noteForEvent (const Event&, int key);

struct Groove
{
    double swing = 0.0;      // 0..0.6
    double humanize = 0.0;   // 0..1  (1 = ±6 ms, ±20 % Velocity)
    double bpm = 145.0;
};
double grooveTick (const Pattern&, size_t eventIndex, long long loop, const Groove&, float& vel);

// inst >= 0: eine Spur mit diesem Instrument; inst = -1: alle Instrumente als eigene Spuren
juce::MidiFile toMidiFile (const Pattern&, int key, double bpm, int inst, const Groove& groove = {});

juce::String checkLoop (const Pattern&);
} // namespace pf
