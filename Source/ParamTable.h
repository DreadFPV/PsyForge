#pragma once
// PsyForge – zentrale Parameter-Tabelle (ohne JUCE): IDs, Bereiche, Hilfetexte, Zuordnung zu SynthParams
#include "Engine.h"
#include <string>
#include <cstring>

namespace pf
{
enum ParamKind { KFloat = 0, KChoice, KBool, KInt };

struct PDef
{
    const char* id;
    const char* name;
    const char* group;     // "global", "k_pitch", "k_amp", "k_click", "k_drive", "k_eq", "b_osc", "b_amp", "b_filter", "b_mod", "b_eq"
    float min, max, def;
    float centre;          // Skew-Mittelpunkt, 0 = linear
    const char* suffix;
    int kind;
    const char* choices;   // "A|B|C"
    float SynthParams::* field;
    const char* help;
};

inline const std::vector<PDef>& paramTable()
{
    using S = SynthParams;
    static const char* driveTypes = "Tanh|Hart|Fold|Röhre";
    static const std::vector<PDef> t = {
        // ---------------- Global ----------------
        { "g_key",    "Tonart",   "global", 0, 11, 6, 0, "", KChoice, "C|C#|D|D#|E|F|F#|G|G#|A|A#|B", &S::key,
          "Tonart für das Einrasten des Kick-Grundtons (Snap) und den Bass-Notch." },
        { "g_a4",     "A4",       "global", 415, 466, 440, 0, " Hz", KFloat, "", &S::a4,
          "Kammerton. 440 Hz Standard, 432 Hz für die 432-Stimmung." },
        { "g_master", "Master",   "global", 0, 1, 0.8f, 0, "", KFloat, "", &S::master, "Gesamtlautstärke." },
        { "g_limit",  "Limiter",  "global", 0, 1, 1, 0, "", KBool, "", &S::limiter,
          "Weicher Limiter auf dem Main-Ausgang (verhindert Übersteuern)." },
        { "g_kickch", "Kick-Kanal", "global", 1, 16, 1, 0, "", KInt, "", &S::kickCh,
          "MIDI-Kanal, auf dem die Kick gespielt wird (jede Note löst die Kick aus)." },
        { "g_bassch", "Bass-Kanal", "global", 1, 16, 2, 0, "", KInt, "", &S::bassCh,
          "MIDI-Kanal für den Bass (die Notenhöhe bestimmt den Ton, monophon)." },
        { "g_note",   "Vorschau-Note", "global", 12, 60, 30, 0, "", KInt, "", &S::note,
          "Bassnote für Vorschau, Anzeige und WAV-Export." },
        { "g_bpm",    "Vorschau-BPM", "global", 90, 200, 145, 0, " BPM", KFloat, "", &S::bpm,
          "Tempo für Vorschau und Export, wenn die DAW nicht läuft." },
        { "g_pattern","Muster",   "global", 0, 3, 0, 0, "", KChoice, "KBBB Rolling|Offbeat|Gallop (K-BB)|Triolen", &S::pattern,
          "Muster der Vorschau-Schleife und des Loop-Exports." },

        // ---------------- Kick: Pitch ----------------
        { "k_level",  "Level",     "k_pitch", 0, 1, 0.9f, 0, "", KFloat, "", &S::kLevel, "Lautstärke der Kick." },
        { "k_start",  "Start",     "k_pitch", 200, 12000, 2600, 2500, " Hz", KFloat, "", &S::kStartHz,
          "Tonhöhe ganz am Anfang (Click-Spitze des Sweeps)." },
        { "k_punch",  "Punch",     "k_pitch", 60, 1200, 190, 220, " Hz", KFloat, "", &S::kPunchHz,
          "Tonhöhe nach dem Click – hier sitzt der Punch (typisch 150–300 Hz)." },
        { "k_end",    "Grundton",  "k_pitch", 20, 200, 48, 55, " Hz", KFloat, "", &S::kEndHz,
          "Grundton, auf dem der Sweep endet. Mit Snap auf die Tonart eingerastet." },
        { "k_clickt", "Click-Zeit","k_pitch", 0.05f, 10, 1.2f, 1.5f, " ms", KFloat, "", &S::kClickTime,
          "Wie schnell Start -> Punch fällt (Zeitkonstante)." },
        { "k_sweep",  "Sweep",     "k_pitch", 1, 150, 16, 20, " ms", KFloat, "", &S::kSweepTime,
          "Wie schnell Punch -> Grundton fällt. Kurz = knackig, lang = 'Boing'." },
        { "k_drift",  "Drift",     "k_pitch", 0, 12, 1, 0, " st", KFloat, "", &S::kDrift,
          "Weiteres Absinken des Grundtons über die Länge (Halbtöne)." },
        { "k_snap",   "Snap",      "k_pitch", 0, 3, 1, 0, "", KChoice, "Aus|Grundton|Grundton+Quinte|Tonleiter", &S::kSnap,
          "Grundton auf die Tonart einrasten." },
        { "k_shape",  "Form",      "k_pitch", 0, 1, 0, 0, "", KFloat, "", &S::kShape,
          "Wellenform: Sinus -> Dreieck -> gesättigter Sinus." },
        { "k_phase",  "Start-Phase","k_pitch", 0, 1, 0, 0, "", KFloat, "", &S::kStartPhase,
          "Phase beim Start (0 = Nulldurchgang, 0,25 = harter Anschlag)." },
        { "k_knock",  "Knock",     "k_pitch", 0, 1, 0.25f, 0, "", KFloat, "", &S::kKnock,
          "2. Oberton im Anschlag – Punch auch auf kleinen Boxen." },
        { "k_follow", "Note folgen","k_pitch", 0, 1, 0, 0, "", KBool, "", &S::kFollow,
          "Grundton folgt der gespielten MIDI-Note (statt fester Frequenz)." },

        // ---------------- Kick: Amp ----------------
        { "k_attack", "Attack",    "k_amp", 0, 10, 0.1f, 1, " ms", KFloat, "", &S::kAttack, "Einschwingzeit." },
        { "k_hold",   "Hold",      "k_amp", 0, 80, 6, 15, " ms", KFloat, "", &S::kHold, "Volle Lautstärke vor dem Abklingen." },
        { "k_decay",  "Decay",     "k_amp", 10, 800, 110, 120, " ms", KFloat, "", &S::kDecay,
          "Zeit bis -20 dB nach dem Hold." },
        { "k_length", "Länge",     "k_amp", 30, 1500, 207, 250, " ms", KFloat, "", &S::kLength,
          "Gesamtlänge (wenn Sync = Frei)." },
        { "k_sync",   "Sync",      "k_amp", 0, 4, 2, 0, "", KChoice, "Frei|1/16|1/8|3/16|1/4", &S::kSync,
          "Länge an das Tempo koppeln. Psy-Kicks sind oft genau 1/8 lang." },
        { "k_fade",   "Ausblenden","k_amp", 0.5f, 100, 16, 15, " ms", KFloat, "", &S::kFade, "Sauberes Ende ohne Knackser." },
        { "k_fdepth", "Fishtail",  "k_amp", 0, 0.95f, 0.3f, 0, "", KFloat, "", &S::kFishDepth,
          "Tiefe der Einschnürung nach dem Anschlag (typische Psy-Fishtail-Form)." },
        { "k_fpos",   "Fish-Pos.", "k_amp", 1, 60, 12, 15, " ms", KFloat, "", &S::kFishPos, "Wo die Einschnürung liegt." },
        { "k_fwidth", "Fish-Breite","k_amp", 1, 60, 8, 12, " ms", KFloat, "", &S::kFishWidth, "Breite der Einschnürung." },
        { "k_vel",    "Velocity",  "k_amp", 0, 1, 0.3f, 0, "", KFloat, "", &S::kVelSens, "Wie stark die Anschlagstärke wirkt." },

        // ---------------- Kick: Click ----------------
        { "k_clvl",   "Click",     "k_click", 0, 1.5f, 0.35f, 0, "", KFloat, "", &S::kClickLevel, "Lautstärke des Click-Layers." },
        { "k_clhz",   "Click-Freq","k_click", 500, 16000, 4500, 4000, " Hz", KFloat, "", &S::kClickHz, "Frequenz des Clicks." },
        { "k_cldec",  "Click-Decay","k_click", 0.1f, 20, 2, 2, " ms", KFloat, "", &S::kClickDecay, "Länge des Clicks." },
        { "k_cltone", "Rauschen/Ton","k_click", 0, 1, 0.3f, 0, "", KFloat, "", &S::kClickTone,
          "0 = Rausch-Click, 1 = Ton-Click." },

        // ---------------- Kick: Drive ----------------
        { "k_drive",  "Drive",     "k_drive", 0, 1, 0.3f, 0, "", KFloat, "", &S::kDrive, "Sättigung – macht die Kick dichter und lauter." },
        { "k_dtype",  "Drive-Typ", "k_drive", 0, 3, 0, 0, "", KChoice, driveTypes, &S::kDriveType, "Art der Sättigung." },
        { "k_trans",  "Transient", "k_drive", 0, 1, 0.6f, 0, "", KFloat, "", &S::kTransient,
          "Anschlag vor der Sättigung schützen (bleibt knackig)." },

        // ---------------- Kick: EQ ----------------
        { "k_lowcut", "Low-Cut",   "k_eq", 10, 120, 20, 30, " Hz", KFloat, "", &S::kLowCut, "Hochpass (10 Hz = aus)." },
        { "k_holehz", "Loch-Freq", "k_eq", 60, 800, 200, 220, " Hz", KFloat, "", &S::kHoleHz,
          "Frequenz der EQ-Absenkung (klassisch ~200 Hz)." },
        { "k_holedb", "Loch-Tiefe","k_eq", 0, 24, 4, 0, " dB", KFloat, "", &S::kHoleDb, "Stärke der Absenkung." },
        { "k_holeq",  "Loch-Q",    "k_eq", 0.2f, 8, 1.2f, 1.2f, "", KFloat, "", &S::kHoleQ, "Breite der Absenkung." },
        { "k_highcut","High-Cut",  "k_eq", 1000, 20000, 16000, 6000, " Hz", KFloat, "", &S::kHighCut, "Tiefpass (20 kHz = aus)." },

        // ---------------- Bass: Oszillatoren ----------------
        { "b_level",  "Level",     "b_osc", 0, 1, 0.75f, 0, "", KFloat, "", &S::bLevel, "Lautstärke des Basses." },
        { "b_oct",    "Oktave",    "b_osc", 0, 4, 2, 0, "", KChoice, "-2|-1|0|+1|+2", &S::bOctave, "Oktav-Verschiebung." },
        { "b_fine",   "Fein",      "b_osc", -100, 100, 0, 0, " ct", KFloat, "", &S::bFine, "Feinstimmung in Cent." },
        { "b_saw",    "Saw",       "b_osc", 0, 1, 1, 0, "", KFloat, "", &S::bSaw, "Anteil Sägezahn (klassischer Psy-Bass)." },
        { "b_square", "Square",    "b_osc", 0, 1, 0, 0, "", KFloat, "", &S::bSquare, "Anteil Rechteck (hohler, runder)." },
        { "b_pw",     "Pulsbreite","b_osc", 0.05f, 0.95f, 0.5f, 0, "", KFloat, "", &S::bPw, "Pulsbreite des Rechtecks." },
        { "b_detune", "Unison",    "b_osc", 0, 50, 0, 0, " ct", KFloat, "", &S::bDetune, "Zweite Stimme verstimmt (0 = aus)." },
        { "b_sub",    "Sub",       "b_osc", 0, 1, 0.35f, 0, "", KFloat, "", &S::bSub, "Sinus-Sub (ungefiltert, sauberer Tiefbass)." },
        { "b_suboct", "Sub-Okt.",  "b_osc", 0, 1, 0, 0, "", KChoice, "0|-1", &S::bSubOct, "Sub auf Grundton oder eine Oktave tiefer." },
        { "b_subinv", "Sub-Phase", "b_osc", 0, 1, 0, 0, "", KChoice, "0°|180°", &S::bSubInv,
          "Phase des Subs zum Saw: 0° = verstärkt den Grundton, 180° = gegenphasig (hohler)." },
        { "b_retrig", "Phasen-Reset","b_osc", 0, 1, 1, 0, "", KBool, "", &S::bRetrig,
          "Jede Note startet an derselben Phase – jede Note klingt gleich (wichtig für Psy)." },
        { "b_phase",  "Start-Phase","b_osc", 0, 1, 0.5f, 0, "", KFloat, "", &S::bStartPhase, "Phase beim Notenstart (0,5 = Saw startet im Nulldurchgang, ohne Knacksen)." },

        // ---------------- Bass: Amp ----------------
        { "b_attack", "Attack",    "b_amp", 0, 30, 0.5f, 2, " ms", KFloat, "", &S::bAttack, "Einschwingzeit." },
        { "b_decay",  "Decay",     "b_amp", 5, 600, 75, 80, " ms", KFloat, "", &S::bDecay, "Zeit bis zum Sustain (-20 dB-Weg)." },
        { "b_sustain","Sustain",   "b_amp", 0, 1, 0.25f, 0, "", KFloat, "", &S::bSustain, "Haltepegel." },
        { "b_release","Release",   "b_amp", 1, 300, 12, 20, " ms", KFloat, "", &S::bRelease, "Ausklingen nach Note-Off." },
        { "b_vel",    "Velocity",  "b_amp", 0, 1, 0.4f, 0, "", KFloat, "", &S::bVelSens, "Wie stark die Anschlagstärke wirkt." },
        { "b_offset", "Versatz",   "b_amp", 0, 30, 0, 0, " ms", KFloat, "", &S::bOffset,
          "Bassnoten verzögern – Phasen-Ausrichtung zur Kick." },

        // ---------------- Bass: Filter ----------------
        { "b_cutoff", "Cutoff",    "b_filter", 30, 12000, 420, 600, " Hz", KFloat, "", &S::bCutoff, "Grund-Cutoff des Tiefpasses." },
        { "b_reso",   "Reso",      "b_filter", 0, 1, 0.2f, 0, "", KFloat, "", &S::bReso, "Resonanz." },
        { "b_fenv",   "Env-Menge", "b_filter", 0, 8, 3, 0, " Okt", KFloat, "", &S::bFenv,
          "Wie weit die Filter-Hüllkurve öffnet (Oktaven)." },
        { "b_fatt",   "Env-Attack","b_filter", 0, 20, 0, 2, " ms", KFloat, "", &S::bFattack, "Anstieg der Filter-Hüllkurve." },
        { "b_fdec",   "Env-Decay", "b_filter", 2, 500, 45, 50, " ms", KFloat, "", &S::bFdecay,
          "Wie schnell das Filter wieder schliesst – der 'Plock'." },
        { "b_key",    "Keytrack",  "b_filter", 0, 1, 0.5f, 0, "", KFloat, "", &S::bKeytrack, "Cutoff folgt der Tonhöhe." },
        { "b_slope",  "Flanke",    "b_filter", 0, 1, 1, 0, "", KChoice, "12 dB|24 dB", &S::bSlope, "Filtersteilheit." },

        // ---------------- Bass: Modulation / Drive / Ducking ----------------
        { "b_pluck",  "Pluck",     "b_mod", 0, 24, 0, 0, " st", KFloat, "", &S::bPluck,
          "Kurzer Pitch-Sprung von oben – macht den Bass 'bouncy' (Brazilian)." },
        { "b_pldec",  "Pluck-Zeit","b_mod", 0.5f, 80, 8, 8, " ms", KFloat, "", &S::bPluckDecay, "Dauer des Pitch-Sprungs." },
        { "b_drive",  "Drive",     "b_mod", 0, 1, 0.2f, 0, "", KFloat, "", &S::bDrive, "Sättigung des Mitten-Anteils (Sub bleibt sauber)." },
        { "b_dtype",  "Drive-Typ", "b_mod", 0, 3, 0, 0, "", KChoice, driveTypes, &S::bDriveType, "Art der Sättigung." },
        { "b_duck",   "Ducking",   "b_mod", 0, 1, 0.6f, 0, "", KFloat, "", &S::bDuck,
          "Bass wird bei jeder Kick abgesenkt (internes Sidechain)." },
        { "b_duckrel","Duck-Release","b_mod", 2, 250, 60, 50, " ms", KFloat, "", &S::bDuckRelease, "Erholzeit des Duckings." },

        // ---------------- Bass: EQ ----------------
        { "b_lowcut", "Low-Cut",   "b_eq", 10, 150, 30, 35, " Hz", KFloat, "", &S::bLowCut, "Hochpass (10 Hz = aus)." },
        { "b_notch",  "Kick-Notch","b_eq", 0, 24, 0, 0, " dB", KFloat, "", &S::bNotchDb,
          "Schmale Absenkung auf dem Kick-Grundton – Kick und Bass beissen sich nicht." },
        { "b_highcut","High-Cut",  "b_eq", 500, 20000, 9000, 5000, " Hz", KFloat, "", &S::bHighCut, "Tiefpass (20 kHz = aus)." },
    };
    return t;
}

inline const PDef* findParam (const std::string& id)
{
    for (auto& d : paramTable()) if (id == d.id) return &d;
    return nullptr;
}

inline SynthParams defaultParams()
{
    SynthParams p;
    for (auto& d : paramTable()) p.*(d.field) = d.def;
    return p;
}

// Normalisierung (gleiches Skew-Verhalten wie JUCE: centre liegt bei 0,5)
inline float skewOf (const PDef& d)
{
    if (d.centre <= d.min || d.centre >= d.max || d.kind != KFloat) return 1.0f;
    return (float) (std::log (0.5) / std::log ((d.centre - d.min) / (d.max - d.min)));
}
inline float toNorm (const PDef& d, float v)
{
    const float x = std::clamp ((v - d.min) / (d.max - d.min), 0.0f, 1.0f);
    const float s = skewOf (d);
    return std::abs (s - 1.0f) < 1e-6f ? x : std::pow (x, s);
}
inline float fromNorm (const PDef& d, float n)
{
    n = std::clamp (n, 0.0f, 1.0f);
    const float s = skewOf (d);
    const float x = std::abs (s - 1.0f) < 1e-6f ? n : std::pow (n, 1.0f / s);
    float v = d.min + (d.max - d.min) * x;
    if (d.kind != KFloat) v = std::round (v);
    return v;
}

inline std::vector<std::string> choiceList (const PDef& d)
{
    std::vector<std::string> out;
    std::string s = d.choices ? d.choices : "", cur;
    for (char c : s) { if (c == '|') { out.push_back (cur); cur.clear(); } else cur += c; }
    if (! s.empty()) out.push_back (cur);
    return out;
}
} // namespace pf
