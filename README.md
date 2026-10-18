# PsyForge

VST3/AU-Instrument (Windows & Mac) für **Goa / Psytrance / Full-On Kick und Rolling-Bass** – direkte Klangerzeugung per Synthese,
mit Waveform-Anzeige, WAV-Export und **WAV-Analyse**, die ein bestehendes One-Shot möglichst genau nachbaut.

Von JustCreateit.ch für Just DereadBeats.

## Installation

1. Unter **Actions** (oder **Releases**) das ZIP für dein System laden: `PsyForge-Windows.zip` oder `PsyForge-macOS.zip`.
2. `PsyForge.vst3` kopieren nach
   - Windows: `C:\Program Files\Common Files\VST3\`
   - Mac: `~/Library/Audio/Plug-Ins/VST3/` (AU: `PsyForge.component` nach `~/Library/Audio/Plug-Ins/Components/`)
3. In Cubase: Studio → VST-Plug-in-Manager → neu scannen. PsyForge erscheint als Instrument.

## In Cubase verwenden

- **MIDI-Kanal 1 = Kick**, **MIDI-Kanal 2 = Bass** (oben im Plugin änderbar).
  Die Kick reagiert auf jede Note, der Bass spielt die Notenhöhe (monophon, jede Note neu angeschlagen).
- Dein MIDI-File (z. B. aus Goadrums oder dem MIDI-Generator) einfach auf die Instrumentenspur legen – Kick-Noten auf Kanal 1, Bass-Noten auf Kanal 2.
- **Ausgänge:** Main (Stereo) + getrennte Ausgänge *Kick* und *Bass*. In Cubase im Instrument-Rack die Ausgänge aktivieren –
  sobald ein Einzelausgang aktiv ist, verschwindet der Klang aus dem Main-Ausgang.
- Das Bass-**Ducking** (Sidechain) passiert intern: jede Kick senkt den Bass kurz ab, auch bei getrennten Ausgängen.

## Bedienung

| Bereich | Was es macht |
|---|---|
| **Waveform** | zeigt die erzeugte Kick, Bassnote oder den Loop. Bei der Kick: Tonhöhenverlauf (blau, Hz rechts), Hüllkurve (gestrichelt), 1/16- und 1/8-Marken im Tempo. Mit der Maus darüberfahren zeigt Zeit, Hz und dB. **In die DAW ziehen = WAV-Export.** |
| **▶ Kick / ▶ Bass / ⟳ Loop** | Vorhören. Der Loop spielt das gewählte Muster (KBBB, Offbeat, Gallop, Triolen) – synchron zur DAW, wenn sie läuft. |
| **Kick-WAV / Bass-WAV / Loop-WAV** | als 24-Bit-WAV speichern (Name enthält Preset, Note und BPM). |
| **Tonart / A4 / 432** | Kick-Grundton rastet auf die Tonart ein (Snap). A4 frei einstellbar, 440/432 per Knopf. |
| **Presets** | Full-On, Progressive, Brazilian Bounce + eigene Presets (Ordner `Dokumente/PsyForge/Presets`). |

### Kick
- **Pitch:** Start (Click-Spitze) → Punch → Grundton, jeweils frei in Hz einstellbar; Click-Zeit und Sweep bestimmen, wie schnell gefallen wird. Drift senkt den Grundton über die Länge.
- **Lautstärke / Fishtail:** Attack, Hold, Decay, Länge (oder Sync auf 1/16, 1/8 …), Fishtail = typische Einschnürung nach dem Anschlag.
- **Click:** Rausch- oder Ton-Click mit eigener Frequenz und Länge. **Drive:** Tanh, Hart, Fold, Röhre, Transient-Schutz. **EQ:** Low-Cut, „200-Hz-Loch“ (Frequenz/Tiefe/Q frei), High-Cut.

### Bass
- Saw + Square (+ Unison) + Sinus-Sub (0/-1 Oktave, 0°/180°), Phasen-Reset pro Note.
- ADSR, 12/24-dB-Tiefpass mit Filter-Hüllkurve (der „Plock“), Keytrack, **Pluck** (Pitch-Sprung, Brazilian-Bounce), Drive,
  Ducking, Versatz (Phasen-Ausrichtung zur Kick), EQ mit **Kick-Notch** auf dem Kick-Grundton.

## WAV-Analyse (ANALYSE-Seite)

1. One-Shot laden oder einfach aufs Plugin ziehen (WAV, AIFF, FLAC) – **eine** Kick oder **eine** Bassnote, ohne Hall.
2. *Analysieren + Feinanpassung* (10–30 s): erkennt Kick oder Bass, misst Tonhöhenverlauf, Hüllkurve, Click, Sättigung und Spektrum,
   schätzt die Regler und passt sie danach automatisch an, bis der Klang möglichst nah dran ist. Die Übereinstimmung wird in % angezeigt.
3. **Übernehmen** lädt das Ergebnis in Kick bzw. Bass (Rückgängig möglich). Das Original liegt orange in der Waveform zum Vergleich, *▶ Original* spielt es ab.

Hinweise:
- Kick-Grundton wird exakt übernommen (Snap aus). Für deine Tonart Snap wieder einschalten.
- Bässe werden auf die nächste Note in deiner Stimmung gesetzt – bei 432-Hz-Samples vorher A4 auf 432 stellen.
- Kicks werden sehr genau getroffen; bei Bässen aus anderen Synths (andere Filter, Layer, Effekte) ist das Ergebnis ein guter Startpunkt zum Nachregeln.

## Selbst bauen

```bash
cmake -B build                        # JUCE 8.0.4 wird automatisch geladen (oder ./JUCE)
cmake --build build --config Release
```
GitHub Actions baut bei jedem Push für Windows und macOS; ein Tag `v*` erzeugt ein Release.

Tests ohne DAW (Linux/Mac):
```bash
cmake -B build -DPSYFORGE_BUILD_TEST=ON && cmake --build build --target psyforge_test PsyForgeUiTest
./build/psyforge_test render out/          # alle Presets als WAV
./build/psyforge_test roundtrip            # Preset rendern -> analysieren -> Übereinstimmung
./build/psyforge_test analyze kick.wav     # eigene Datei analysieren
```

## Aufbau

| Datei | Inhalt |
|---|---|
| `Source/Engine.h` | Klang-Engine (Kick, Bass, Ducking, Offline-Render), ohne JUCE |
| `Source/ParamTable.h` | alle Parameter: IDs, Bereiche, Hilfetexte |
| `Source/Presets.h` | Werks-Presets |
| `Source/Analysis.h` | WAV-Analyse und automatische Anpassung (Pattern-Search + CMA-ES) |
| `Source/PluginProcessor.*` | Plugin: MIDI-Kanäle, Ausgänge, Presets, Export, Analyse-Thread |
| `Source/PluginEditor.*` | Oberfläche |
