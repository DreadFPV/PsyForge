#pragma once
// PsyForge – MIDI-Clip: hineingezogene MIDI-Datei, gefiltert auf das Instrument der Instanz
#include <juce_audio_basics/juce_audio_basics.h>
#include <map>
#include "Patterns.h"

namespace pf
{
struct ClipNote { double start = 0, length = 0.25; int note = 60; float vel = 1.0f; int track = 0; };

struct MidiClip
{
    juce::String name;
    double lengthBeats = 16.0;
    std::vector<ClipNote> notes;      // alle Noten der Datei (Beats ab Clip-Start)
    juce::StringArray trackNames;     // pro Spur (leer = "Spur n")
    std::vector<int> trackCounts;
    std::map<int, int> noteCounts;    // Note -> Anzahl
};

// Auswahl, welche Noten der Clip-Datei dieses Instrument spielt
enum ClipFilterKind { ClipAuto = 0, ClipAll = 1, ClipTrackBase = 100, ClipNoteBase = 1000 };

inline bool loadMidiClip (const juce::File& file, MidiClip& clip, juce::String& error)
{
    juce::FileInputStream in (file);
    if (! in.openedOk()) { error = "Datei kann nicht geöffnet werden."; return false; }
    juce::MidiFile mf;
    if (! mf.readFrom (in)) { error = "Keine gültige MIDI-Datei."; return false; }
    const int tpq = mf.getTimeFormat();
    if (tpq <= 0) { error = "SMPTE-Zeitformat wird nicht unterstützt."; return false; }

    MidiClip c;
    c.name = file.getFileNameWithoutExtension();
    double firstStart = 1e18, lastEnd = 0;
    for (int t = 0; t < mf.getNumTracks(); ++t)
    {
        juce::MidiMessageSequence seq (*mf.getTrack (t));
        seq.updateMatchedPairs();
        juce::String tname;
        int count = 0;
        for (int i = 0; i < seq.getNumEvents(); ++i)
        {
            const auto* ev = seq.getEventPointer (i);
            const auto& m = ev->message;
            if (m.isTrackNameEvent() && tname.isEmpty()) tname = m.getTextFromTextMetaEvent();
            if (! m.isNoteOn()) continue;
            ClipNote n;
            n.start = m.getTimeStamp() / tpq;
            const double off = ev->noteOffObject != nullptr ? ev->noteOffObject->message.getTimeStamp() / tpq : n.start + 0.25;
            n.length = std::max (1.0 / 64.0, off - n.start);
            n.note = m.getNoteNumber();
            n.vel = m.getFloatVelocity();
            n.track = t;
            c.notes.push_back (n);
            c.noteCounts[n.note]++;
            firstStart = std::min (firstStart, n.start);
            lastEnd = std::max (lastEnd, n.start + n.length);
            ++count;
        }
        c.trackNames.add (tname);
        c.trackCounts.push_back (count);
    }
    if (c.notes.empty()) { error = "Die Datei enthält keine Noten."; return false; }
    // Clip beginnt am Takt der ersten Note und ist ganze Takte lang
    const double barStart = std::floor (firstStart / 4.0) * 4.0;
    for (auto& n : c.notes) n.start -= barStart;
    c.lengthBeats = std::max (4.0, std::ceil ((lastEnd - barStart - 1e-6) / 4.0) * 4.0);
    std::sort (c.notes.begin(), c.notes.end(), [] (const ClipNote& a, const ClipNote& b) { return a.start < b.start; });
    clip = std::move (c);
    return true;
}

inline juce::StringArray instrumentKeywords (int inst)
{
    switch (inst)
    {
        case InKick:  return { "kick", "bd", "bassdrum" };
        case InBass:  return { "bass", "sub" };
        case InSnare: return { "snare", "sd" };
        case InClap:  return { "clap", "cp" };
        case InCHat:  return { "closed", "chh", "ch ", "hihat", "hi-hat", "hh", "hat" };
        case InOHat:  return { "open", "ohh", "oh " };
        case InRide:  return { "ride", "cymbal" };
        case InPerc:  return { "perc", "tom" };
        case InZap:   return { "zap", "laser" };
        case InRiser: return { "riser", "sweep", "fx" };
        default:      return {};
    }
}

// Automatische Auswahl: Spur mit passendem Namen, sonst passende GM-Noten, sonst alle Noten
inline int autoClipFilter (const MidiClip& c, int inst)
{
    const auto keys = instrumentKeywords (inst);
    for (const auto& k : keys)
        for (int t = 0; t < c.trackNames.size(); ++t)
        {
            if (c.trackCounts[(size_t) t] == 0) continue;
            const auto name = c.trackNames[t].toLowerCase() + " ";
            if (name.contains (k) && ! (inst == InCHat && (name.contains ("open") || name.contains ("ohh")))) return ClipTrackBase + t;
        }
    if (c.noteCounts.size() > 1 && inst != InBass)
        for (const auto& [note, count] : c.noteCounts)
            if (gmAccepts (inst, note)) return -2;   // GM-Filter
    return ClipAll;
}

inline bool clipNoteMatches (const MidiClip& c, const ClipNote& n, int inst, int filter)
{
    if (filter == ClipAuto) filter = autoClipFilter (c, inst);
    if (filter == ClipAll) return true;
    if (filter == -2) return gmAccepts (inst, n.note);
    if (filter >= ClipNoteBase) return n.note == filter - ClipNoteBase;
    if (filter >= ClipTrackBase) return n.track == filter - ClipTrackBase;
    return true;
}

// Note, die das Instrument im Export bekommt (Drums: GM-Note; Perc und Bass behalten ihre Tonhöhe)
inline int exportNoteFor (int inst, int note)
{
    if (inst == InBass) return note;
    if (inst == InPerc) return gmAccepts (InPerc, note) ? note : NotePerc;
    return gmNote (inst);
}

inline juce::MidiFile clipToMidiFile (const MidiClip& c, int inst, int filter, double bpm)
{
    juce::MidiMessageSequence seq;
    auto tempo = juce::MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / juce::jmax (40.0, bpm)));
    tempo.setTimeStamp (0); seq.addEvent (tempo);
    auto nm = juce::MidiMessage::textMetaEvent (3, juce::String ("PsyForge ") + instrumentName (inst));
    nm.setTimeStamp (0); seq.addEvent (nm);
    for (const auto& n : c.notes)
    {
        if (! clipNoteMatches (c, n, inst, filter)) continue;
        const int note = exportNoteFor (inst, n.note);
        const auto vel = (juce::uint8) juce::jlimit (1, 127, (int) std::round (n.vel * 127.0f));
        seq.addEvent (juce::MidiMessage::noteOn (1, note, vel), std::round (n.start * ExportPPQ));
        seq.addEvent (juce::MidiMessage::noteOff (1, note), std::round ((n.start + n.length) * ExportPPQ) - 1);
    }
    auto end = juce::MidiMessage::endOfTrack();
    end.setTimeStamp (c.lengthBeats * ExportPPQ);
    seq.addEvent (end);
    seq.sort(); seq.updateMatchedPairs();
    juce::MidiFile f;
    f.setTicksPerQuarterNote (ExportPPQ);
    f.addTrack (seq);
    return f;
}

inline juce::String clipFilterLabel (const MidiClip& c, int filter, int inst)
{
    if (filter == ClipAuto)
    {
        const int a = autoClipFilter (c, inst);
        return "Automatisch (" + (a == -2 ? juce::String ("Drum-Map") : clipFilterLabel (c, a, inst)) + ")";
    }
    if (filter == ClipAll) return "Alle Noten";
    if (filter >= ClipNoteBase)
    {
        const int n = filter - ClipNoteBase;
        return "Note " + juce::String (noteName (n)) + juce::String (noteOctave (n));
    }
    if (filter >= ClipTrackBase)
    {
        const int t = filter - ClipTrackBase;
        const auto nm = c.trackNames[t].isNotEmpty() ? c.trackNames[t] : "Spur " + juce::String (t + 1);
        return nm;
    }
    return {};
}
} // namespace pf
