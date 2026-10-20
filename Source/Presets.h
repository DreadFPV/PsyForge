#pragma once
// PsyForge – Werks-Presets pro Instrument
// Ein Preset setzt nur die Klangregler des eigenen Instruments (nicht gesetzte = Standard); alles andere bleibt.
#include "ParamTable.h"
#include "Drums.h"

namespace pf
{
struct FactoryPreset
{
    const char* name;
    const char* category;
    std::vector<std::pair<const char*, float>> values;
    int inst = -1;     // Instrument; -1 = Kick+Bass-Kombination (intern)
};

// Parameter, die ein Klang-Preset NICHT verändert
inline bool isSessionParam (const std::string& id) { return id.rfind ("g_", 0) == 0; }

inline bool belongsTo (const char* id, int inst) { return std::strncmp (id, instrumentPrefix (inst), 2) == 0; }

inline const std::vector<FactoryPreset>& kickBassCombos()
{
    static const std::vector<FactoryPreset> p = {
        { "Init", "Allround", {} },

        // ================= FULL-ON (145 BPM, rollende KBBB) =================
        { "FO Tight Rolling", "Full-On", {
            { "k_start", 3000 }, { "k_punch", 210 }, { "k_end", 48 }, { "k_clickt", 1.0f }, { "k_sweep", 14 }, { "k_drift", 0.8f },
            { "k_knock", 0.3f }, { "k_hold", 5 }, { "k_decay", 90 }, { "k_sync", 2 }, { "k_fade", 18 },
            { "k_fdepth", 0.35f }, { "k_fpos", 10 }, { "k_fwidth", 7 },
            { "k_clvl", 0.4f }, { "k_clhz", 5000 }, { "k_cldec", 1.5f }, { "k_cltone", 0.25f },
            { "k_drive", 0.35f }, { "k_dtype", 0 }, { "k_trans", 0.6f }, { "k_lowcut", 22 }, { "k_holehz", 200 }, { "k_holedb", 4 },
            { "b_saw", 1 }, { "b_square", 0 }, { "b_sub", 0.35f }, { "b_attack", 0.3f }, { "b_decay", 60 }, { "b_sustain", 0.15f },
            { "b_release", 10 }, { "b_cutoff", 380 }, { "b_reso", 0.25f }, { "b_fenv", 3.2f }, { "b_fdec", 38 }, { "b_key", 0.5f },
            { "b_drive", 0.25f }, { "b_duck", 0.5f }, { "b_duckrel", 50 }, { "b_lowcut", 32 }, { "b_notch", 3 }, { "b_highcut", 8000 } } },

        { "FO Festival Punch", "Full-On", {
            { "k_start", 3200 }, { "k_punch", 260 }, { "k_end", 50 }, { "k_clickt", 0.9f }, { "k_sweep", 18 }, { "k_drift", 1.0f },
            { "k_knock", 0.4f }, { "k_hold", 7 }, { "k_decay", 100 }, { "k_sync", 2 },
            { "k_fdepth", 0.4f }, { "k_fpos", 11 }, { "k_fwidth", 8 },
            { "k_clvl", 0.5f }, { "k_clhz", 5500 }, { "k_cldec", 1.6f }, { "k_cltone", 0.35f },
            { "k_drive", 0.5f }, { "k_dtype", 3 }, { "k_trans", 0.65f }, { "k_holedb", 5 },
            { "b_saw", 1 }, { "b_sub", 0.3f }, { "b_detune", 8 }, { "b_decay", 70 }, { "b_sustain", 0.2f }, { "b_release", 12 },
            { "b_cutoff", 520 }, { "b_reso", 0.3f }, { "b_fenv", 3.5f }, { "b_fdec", 45 },
            { "b_drive", 0.35f }, { "b_duck", 0.55f }, { "b_notch", 3 } } },

        { "FO Dark Drive", "Full-On", {
            { "k_start", 2400 }, { "k_punch", 170 }, { "k_end", 44 }, { "k_sweep", 22 }, { "k_drift", 1.5f },
            { "k_knock", 0.2f }, { "k_hold", 6 }, { "k_decay", 95 }, { "k_sync", 2 },
            { "k_fdepth", 0.3f }, { "k_fpos", 13 }, { "k_fwidth", 9 },
            { "k_clvl", 0.3f }, { "k_clhz", 3500 }, { "k_cltone", 0.1f },
            { "k_drive", 0.6f }, { "k_dtype", 1 }, { "k_trans", 0.5f }, { "k_holedb", 5 }, { "k_highcut", 12000 },
            { "b_saw", 0.7f }, { "b_square", 0.4f }, { "b_pw", 0.4f }, { "b_sub", 0.4f }, { "b_decay", 55 }, { "b_sustain", 0.1f },
            { "b_cutoff", 300 }, { "b_reso", 0.45f }, { "b_fenv", 3.8f }, { "b_fdec", 30 },
            { "b_drive", 0.5f }, { "b_dtype", 2 }, { "b_duck", 0.5f }, { "b_notch", 4 }, { "b_highcut", 7000 } } },

        { "FO Clean Classic", "Full-On", {
            { "k_start", 2600 }, { "k_punch", 180 }, { "k_end", 48 }, { "k_sweep", 16 },
            { "k_knock", 0.2f }, { "k_decay", 105 }, { "k_sync", 2 }, { "k_fdepth", 0.25f },
            { "k_clvl", 0.25f }, { "k_drive", 0.15f },
            { "b_saw", 1 }, { "b_sub", 0.35f }, { "b_decay", 70 }, { "b_sustain", 0.2f },
            { "b_cutoff", 450 }, { "b_reso", 0.15f }, { "b_fenv", 2.5f }, { "b_fdec", 50 }, { "b_drive", 0.1f } } },

        // ================= PROGRESSIVE (136–140 BPM, rund und tief) =================
        { "Prog Deep Round", "Progressive", {
            { "k_start", 1800 }, { "k_punch", 150 }, { "k_end", 45 }, { "k_clickt", 1.5f }, { "k_sweep", 22 }, { "k_drift", 1.5f },
            { "k_knock", 0.15f }, { "k_hold", 8 }, { "k_decay", 150 }, { "k_sync", 2 }, { "k_fade", 25 },
            { "k_fdepth", 0.2f }, { "k_fpos", 14 }, { "k_fwidth", 10 },
            { "k_clvl", 0.2f }, { "k_clhz", 3500 }, { "k_cldec", 2.5f }, { "k_cltone", 0.4f },
            { "k_drive", 0.2f }, { "k_holehz", 220 }, { "k_holedb", 3 },
            { "b_saw", 0.4f }, { "b_square", 0.5f }, { "b_sub", 0.5f }, { "b_attack", 1 }, { "b_decay", 110 }, { "b_sustain", 0.35f },
            { "b_release", 25 }, { "b_cutoff", 260 }, { "b_reso", 0.1f }, { "b_fenv", 2.2f }, { "b_fdec", 70 },
            { "b_drive", 0.1f }, { "b_duck", 0.65f }, { "b_duckrel", 80 } } },

        { "Prog Soft Groove", "Progressive", {
            { "k_start", 2000 }, { "k_punch", 160 }, { "k_end", 46 }, { "k_sweep", 20 }, { "k_drift", 1.2f },
            { "k_knock", 0.15f }, { "k_hold", 7 }, { "k_decay", 130 }, { "k_sync", 2 }, { "k_fdepth", 0.2f },
            { "k_clvl", 0.15f }, { "k_clhz", 3200 }, { "k_drive", 0.15f },
            { "b_saw", 0.7f }, { "b_square", 0.3f }, { "b_sub", 0.45f }, { "b_decay", 90 }, { "b_sustain", 0.3f }, { "b_release", 18 },
            { "b_cutoff", 330 }, { "b_reso", 0.15f }, { "b_fenv", 2.5f }, { "b_fdec", 60 }, { "b_drive", 0.15f }, { "b_duck", 0.6f } } },

        { "Prog Tech Pump", "Progressive", {
            { "k_start", 2800 }, { "k_punch", 200 }, { "k_end", 47 }, { "k_sweep", 15 },
            { "k_knock", 0.25f }, { "k_decay", 120 }, { "k_sync", 2 }, { "k_fdepth", 0.3f },
            { "k_clvl", 0.3f }, { "k_drive", 0.35f },
            { "b_saw", 1 }, { "b_sub", 0.35f }, { "b_decay", 65 }, { "b_sustain", 0.15f },
            { "b_cutoff", 400 }, { "b_reso", 0.35f }, { "b_fenv", 3.0f }, { "b_fdec", 35 }, { "b_drive", 0.3f }, { "b_duck", 0.55f } } },

        // ================= BRAZILIAN BOUNCE (145 BPM, Pluck, mehr Mitten) =================
        { "BR Bounce Classic", "Brazilian Bounce", {
            { "k_start", 3500 }, { "k_punch", 240 }, { "k_end", 50 }, { "k_clickt", 0.8f }, { "k_sweep", 13 }, { "k_drift", 0.6f },
            { "k_knock", 0.35f }, { "k_hold", 4 }, { "k_decay", 80 }, { "k_sync", 2 },
            { "k_fdepth", 0.4f }, { "k_fpos", 9 }, { "k_fwidth", 6 },
            { "k_clvl", 0.5f }, { "k_clhz", 6000 }, { "k_cldec", 1.2f }, { "k_cltone", 0.3f },
            { "k_drive", 0.45f }, { "k_dtype", 0 }, { "k_trans", 0.7f }, { "k_holedb", 4 },
            { "b_saw", 1 }, { "b_sub", 0.3f }, { "b_attack", 0.2f }, { "b_decay", 85 }, { "b_sustain", 0.3f }, { "b_release", 15 },
            { "b_cutoff", 480 }, { "b_reso", 0.3f }, { "b_fenv", 3.6f }, { "b_fdec", 55 },
            { "b_pluck", 7 }, { "b_pldec", 9 }, { "b_drive", 0.35f }, { "b_dtype", 3 },
            { "b_duck", 0.6f }, { "b_duckrel", 55 }, { "b_notch", 3 } } },

        { "BR Bounce Hard", "Brazilian Bounce", {
            { "k_start", 3800 }, { "k_punch", 250 }, { "k_end", 50 }, { "k_clickt", 0.7f }, { "k_sweep", 12 },
            { "k_knock", 0.4f }, { "k_hold", 4 }, { "k_decay", 75 }, { "k_sync", 2 },
            { "k_fdepth", 0.45f }, { "k_fpos", 9 }, { "k_fwidth", 6 },
            { "k_clvl", 0.6f }, { "k_clhz", 6500 }, { "k_cldec", 1.0f },
            { "k_drive", 0.55f }, { "k_dtype", 1 }, { "k_trans", 0.7f }, { "k_holedb", 5 },
            { "b_saw", 1 }, { "b_sub", 0.25f }, { "b_detune", 6 }, { "b_decay", 80 }, { "b_sustain", 0.25f }, { "b_release", 12 },
            { "b_cutoff", 600 }, { "b_reso", 0.35f }, { "b_fenv", 3.2f }, { "b_fdec", 45 },
            { "b_pluck", 12 }, { "b_pldec", 6 }, { "b_drive", 0.55f }, { "b_dtype", 1 },
            { "b_duck", 0.6f }, { "b_notch", 4 } } },

        { "BR Bounce Gallop", "Brazilian Bounce", {
            { "k_start", 3300 }, { "k_punch", 230 }, { "k_end", 49 }, { "k_sweep", 14 },
            { "k_knock", 0.3f }, { "k_hold", 5 }, { "k_decay", 85 }, { "k_sync", 2 }, { "k_fdepth", 0.35f },
            { "k_clvl", 0.45f }, { "k_clhz", 5500 }, { "k_drive", 0.4f },
            { "b_saw", 0.8f }, { "b_square", 0.3f }, { "b_sub", 0.35f }, { "b_decay", 110 }, { "b_sustain", 0.4f }, { "b_release", 18 },
            { "b_cutoff", 420 }, { "b_reso", 0.25f }, { "b_fenv", 3.8f }, { "b_fdec", 65 },
            { "b_pluck", 5 }, { "b_pldec", 12 }, { "b_drive", 0.3f }, { "b_duck", 0.55f } } },
    };
    return p;
}

inline const std::vector<FactoryPreset>& drumPresets()
{
    static const std::vector<FactoryPreset> p = {
        // ---- Snare ----
        { "Snare Classic", "Allround", {}, InSnare },
        { "FO Snare Tight", "Full-On", { { "s_tone1", 200 }, { "s_tone2", 340 }, { "s_bdec", 60 }, { "s_pdrop", 8 }, { "s_noise", 0.8f }, { "s_nhp", 2000 },
            { "s_nbp", 6000 }, { "s_ndec", 120 }, { "s_snap", 0.4f }, { "s_drive", 0.35f } }, InSnare },
        { "FO Snare Crack", "Full-On", { { "s_tone1", 230 }, { "s_bdec", 45 }, { "s_noise", 1.0f }, { "s_nbp", 7000 }, { "s_nq", 1.2f }, { "s_ndec", 90 },
            { "s_snap", 0.6f }, { "s_drive", 0.5f }, { "s_dtype", 1 } }, InSnare },
        { "Goa Snare 909", "Goa", { { "s_tone1", 185 }, { "s_tone2", 330 }, { "s_mix", 0.4f }, { "s_bdec", 80 }, { "s_pdrop", 5 }, { "s_noise", 0.9f },
            { "s_nhp", 1800 }, { "s_nbp", 5500 }, { "s_ndec", 170 } }, InSnare },
        { "Prog Snare Soft", "Progressive", { { "s_tone1", 180 }, { "s_bdec", 90 }, { "s_noise", 0.6f }, { "s_nhp", 1200 }, { "s_nbp", 4000 },
            { "s_ndec", 200 }, { "s_snap", 0.15f }, { "s_drive", 0.15f }, { "s_highcut", 11000 } }, InSnare },
        { "BR Snare Fat", "Brazilian Bounce", { { "s_tone1", 175 }, { "s_bdec", 100 }, { "s_body", 0.9f }, { "s_noise", 0.8f }, { "s_ndec", 160 },
            { "s_snap", 0.35f }, { "s_drive", 0.45f }, { "s_dtype", 3 } }, InSnare },
        { "Dark Snare Metal", "Dark / Hi-Tech", { { "s_tone1", 260 }, { "s_tone2", 610 }, { "s_mix", 0.5f }, { "s_bdec", 50 }, { "s_pdrop", 12 },
            { "s_noise", 0.9f }, { "s_nbp", 8000 }, { "s_nq", 2.5f }, { "s_ndec", 110 }, { "s_drive", 0.6f }, { "s_dtype", 2 } }, InSnare },

        // ---- Clap ----
        { "Clap Classic", "Allround", {}, InClap },
        { "FO Clap Tight", "Full-On", { { "c_bursts", 3 }, { "c_space", 8 }, { "c_tail", 0.6f }, { "c_tdec", 120 }, { "c_bp", 1600 }, { "c_q", 1.5f },
            { "c_drive", 0.3f } }, InClap },
        { "Prog Clap Big", "Progressive", { { "c_bursts", 4 }, { "c_space", 12 }, { "c_jitter", 0.35f }, { "c_tail", 0.85f }, { "c_tdec", 380 },
            { "c_bp", 1200 }, { "c_q", 1.0f } }, InClap },
        { "BR Clap Snappy", "Brazilian Bounce", { { "c_bursts", 2 }, { "c_space", 7 }, { "c_bdec", 2.5f }, { "c_tdec", 150 }, { "c_bp", 2000 },
            { "c_q", 1.8f }, { "c_drive", 0.4f } }, InClap },
        { "Dark Clap", "Dark / Hi-Tech", { { "c_bp", 900 }, { "c_q", 2.2f }, { "c_tdec", 250 }, { "c_lp", 7000 }, { "c_drive", 0.5f } }, InClap },

        // ---- Closed Hat ----
        { "CH Tight", "Allround", { { "h_dec", 35 }, { "h_freq", 380 }, { "h_noise", 0.3f }, { "h_bp", 10000 }, { "h_hp", 8000 } }, InCHat },
        { "FO CH Crisp", "Full-On", { { "h_dec", 45 }, { "h_noise", 0.4f }, { "h_bp", 11000 }, { "h_q", 1.2f }, { "h_hp", 7500 }, { "h_drive", 0.2f } }, InCHat },
        { "Prog CH Soft", "Progressive", { { "h_dec", 60 }, { "h_noise", 0.6f }, { "h_bp", 8000 }, { "h_hp", 6000 }, { "h_hc", 13000 } }, InCHat },
        { "Dark CH Metal", "Dark / Hi-Tech", { { "h_dec", 40 }, { "h_freq", 520 }, { "h_inh", 0.9f }, { "h_noise", 0.15f }, { "h_bp", 7000 },
            { "h_q", 2.0f }, { "h_drive", 0.4f } }, InCHat },
        { "Hi-Tech CH Tick", "Dark / Hi-Tech", { { "h_dec", 22 }, { "h_freq", 600 }, { "h_noise", 0.5f }, { "h_bp", 12000 }, { "h_hp", 9500 } }, InCHat },

        // ---- Open Hat ----
        { "OH Classic", "Allround", {}, InOHat },
        { "FO OH Long", "Full-On", { { "o_dec", 380 }, { "o_bp", 9000 }, { "o_noise", 0.4f } }, InOHat },
        { "Prog OH Short", "Progressive", { { "o_dec", 180 }, { "o_noise", 0.55f }, { "o_bp", 7500 }, { "o_hc", 14000 } }, InOHat },
        { "Dark OH Trashy", "Dark / Hi-Tech", { { "o_inh", 0.85f }, { "o_noise", 0.2f }, { "o_dec", 300 }, { "o_drive", 0.4f }, { "o_q", 1.5f } }, InOHat },

        // ---- Ride ----
        { "Ride Classic", "Allround", {}, InRide },
        { "FO Ride Bright", "Full-On", { { "r_freq", 460 }, { "r_bp", 7500 }, { "r_hp", 4500 }, { "r_dec", 900 }, { "r_bell", 0.25f } }, InRide },
        { "Ride Bell", "Allround", { { "r_bell", 0.7f }, { "r_bellhz", 1100 }, { "r_dec", 800 }, { "r_noise", 0.15f } }, InRide },
        { "Prog Ride Wash", "Progressive", { { "r_noise", 0.45f }, { "r_dec", 1300 }, { "r_bp", 5000 }, { "r_bell", 0.1f } }, InRide },

        // ---- Perc ----
        { "Goa Blip", "Goa", { { "p_tune", 300 }, { "p_penv", 18 }, { "p_ptime", 8 }, { "p_dec", 90 }, { "p_drive", 0.4f } }, InPerc },
        { "Perc Wood", "Allround", { { "p_tune", 420 }, { "p_wave", 0.3f }, { "p_fm", 0.35f }, { "p_fmr", 2.7f }, { "p_penv", 6 }, { "p_ptime", 5 },
            { "p_dec", 70 }, { "p_lp", 6000 } }, InPerc },
        { "Perc Tom", "Allround", { { "p_tune", 140 }, { "p_penv", 9 }, { "p_ptime", 30 }, { "p_dec", 220 }, { "p_drive", 0.3f } }, InPerc },
        { "FO Perc Laser FM", "Full-On", { { "p_tune", 500 }, { "p_fm", 0.7f }, { "p_fmr", 3.5f }, { "p_penv", 24 }, { "p_ptime", 20 }, { "p_dec", 150 },
            { "p_reso", 0.5f } }, InPerc },
        { "Dark Perc Click", "Dark / Hi-Tech", { { "p_tune", 900 }, { "p_wave", 0.9f }, { "p_noise", 0.4f }, { "p_penv", 5 }, { "p_dec", 40 },
            { "p_lp", 5000 }, { "p_reso", 0.6f } }, InPerc },

        // ---- Zap ----
        { "Zap Classic", "Allround", {}, InZap },
        { "Zap Laser Long", "Full-On", { { "z_start", 6000 }, { "z_end", 80 }, { "z_sweep", 700 }, { "z_curve", 0.4f }, { "z_len", 900 },
            { "z_wave", 0.3f }, { "z_reso", 0.6f } }, InZap },
        { "Zap Short Pew", "Allround", { { "z_start", 4000 }, { "z_end", 300 }, { "z_sweep", 80 }, { "z_curve", 0.7f }, { "z_len", 120 } }, InZap },
        { "Dark Zap Drop", "Dark / Hi-Tech", { { "z_start", 1500 }, { "z_end", 30 }, { "z_sweep", 500 }, { "z_curve", 0.3f }, { "z_len", 600 },
            { "z_wave", 0.7f }, { "z_lp", 2 }, { "z_reso", 0.5f }, { "z_drive", 0.5f } }, InZap },

        // ---- Riser ----
        { "Riser Classic", "Allround", {}, InRiser },
        { "Riser Noise", "Allround", { { "x_tone", 0.0f }, { "x_noise", 0.9f }, { "x_reso", 0.6f } }, InRiser },
        { "Riser Tonal", "Allround", { { "x_tone", 0.8f }, { "x_noise", 0.3f }, { "x_start", 100 }, { "x_end", 3000 } }, InRiser },
        { "Riser Big", "Full-On", { { "x_len", 16 }, { "x_end", 10000 }, { "x_curve", 0.7f }, { "x_reso", 0.5f }, { "x_drive", 0.3f } }, InRiser },
    };
    return p;
}

// Alle Werks-Presets, nach Instrument (Kick- und Bass-Presets aus den Kombinationen abgeleitet)
inline const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> all = []
    {
        std::vector<FactoryPreset> out;
        for (int inst : { InKick, InBass })
            for (auto& c : kickBassCombos())
            {
                FactoryPreset fp;
                fp.name = c.name; fp.category = c.category; fp.inst = inst;
                for (auto& v : c.values) if (belongsTo (v.first, inst)) fp.values.push_back (v);
                out.push_back (fp);
            }
        for (auto& d : drumPresets()) out.push_back (d);
        return out;
    }();
    return all;
}

// Setzt nur die Regler des Preset-Instruments (auf Standard + Preset-Werte), alles andere bleibt
inline SynthParams applyPreset (const FactoryPreset& fp, const SynthParams& current)
{
    SynthParams p = current;
    const SynthParams def = defaultParams();
    for (auto& d : paramTable())
        if (fp.inst < 0 ? ! isSessionParam (d.id) : belongsTo (d.id, fp.inst)) p.*(d.field) = def.*(d.field);
    for (auto& [id, v] : fp.values)
        if (auto* d = findParam (id))
            if (fp.inst < 0 || belongsTo (d->id, fp.inst)) p.*(d->field) = v;
    return p;
}
} // namespace pf
