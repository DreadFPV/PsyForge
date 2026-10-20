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
        { "g_inst",   "Instrument", "global", 0, 9, 0, 0, "", KChoice, "Kick|Bass|Snare|Clap|Closed Hat|Open Hat|Ride|Perc|Zap|Riser", &S::inst,
          "Welches Instrument diese PsyForge-Instanz spielt. Pro Cubase-Spur eine Instanz mit eigenem Instrument." },
        { "g_midif",  "Noten",    "global", 0, 1, 0, 0, "", KChoice, "Alle Noten|Nur Drum-Map", &S::midiFilter,
          "Alle Noten: jede Note auf der Spur spielt das Instrument. Nur Drum-Map: nur die passende GM-Note (z. B. D1 = Snare) – praktisch, wenn mehrere Instrumente auf einer MIDI-Spur liegen." },
        { "g_note",   "Vorschau-Note", "global", 12, 72, 30, 0, "", KInt, "", &S::note,
          "Bassnote für Vorschau, Anzeige und WAV-Export." },
        { "g_bpm",    "Vorschau-BPM", "global", 90, 200, 145, 0, " BPM", KFloat, "", &S::bpm,
          "Tempo für Vorschau und Export, wenn die DAW nicht läuft." },
        { "g_style",  "Stil",     "global", 0, 7, 2, 0, "", KChoice, "Oldschool Goa|Neo-Goa|Full-On|Progressive|Psygressive|Darkpsy / Forest|Hi-Tech|Prog BR", &S::style,
          "Stil der Patterns (Rhythmus, Hats, Perc, Bass-Standard)." },
        { "g_section","Section",  "global", 0, 8, 2, 0, "", KChoice, "Intro|Buildup|Drop|Offbeat|Full Energy|Festival|Breakdown|Drop 2|End", &S::section,
          "Song-Teil des Patterns (je 4 Takte)." },
        { "g_bassrhy","Bass-Rhythmus", "global", 0, 7, 0, 0, "", KChoice,
          "Stil-Standard|Offbeat (K-B-)|Gallop (K-BB)|Rolling (KBBB)|Triplet (K-B-B)|Triplet 16 (Hi-Tech)|Bounce (BR)|Darkroll (KBBB+1/32)", &S::bassRhy,
          "Rhythmus der Bassline. Gilt für alle Instanzen gleich, wenn du überall denselben wählst." },
        { "g_var",    "Variation", "global", 0, 12, 0, 0, "", KInt, "", &S::variation,
          "Groove-Variante (0 = Original). Gleiche Zahl in allen Instanzen = passende Patterns." },
        { "g_swing",  "Swing",    "global", 0, 0.6f, 0, 0, "", KFloat, "", &S::swing, "Verschiebt die geraden 16tel (Hats, Ride, Perc, Fills)." },
        { "g_human",  "Humanize", "global", 0, 1, 0.3f, 0, "", KFloat, "", &S::humanize, "Leichte Timing- und Velocity-Schwankungen (nicht bei Kick/Bass)." },
        { "g_seq",    "Pattern spielen", "global", 0, 1, 0, 0, "", KBool, "", &S::seqOn,
          "An: PsyForge spielt das Pattern (oder den MIDI-Clip) selbst, synchron zu Cubase. Aus: nur die MIDI-Noten der Spur spielen." },
        { "g_source", "Quelle",   "global", 0, 1, 0, 0, "", KChoice, "Pattern|MIDI-Clip", &S::source,
          "Was 'Pattern spielen' und der MIDI-Export verwenden: das eingebaute Pattern oder den hineingezogenen MIDI-Clip." },

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
        { "b_ducksrc","Duck-Quelle","b_mod", 0, 3, 1, 0, "", KChoice, "Aus|Kick-Instanz|Viertel-Raster|Sidechain", &S::bDuckSrc,
          "Wer den Bass absenkt: Kick-Instanz = eine PsyForge-Kick im selben Projekt (automatisch verbunden), Viertel-Raster = auf jeder Zählzeit, Sidechain = Audio am Sidechain-Eingang." },

        // ---------------- Bass: EQ ----------------
        { "b_lowcut", "Low-Cut",   "b_eq", 10, 150, 30, 35, " Hz", KFloat, "", &S::bLowCut, "Hochpass (10 Hz = aus)." },
        { "b_notch",  "Kick-Notch","b_eq", 0, 24, 0, 0, " dB", KFloat, "", &S::bNotchDb,
          "Schmale Absenkung auf dem Kick-Grundton – Kick und Bass beissen sich nicht." },
        { "b_highcut","High-Cut",  "b_eq", 500, 20000, 9000, 5000, " Hz", KFloat, "", &S::bHighCut, "Tiefpass (20 kHz = aus)." },

        // ---------------- Snare ----------------
        { "s_level",  "Level",     "s_body", 0, 1, 0.8f, 0, "", KFloat, "", &S::sLevel, "Lautstärke der Snare." },
        { "s_tone1",  "Ton 1",     "s_body", 80, 600, 190, 220, " Hz", KFloat, "", &S::sTone1, "Grundton des Snare-Körpers." },
        { "s_tone2",  "Ton 2",     "s_body", 120, 1200, 330, 350, " Hz", KFloat, "", &S::sTone2, "Zweiter Ton (Fell-Oberton)." },
        { "s_mix",    "Ton-Mix",   "s_body", 0, 1, 0.35f, 0, "", KFloat, "", &S::sToneMix, "Verhältnis Ton 1 / Ton 2." },
        { "s_body",   "Körper",    "s_body", 0, 1, 0.7f, 0, "", KFloat, "", &S::sBodyLevel, "Lautstärke des tonalen Körpers." },
        { "s_bdec",   "Körper-Decay","s_body", 10, 500, 70, 80, " ms", KFloat, "", &S::sBodyDecay, "Ausklingen des Körpers (bis -20 dB)." },
        { "s_pdrop",  "Pitch-Fall","s_body", 0, 24, 7, 0, " st", KFloat, "", &S::sPitchDrop, "Wie weit der Ton am Anfang höher liegt." },
        { "s_ptime",  "Pitch-Zeit","s_body", 0.5f, 80, 10, 10, " ms", KFloat, "", &S::sPitchTime, "Wie schnell der Ton fällt." },
        { "s_klock",  "Key-Lock",  "s_body", 0, 1, 0, 0, "", KBool, "", &S::sKeyLock, "Töne auf die Moll-Tonleiter der Tonart einrasten." },
        { "s_noise",  "Rauschen",  "s_noise", 0, 1.5f, 0.7f, 0, "", KFloat, "", &S::sNoiseLevel, "Lautstärke des Teppichs (Snare-Draht)." },
        { "s_nhp",    "Rausch-HP", "s_noise", 100, 10000, 1500, 1500, " Hz", KFloat, "", &S::sNoiseHp, "Hochpass des Rauschens." },
        { "s_nbp",    "Rausch-BP", "s_noise", 500, 14000, 5000, 4000, " Hz", KFloat, "", &S::sNoiseBp, "Mittenfrequenz der Rausch-Betonung." },
        { "s_nq",     "Rausch-Q",  "s_noise", 0.3f, 8, 0.8f, 1.2f, "", KFloat, "", &S::sNoiseQ, "Breite der Betonung." },
        { "s_natt",   "Rausch-Attack","s_noise", 0, 20, 0.5f, 2, " ms", KFloat, "", &S::sNoiseAttack, "Einschwingen des Rauschens." },
        { "s_ndec",   "Rausch-Decay","s_noise", 10, 800, 140, 150, " ms", KFloat, "", &S::sNoiseDecay, "Ausklingen des Rauschens." },
        { "s_snap",   "Snap",      "s_snap", 0, 1.5f, 0.3f, 0, "", KFloat, "", &S::sSnap, "Kurzer Anschlag-Click." },
        { "s_snaphz", "Snap-Freq", "s_snap", 800, 12000, 3000, 3000, " Hz", KFloat, "", &S::sSnapHz, "Frequenz des Clicks." },
        { "s_drive",  "Drive",     "s_snap", 0, 1, 0.3f, 0, "", KFloat, "", &S::sDrive, "Sättigung." },
        { "s_dtype",  "Drive-Typ", "s_snap", 0, 3, 0, 0, "", KChoice, driveTypes, &S::sDriveType, "Art der Sättigung." },
        { "s_lowcut", "Low-Cut",   "s_snap", 20, 600, 80, 100, " Hz", KFloat, "", &S::sLowCut, "Hochpass (20 Hz = aus)." },
        { "s_highcut","High-Cut",  "s_snap", 1000, 20000, 16000, 6000, " Hz", KFloat, "", &S::sHighCut, "Tiefpass (20 kHz = aus)." },
        { "s_vel",    "Velocity",  "s_snap", 0, 1, 0.5f, 0, "", KFloat, "", &S::sVelSens, "Wie stark die Anschlagstärke wirkt." },

        // ---------------- Clap ----------------
        { "c_level",  "Level",     "c_main", 0, 1, 0.8f, 0, "", KFloat, "", &S::cLevel, "Lautstärke des Claps." },
        { "c_bursts", "Hände",     "c_main", 1, 8, 3, 0, "", KFloat, "", &S::cBursts, "Anzahl der kurzen Schläge vor dem Hauptschlag." },
        { "c_space",  "Abstand",   "c_main", 2, 30, 10, 10, " ms", KFloat, "", &S::cSpacing, "Abstand zwischen den Schlägen." },
        { "c_jitter", "Streuung",  "c_main", 0, 1, 0.2f, 0, "", KFloat, "", &S::cJitter, "Unregelmässigkeit der Abstände." },
        { "c_bdec",   "Schlag-Decay","c_main", 0.5f, 20, 3.5f, 4, " ms", KFloat, "", &S::cBurstDecay, "Länge eines einzelnen Schlags." },
        { "c_tail",   "Hall-Level","c_main", 0, 1, 0.7f, 0, "", KFloat, "", &S::cTailLevel, "Lautstärke des Ausklangs." },
        { "c_tdec",   "Hall-Decay","c_main", 20, 1500, 180, 200, " ms", KFloat, "", &S::cTailDecay, "Länge des Ausklangs." },
        { "c_bp",     "Ton",       "c_filter", 300, 8000, 1400, 1500, " Hz", KFloat, "", &S::cBpHz, "Mittenfrequenz (Klangfarbe)." },
        { "c_q",      "Q",         "c_filter", 0.3f, 6, 1.3f, 1.3f, "", KFloat, "", &S::cBpQ, "Breite des Bandpasses." },
        { "c_hp",     "Low-Cut",   "c_filter", 100, 4000, 600, 700, " Hz", KFloat, "", &S::cHpHz, "Hochpass." },
        { "c_lp",     "High-Cut",  "c_filter", 2000, 20000, 12000, 8000, " Hz", KFloat, "", &S::cLpHz, "Tiefpass." },
        { "c_drive",  "Drive",     "c_filter", 0, 1, 0.2f, 0, "", KFloat, "", &S::cDrive, "Sättigung." },
        { "c_vel",    "Velocity",  "c_filter", 0, 1, 0.5f, 0, "", KFloat, "", &S::cVelSens, "Wie stark die Anschlagstärke wirkt." },

        // ---------------- Closed Hat ----------------
        { "h_level", "Level", "h_tone", 0, 1, 0.6f, 0, "", KFloat, "", &S::hLevel, "Lautstärke." },
        { "h_freq",  "Grundton", "h_tone", 100, 1200, 360, 400, " Hz", KFloat, "", &S::hFreq, "Tonhöhe der 6 Metall-Oszillatoren." },
        { "h_inh",   "Metall",   "h_tone", 0, 1, 0.5f, 0, "", KFloat, "", &S::hInharm, "Wie unharmonisch (blechern) die Obertöne liegen." },
        { "h_noise", "Rauschen", "h_tone", 0, 1, 0.35f, 0, "", KFloat, "", &S::hNoise, "Anteil Rauschen gegenüber den Oszillatoren." },
        { "h_bell",  "Glocke",   "h_tone", 0, 1, 0.0f, 0, "", KFloat, "", &S::hBell, "Tonaler Glocken-Anteil (Ride-Bell)." },
        { "h_bellhz","Glocken-Ton","h_tone", 300, 4000, 800, 1000, " Hz", KFloat, "", &S::hBellHz, "Tonhöhe der Glocke." },
        { "h_bp",    "Farbe",    "h_filter", 1000, 16000, 9000, 7000, " Hz", KFloat, "", &S::hBpHz, "Mittenfrequenz der Betonung." },
        { "h_q",     "Q",        "h_filter", 0.3f, 6, 0.8f, 1.0f, "", KFloat, "", &S::hBpQ, "Breite der Betonung." },
        { "h_hp",    "Low-Cut",  "h_filter", 500, 14000, 7000, 5000, " Hz", KFloat, "", &S::hHpHz, "Hochpass." },
        { "h_hc",    "High-Cut", "h_filter", 3000, 20000, 18000, 12000, " Hz", KFloat, "", &S::hHighCut, "Tiefpass (20 kHz = aus)." },
        { "h_att",   "Attack",   "h_amp", 0, 30, 0.3f, 3, " ms", KFloat, "", &S::hAttack, "Einschwingzeit." },
        { "h_dec",   "Decay",    "h_amp", 5, 400, 45, 60, " ms", KFloat, "", &S::hDecay, "Ausklingen (bis -20 dB)." },
        { "h_drive", "Drive",    "h_amp", 0, 1, 0.1f, 0, "", KFloat, "", &S::hDrive, "Sättigung." },
        { "h_vel",   "Velocity", "h_amp", 0, 1, 0.6f, 0, "", KFloat, "", &S::hVelSens, "Wie stark die Anschlagstärke wirkt." },

        // ---------------- Open Hat ----------------
        { "o_level", "Level", "o_tone", 0, 1, 0.55f, 0, "", KFloat, "", &S::oLevel, "Lautstärke." },
        { "o_freq",  "Grundton", "o_tone", 100, 1200, 340, 400, " Hz", KFloat, "", &S::oFreq, "Tonhöhe der 6 Metall-Oszillatoren." },
        { "o_inh",   "Metall",   "o_tone", 0, 1, 0.5f, 0, "", KFloat, "", &S::oInharm, "Wie unharmonisch (blechern) die Obertöne liegen." },
        { "o_noise", "Rauschen", "o_tone", 0, 1, 0.35f, 0, "", KFloat, "", &S::oNoise, "Anteil Rauschen gegenüber den Oszillatoren." },
        { "o_bell",  "Glocke",   "o_tone", 0, 1, 0.0f, 0, "", KFloat, "", &S::oBell, "Tonaler Glocken-Anteil (Ride-Bell)." },
        { "o_bellhz","Glocken-Ton","o_tone", 300, 4000, 800, 1000, " Hz", KFloat, "", &S::oBellHz, "Tonhöhe der Glocke." },
        { "o_bp",    "Farbe",    "o_filter", 1000, 16000, 8500, 7000, " Hz", KFloat, "", &S::oBpHz, "Mittenfrequenz der Betonung." },
        { "o_q",     "Q",        "o_filter", 0.3f, 6, 0.8f, 1.0f, "", KFloat, "", &S::oBpQ, "Breite der Betonung." },
        { "o_hp",    "Low-Cut",  "o_filter", 500, 14000, 6000, 5000, " Hz", KFloat, "", &S::oHpHz, "Hochpass." },
        { "o_hc",    "High-Cut", "o_filter", 3000, 20000, 18000, 12000, " Hz", KFloat, "", &S::oHighCut, "Tiefpass (20 kHz = aus)." },
        { "o_att",   "Attack",   "o_amp", 0, 30, 0.5f, 3, " ms", KFloat, "", &S::oAttack, "Einschwingzeit." },
        { "o_dec",   "Decay",    "o_amp", 5, 1500, 260, 260, " ms", KFloat, "", &S::oDecay, "Ausklingen (bis -20 dB)." },
        { "o_drive", "Drive",    "o_amp", 0, 1, 0.1f, 0, "", KFloat, "", &S::oDrive, "Sättigung." },
        { "o_vel",   "Velocity", "o_amp", 0, 1, 0.5f, 0, "", KFloat, "", &S::oVelSens, "Wie stark die Anschlagstärke wirkt." },

        // ---------------- Ride ----------------
        { "r_level", "Level", "r_tone", 0, 1, 0.45f, 0, "", KFloat, "", &S::rLevel, "Lautstärke." },
        { "r_freq",  "Grundton", "r_tone", 100, 1200, 420, 400, " Hz", KFloat, "", &S::rFreq, "Tonhöhe der 6 Metall-Oszillatoren." },
        { "r_inh",   "Metall",   "r_tone", 0, 1, 0.6f, 0, "", KFloat, "", &S::rInharm, "Wie unharmonisch (blechern) die Obertöne liegen." },
        { "r_noise", "Rauschen", "r_tone", 0, 1, 0.25f, 0, "", KFloat, "", &S::rNoise, "Anteil Rauschen gegenüber den Oszillatoren." },
        { "r_bell",  "Glocke",   "r_tone", 0, 1, 0.35f, 0, "", KFloat, "", &S::rBell, "Tonaler Glocken-Anteil (Ride-Bell)." },
        { "r_bellhz","Glocken-Ton","r_tone", 300, 4000, 900, 1000, " Hz", KFloat, "", &S::rBellHz, "Tonhöhe der Glocke." },
        { "r_bp",    "Farbe",    "r_filter", 1000, 16000, 6000, 7000, " Hz", KFloat, "", &S::rBpHz, "Mittenfrequenz der Betonung." },
        { "r_q",     "Q",        "r_filter", 0.3f, 6, 0.7f, 1.0f, "", KFloat, "", &S::rBpQ, "Breite der Betonung." },
        { "r_hp",    "Low-Cut",  "r_filter", 500, 14000, 3500, 5000, " Hz", KFloat, "", &S::rHpHz, "Hochpass." },
        { "r_hc",    "High-Cut", "r_filter", 3000, 20000, 16000, 12000, " Hz", KFloat, "", &S::rHighCut, "Tiefpass (20 kHz = aus)." },
        { "r_att",   "Attack",   "r_amp", 0, 30, 1.0f, 3, " ms", KFloat, "", &S::rAttack, "Einschwingzeit." },
        { "r_dec",   "Decay",    "r_amp", 5, 3000, 700, 700, " ms", KFloat, "", &S::rDecay, "Ausklingen (bis -20 dB)." },
        { "r_drive", "Drive",    "r_amp", 0, 1, 0.05f, 0, "", KFloat, "", &S::rDrive, "Sättigung." },
        { "r_vel",   "Velocity", "r_amp", 0, 1, 0.5f, 0, "", KFloat, "", &S::rVelSens, "Wie stark die Anschlagstärke wirkt." },

        // ---------------- Perc ----------------
        { "p_level",  "Level",     "p_osc", 0, 1, 0.7f, 0, "", KFloat, "", &S::pLevel, "Lautstärke." },
        { "p_tune",   "Ton",       "p_osc", 40, 2000, 220, 250, " Hz", KFloat, "", &S::pTune, "Tonhöhe bei Note A1 (45). Andere Noten verschieben in Halbtönen." },
        { "p_wave",   "Form",      "p_osc", 0, 1, 0, 0, "", KFloat, "", &S::pWave, "Sinus -> Dreieck -> Rechteck." },
        { "p_fm",     "FM",        "p_osc", 0, 1, 0, 0, "", KFloat, "", &S::pFm, "Frequenzmodulation (metallisch/holzig)." },
        { "p_fmr",    "FM-Verh.",  "p_osc", 0.5f, 8, 1.5f, 2, "", KFloat, "", &S::pFmRatio, "Verhältnis des Modulators." },
        { "p_noise",  "Rauschen",  "p_osc", 0, 1, 0, 0, "", KFloat, "", &S::pNoise, "Rausch-Anschlag." },
        { "p_snap",   "Snap",      "p_osc", 0, 3, 3, 0, "", KChoice, "Aus|Grundton|Grundton+Quinte|Tonleiter", &S::pSnap, "Ton auf die Tonart einrasten." },
        { "p_penv",   "Pitch-Env", "p_env", 0, 36, 12, 0, " st", KFloat, "", &S::pPitchEnv, "Wie weit der Ton am Anfang höher liegt." },
        { "p_ptime",  "Pitch-Zeit","p_env", 0.5f, 150, 12, 15, " ms", KFloat, "", &S::pPitchTime, "Wie schnell der Ton fällt." },
        { "p_att",    "Attack",    "p_env", 0, 30, 0.2f, 3, " ms", KFloat, "", &S::pAttack, "Einschwingzeit." },
        { "p_dec",    "Decay",     "p_env", 5, 1500, 120, 150, " ms", KFloat, "", &S::pDecay, "Ausklingen (bis -20 dB)." },
        { "p_lp",     "Filter",    "p_env", 200, 16000, 8000, 3000, " Hz", KFloat, "", &S::pLpHz, "Tiefpass." },
        { "p_reso",   "Reso",      "p_env", 0, 1, 0.2f, 0, "", KFloat, "", &S::pReso, "Resonanz." },
        { "p_drive",  "Drive",     "p_env", 0, 1, 0.3f, 0, "", KFloat, "", &S::pDrive, "Sättigung." },
        { "p_vel",    "Velocity",  "p_env", 0, 1, 0.5f, 0, "", KFloat, "", &S::pVelSens, "Wie stark die Anschlagstärke wirkt." },

        // ---------------- Zap ----------------
        { "z_level",  "Level",     "z_main", 0, 1, 0.6f, 0, "", KFloat, "", &S::zLevel, "Lautstärke." },
        { "z_start",  "Start",     "z_main", 200, 16000, 3000, 2500, " Hz", KFloat, "", &S::zStartHz, "Tonhöhe am Anfang." },
        { "z_end",    "Ende",      "z_main", 15, 2000, 60, 120, " Hz", KFloat, "", &S::zEndHz, "Tonhöhe am Ende des Sweeps." },
        { "z_sweep",  "Sweep",     "z_main", 2, 3000, 250, 250, " ms", KFloat, "", &S::zSweep, "Dauer des Pitch-Sweeps." },
        { "z_curve",  "Kurve",     "z_main", 0, 1, 0.6f, 0, "", KFloat, "", &S::zCurve, "0 = gleichmässig, 1 = fällt sofort stark." },
        { "z_len",    "Länge",     "z_main", 10, 4000, 350, 400, " ms", KFloat, "", &S::zLength, "Gesamtlänge." },
        { "z_wave",   "Form",      "z_tone", 0, 1, 0, 0, "", KFloat, "", &S::zWave, "Sinus -> Sägezahn." },
        { "z_lp",     "Filter",    "z_tone", 1, 16, 4, 4, " x", KFloat, "", &S::zLpRatio, "Filter folgt der Tonhöhe (Vielfaches)." },
        { "z_reso",   "Reso",      "z_tone", 0, 1, 0.3f, 0, "", KFloat, "", &S::zReso, "Resonanz (Laser-Charakter)." },
        { "z_drive",  "Drive",     "z_tone", 0, 1, 0.2f, 0, "", KFloat, "", &S::zDrive, "Sättigung." },
        { "z_vel",    "Velocity",  "z_tone", 0, 1, 0.5f, 0, "", KFloat, "", &S::zVelSens, "Wie stark die Anschlagstärke wirkt." },

        // ---------------- Riser ----------------
        { "x_level",  "Level",     "x_main", 0, 1, 0.55f, 0, "", KFloat, "", &S::xLevel, "Lautstärke." },
        { "x_len",    "Länge",     "x_main", 0.5f, 32, 8, 0, " Beats", KFloat, "", &S::xLength, "Sweep-Länge bei Live-MIDI (im Pattern = Notenlänge)." },
        { "x_noise",  "Rauschen",  "x_main", 0, 1, 0.7f, 0, "", KFloat, "", &S::xNoise, "Anteil Rausch-Sweep." },
        { "x_tone",   "Ton",       "x_main", 0, 1, 0.4f, 0, "", KFloat, "", &S::xTone, "Anteil steigender Ton." },
        { "x_curve",  "Kurve",     "x_main", 0, 1, 0.5f, 0, "", KFloat, "", &S::xCurve, "Verlauf: früh oder spät ansteigend." },
        { "x_start",  "Start",     "x_tone", 30, 2000, 200, 300, " Hz", KFloat, "", &S::xStartHz, "Anfangsfrequenz." },
        { "x_end",    "Ende",      "x_tone", 500, 16000, 6000, 5000, " Hz", KFloat, "", &S::xEndHz, "Endfrequenz." },
        { "x_reso",   "Reso",      "x_tone", 0, 1, 0.4f, 0, "", KFloat, "", &S::xReso, "Resonanz des Rauschfilters." },
        { "x_rel",    "Release",   "x_tone", 10, 3000, 300, 400, " ms", KFloat, "", &S::xRelease, "Ausklingen nach dem Ende." },
        { "x_drive",  "Drive",     "x_tone", 0, 1, 0.1f, 0, "", KFloat, "", &S::xDrive, "Sättigung." },
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
