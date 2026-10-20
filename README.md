# PsyForge

VST3/AU-Instrument (Windows & Mac) für **Goa / Psytrance / Full-On** – alle Drums und der Bass per Synthese,
dazu die Song-Patterns aus Goadrums, MIDI-Drag & Drop, MIDI-Clip-Import und eine **WAV-Analyse**, die ein bestehendes
One-Shot möglichst genau nachbaut.

Von JustCreateit.ch für Just DereadBeats. PsyForge ersetzt Goadrums.

## Das Prinzip: eine Instanz pro Instrument

Jede PsyForge-Instanz spielt **ein** Instrument: Kick, Bass, Snare, Clap, Closed Hat, Open Hat, Ride, Perc, Zap oder Riser.
Du legst in Cubase pro Instrument eine Instrumentenspur mit PsyForge an und wählst oben *Instrument dieser Spur*.
So hat jedes Instrument seine eigene Spur, seinen eigenen Mixer-Kanal und seine eigenen MIDI-Noten.

## Schnellstart in Cubase

1. Instrumentenspur mit **PsyForge** anlegen, oben **Instrument** wählen (z. B. Kick).
2. Rechts **Stil** und **Section** wählen und den Chip **⇩ MIDI: Section** auf die Spur ziehen.
3. Play – PsyForge spielt **alle Noten seiner Spur** (Kanal und Tonhöhe egal; beim Bass bestimmt die Note den Ton).
4. Für den Bass eine zweite Spur mit PsyForge → *Bass*. Das Ducking hört automatisch auf die Kick-Instanz.

Alle Instrumente auf einmal: Chip **⇩ Alle Instr.: Section** (oder *Full Track*) ins Projekt ziehen. Cubase legt pro Instrument
eine MIDI-Spur an (Kick, Bass, Snare …). Diese Spuren auf die passenden PsyForge-Instanzen leiten.

