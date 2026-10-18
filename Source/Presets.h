#pragma once
// PsyForge – Werks-Presets (Full-On, Progressive, Brazilian Bounce)
// Nicht gesetzte Werte = Standard. Tonart, A4, Master, Limiter, MIDI-Kanäle und Vorschau-Note bleiben beim Laden unverändert.
#include "ParamTable.h"

namespace pf
{
struct FactoryPreset
{
    const char* name;
    const char* category;
    std::vector<std::pair<const char*, float>> values;
};

// Parameter, die ein Klang-Preset NICHT verändert
inline bool isSessionParam (const std::string& id)
{
    return id == "g_key" || id == "g_a4" || id == "g_master" || id == "g_limit"
        || id == "g_kickch" || id == "g_bassch" || id == "g_note";
}

inline const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> p = {
        { "Init", "Init", {} },

        // ================= FULL-ON (145 BPM, rollende KBBB) =================
        { "FO Tight Rolling", "Full-On", {
            { "g_bpm", 145 }, { "g_pattern", 0 },
            { "k_start", 3000 }, { "k_punch", 210 }, { "k_end", 48 }, { "k_clickt", 1.0f }, { "k_sweep", 14 }, { "k_drift", 0.8f },
            { "k_knock", 0.3f }, { "k_hold", 5 }, { "k_decay", 90 }, { "k_sync", 2 }, { "k_fade", 18 },
            { "k_fdepth", 0.35f }, { "k_fpos", 10 }, { "k_fwidth", 7 },
            { "k_clvl", 0.4f }, { "k_clhz", 5000 }, { "k_cldec", 1.5f }, { "k_cltone", 0.25f },
            { "k_drive", 0.35f }, { "k_dtype", 0 }, { "k_trans", 0.6f }, { "k_lowcut", 22 }, { "k_holehz", 200 }, { "k_holedb", 4 },
            { "b_saw", 1 }, { "b_square", 0 }, { "b_sub", 0.35f }, { "b_attack", 0.3f }, { "b_decay", 60 }, { "b_sustain", 0.15f },
            { "b_release", 10 }, { "b_cutoff", 380 }, { "b_reso", 0.25f }, { "b_fenv", 3.2f }, { "b_fdec", 38 }, { "b_key", 0.5f },
            { "b_drive", 0.25f }, { "b_duck", 0.5f }, { "b_duckrel", 50 }, { "b_lowcut", 32 }, { "b_notch", 3 }, { "b_highcut", 8000 } } },

        { "FO Festival Punch", "Full-On", {
            { "g_bpm", 145 }, { "g_pattern", 0 },
            { "k_start", 3200 }, { "k_punch", 260 }, { "k_end", 50 }, { "k_clickt", 0.9f }, { "k_sweep", 18 }, { "k_drift", 1.0f },
            { "k_knock", 0.4f }, { "k_hold", 7 }, { "k_decay", 100 }, { "k_sync", 2 },
            { "k_fdepth", 0.4f }, { "k_fpos", 11 }, { "k_fwidth", 8 },
            { "k_clvl", 0.5f }, { "k_clhz", 5500 }, { "k_cldec", 1.6f }, { "k_cltone", 0.35f },
            { "k_drive", 0.5f }, { "k_dtype", 3 }, { "k_trans", 0.65f }, { "k_holedb", 5 },
            { "b_saw", 1 }, { "b_sub", 0.3f }, { "b_detune", 8 }, { "b_decay", 70 }, { "b_sustain", 0.2f }, { "b_release", 12 },
            { "b_cutoff", 520 }, { "b_reso", 0.3f }, { "b_fenv", 3.5f }, { "b_fdec", 45 },
            { "b_drive", 0.35f }, { "b_duck", 0.55f }, { "b_notch", 3 } } },

        { "FO Dark Drive", "Full-On", {
            { "g_bpm", 147 }, { "g_pattern", 0 },
            { "k_start", 2400 }, { "k_punch", 170 }, { "k_end", 44 }, { "k_sweep", 22 }, { "k_drift", 1.5f },
            { "k_knock", 0.2f }, { "k_hold", 6 }, { "k_decay", 95 }, { "k_sync", 2 },
            { "k_fdepth", 0.3f }, { "k_fpos", 13 }, { "k_fwidth", 9 },
            { "k_clvl", 0.3f }, { "k_clhz", 3500 }, { "k_cltone", 0.1f },
            { "k_drive", 0.6f }, { "k_dtype", 1 }, { "k_trans", 0.5f }, { "k_holedb", 5 }, { "k_highcut", 12000 },
            { "b_saw", 0.7f }, { "b_square", 0.4f }, { "b_pw", 0.4f }, { "b_sub", 0.4f }, { "b_decay", 55 }, { "b_sustain", 0.1f },
            { "b_cutoff", 300 }, { "b_reso", 0.45f }, { "b_fenv", 3.8f }, { "b_fdec", 30 },
            { "b_drive", 0.5f }, { "b_dtype", 2 }, { "b_duck", 0.5f }, { "b_notch", 4 }, { "b_highcut", 7000 } } },

        { "FO Clean Classic", "Full-On", {
            { "g_bpm", 145 }, { "g_pattern", 0 },
            { "k_start", 2600 }, { "k_punch", 180 }, { "k_end", 48 }, { "k_sweep", 16 },
            { "k_knock", 0.2f }, { "k_decay", 105 }, { "k_sync", 2 }, { "k_fdepth", 0.25f },
            { "k_clvl", 0.25f }, { "k_drive", 0.15f },
            { "b_saw", 1 }, { "b_sub", 0.35f }, { "b_decay", 70 }, { "b_sustain", 0.2f },
            { "b_cutoff", 450 }, { "b_reso", 0.15f }, { "b_fenv", 2.5f }, { "b_fdec", 50 }, { "b_drive", 0.1f } } },

        // ================= PROGRESSIVE (136–140 BPM, rund und tief) =================
        { "Prog Deep Round", "Progressive", {
            { "g_bpm", 138 }, { "g_pattern", 1 },
            { "k_start", 1800 }, { "k_punch", 150 }, { "k_end", 45 }, { "k_clickt", 1.5f }, { "k_sweep", 22 }, { "k_drift", 1.5f },
            { "k_knock", 0.15f }, { "k_hold", 8 }, { "k_decay", 150 }, { "k_sync", 2 }, { "k_fade", 25 },
            { "k_fdepth", 0.2f }, { "k_fpos", 14 }, { "k_fwidth", 10 },
            { "k_clvl", 0.2f }, { "k_clhz", 3500 }, { "k_cldec", 2.5f }, { "k_cltone", 0.4f },
            { "k_drive", 0.2f }, { "k_holehz", 220 }, { "k_holedb", 3 },
            { "b_saw", 0.4f }, { "b_square", 0.5f }, { "b_sub", 0.5f }, { "b_attack", 1 }, { "b_decay", 110 }, { "b_sustain", 0.35f },
            { "b_release", 25 }, { "b_cutoff", 260 }, { "b_reso", 0.1f }, { "b_fenv", 2.2f }, { "b_fdec", 70 },
            { "b_drive", 0.1f }, { "b_duck", 0.65f }, { "b_duckrel", 80 } } },

        { "Prog Soft Groove", "Progressive", {
            { "g_bpm", 136 }, { "g_pattern", 0 },
            { "k_start", 2000 }, { "k_punch", 160 }, { "k_end", 46 }, { "k_sweep", 20 }, { "k_drift", 1.2f },
            { "k_knock", 0.15f }, { "k_hold", 7 }, { "k_decay", 130 }, { "k_sync", 2 }, { "k_fdepth", 0.2f },
            { "k_clvl", 0.15f }, { "k_clhz", 3200 }, { "k_drive", 0.15f },
            { "b_saw", 0.7f }, { "b_square", 0.3f }, { "b_sub", 0.45f }, { "b_decay", 90 }, { "b_sustain", 0.3f }, { "b_release", 18 },
            { "b_cutoff", 330 }, { "b_reso", 0.15f }, { "b_fenv", 2.5f }, { "b_fdec", 60 }, { "b_drive", 0.15f }, { "b_duck", 0.6f } } },

        { "Prog Tech Pump", "Progressive", {
            { "g_bpm", 138 }, { "g_pattern", 0 },
            { "k_start", 2800 }, { "k_punch", 200 }, { "k_end", 47 }, { "k_sweep", 15 },
            { "k_knock", 0.25f }, { "k_decay", 120 }, { "k_sync", 2 }, { "k_fdepth", 0.3f },
            { "k_clvl", 0.3f }, { "k_drive", 0.35f },
            { "b_saw", 1 }, { "b_sub", 0.35f }, { "b_decay", 65 }, { "b_sustain", 0.15f },
            { "b_cutoff", 400 }, { "b_reso", 0.35f }, { "b_fenv", 3.0f }, { "b_fdec", 35 }, { "b_drive", 0.3f }, { "b_duck", 0.55f } } },

        // ================= BRAZILIAN BOUNCE (145 BPM, Pluck, mehr Mitten) =================
        { "BR Bounce Classic", "Brazilian Bounce", {
            { "g_bpm", 145 }, { "g_pattern", 0 },
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
            { "g_bpm", 146 }, { "g_pattern", 0 },
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
            { "g_bpm", 145 }, { "g_pattern", 2 },
            { "k_start", 3300 }, { "k_punch", 230 }, { "k_end", 49 }, { "k_sweep", 14 },
            { "k_knock", 0.3f }, { "k_hold", 5 }, { "k_decay", 85 }, { "k_sync", 2 }, { "k_fdepth", 0.35f },
            { "k_clvl", 0.45f }, { "k_clhz", 5500 }, { "k_drive", 0.4f },
            { "b_saw", 0.8f }, { "b_square", 0.3f }, { "b_sub", 0.35f }, { "b_decay", 110 }, { "b_sustain", 0.4f }, { "b_release", 18 },
            { "b_cutoff", 420 }, { "b_reso", 0.25f }, { "b_fenv", 3.8f }, { "b_fdec", 65 },
            { "b_pluck", 5 }, { "b_pldec", 12 }, { "b_drive", 0.3f }, { "b_duck", 0.55f } } },
    };
    return p;
}

inline SynthParams applyPreset (const FactoryPreset& fp, const SynthParams& current)
{
    SynthParams p = defaultParams();
    for (auto& d : paramTable())
        if (isSessionParam (d.id)) p.*(d.field) = current.*(d.field);
    for (auto& [id, v] : fp.values)
        if (auto* d = findParam (id)) p.*(d->field) = v;
    return p;
}
} // namespace pf