Liegen mehrere Instrumente auf **einer** MIDI-Spur (z. B. eine fremde Drum-MIDI), oben **Noten: Nur Drum-Map** wählen –
dann spielt die Instanz nur ihre GM-Note (C1 Kick, D1 Snare, D#1 Clap, F#1 Closed Hat, A#1 Open Hat, D#2 Ride, F1–D2 Perc, C#2 Zap, F#2 Riser).

## Bedienung

| Bereich | Was es macht |
|---|---|
| **Instrument dieser Spur** | Welches Instrument die Instanz spielt. Sound, Presets, Patterns und Analyse richten sich danach. |
| **Waveform** | Ein Anschlag mit Tonhöhe (blau, Hz rechts), Hüllkurve (gestrichelt), 1/16- und 1/8-Marken. In die DAW ziehen = WAV. |
| **Pattern & MIDI** | Stil (8), Section (9), Bass-Rhythmus (7), Variation (12 + Zufall), Swing, Humanize. Die Notenleiste zeigt, was diese Instanz spielt. |
| **Pattern spielen** | An: PsyForge spielt das Pattern bzw. den Clip selbst, synchron zu Cubase (zusätzlich zur Spur). |
| **▶ Anschlag / ⟳ Vorhören** | Einen Schlag hören / Pattern im Loop hören, auch ohne Play in Cubase. |
| **Chips** | Ziehen = in Cubase ablegen, Klicken = speichern unter. MIDI: Section, Full Track, alle Instrumente; WAV: Anschlag, Loop. |
| **Alle Sections …** | Alle 9 Sections dieses Instruments als einzelne MIDI-Dateien in einen Ordner. |
| **Tonart / A4 / 432** | Kick-Snap, Bass-Notch, Perc- und Snare-Key-Lock folgen der Tonart. A4 frei, 440/432 per Knopf. |
| **Presets** | Werks-Presets pro Instrument (Full-On, Progressive, Brazilian Bounce, Goa, Dark/Hi-Tech) + eigene (`Dokumente/PsyForge/Presets/<Instrument>`). |

### Tabs
- **SOUND** – alle Regler des gewählten Instruments (Experten-Ansicht).
- **MIDI-CLIP** – eine MIDI-Datei hineinziehen (z. B. aus dem MIDI-Generator oder Midizer). *Noten* wählt, was die Instanz davon spielt:
  automatisch (Spurname oder Drum-Map), alle Noten, eine Spur oder eine Note. Mit *Quelle = MIDI-Clip* spielen „Pattern spielen“
  und „Vorhören“ den Clip im Loop. Der Chip exportiert die gefilterten Noten auf der richtigen Note. Der Clip wird im Projekt gespeichert.
- **ANALYSE** – ein One-Shot des Instruments laden, *Analysieren + Feinanpassung* (10–45 s), *Übernehmen*. Original orange in der Waveform,
  *▶ Original* zum Vergleichen.

### Bass-Ducking (Duck-Quelle)
- **Kick-Instanz** (Standard): Jede PsyForge-Kick im selben Projekt meldet ihre Schläge, der Bass duckt darauf. Läuft keine Kick-Instanz, duckt er auf jeder Zählzeit.
- **Viertel-Raster**: auf jeder Zählzeit.
- **Sidechain**: Audio am Sidechain-Eingang (in Cubase am Instrument aktivieren).

## Instrumente

| Instrument | Klangerzeugung |
|---|---|
| Kick | Sinus mit 3-stufigem Pitch-Sweep (Start → Punch → Grundton), Fishtail-Hüllkurve, Click, Knock, Drive, EQ mit 200-Hz-Loch, 1/8-Sync |
| Bass | Saw/Square/Unison + Sub, 12/24-dB-Filter mit Hüllkurve, Pluck, Drive, Ducking, Kick-Notch |
| Snare | 2 Töne mit Pitch-Fall + Rauschteppich (HP/BP) + Snap, Drive, EQ, Key-Lock |
| Clap | 1–8 Schläge mit Abstand/Streuung + Hall-Ausklang, Bandpass, Drive |
| Closed/Open Hat, Ride | 6 Metall-Oszillatoren + Rauschen, Bandpass/Hochpass, Glocke (Ride-Bell), Drive |
| Perc | Sinus→Dreieck→Rechteck, FM, Pitch-Hüllkurve, Filter, Note A1 = Grundton, Snap auf die Tonart |
| Zap | Pitch-Sweep mit Kurve, Filter folgt der Tonhöhe, Resonanz |
| Riser | Rausch-Sweep + steigender Ton, Länge = Notenlänge (bzw. Regler bei Live-MIDI) |

## Installation

1. Unter **Actions** (letzter Build → Artifacts) oder **Releases** das ZIP laden: `PsyForge-Windows.zip` oder `PsyForge-macOS.zip`.
2. `PsyForge.vst3` kopieren nach
   - Windows: `C:\Program Files\Common Files\VST3\`
   - Mac: `~/Library/Audio/Plug-Ins/VST3/` (AU: `PsyForge.component` nach `~/Library/Audio/Plug-Ins/Components/`)
   - Mac einmalig freigeben: `xattr -cr ~/Library/Audio/Plug-Ins/VST3/PsyForge.vst3`
3. In Cubase: Studio → VST-Plug-in-Manager → neu scannen.

Hinweis: Projekte mit PsyForge 0.1 laden, aber Kick und Bass liegen jetzt in getrennten Instanzen.

## Selbst bauen

```bash
cmake -B build                        # JUCE 8.0.4 wird automatisch geladen (oder ./JUCE)
cmake --build build --config Release
```

Tests ohne DAW:
```bash
cmake -B build -DPSYFORGE_BUILD_TEST=ON && cmake --build build --target psyforge_test PsyForgeUiTest
./build/psyforge_test_artefacts/Release/psyforge_test render out/        # alle Presets als WAV
./build/psyforge_test_artefacts/Release/psyforge_test patterns           # Loop-Grenzen aller Patterns
./build/psyforge_test_artefacts/Release/psyforge_test roundtrip          # Analyse-Test für alle Instrumente
./build/psyforge_test_artefacts/Release/psyforge_test analyze 2 snare.wav
```

## Aufbau

| Datei | Inhalt |
|---|---|
| `Source/Engine.h` | Kick und Bass, Filter, Hilfsfunktionen |
| `Source/Drums.h` | Snare, Clap, Hats/Ride, Perc, Zap, Riser |
| `Source/Synth.h` | ein Instrument pro Instanz, Drum-Map, Render-Funktionen |
| `Source/ParamTable.h` | alle Parameter (IDs, Bereiche, Hilfetexte) |
| `Source/Presets.h` | Werks-Presets pro Instrument |
| `Source/Patterns.*` | Song-Patterns aus Goadrums, MIDI-Export |
| `Source/Clip.h` | MIDI-Clip-Import und -Filter |
| `Source/Analysis.h` | WAV-Analyse und automatische Anpassung (Pattern-Search + CMA-ES) |
| `Source/PluginProcessor.*` | Plugin: Spur-MIDI, Pattern-Wiedergabe, Ducking-Link, Export, Analyse-Thread |
| `Source/PluginEditor.*` | Oberfläche |
