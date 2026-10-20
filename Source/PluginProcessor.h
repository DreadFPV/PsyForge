#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <atomic>
#include <memory>
#include <thread>
#include <mutex>
#include "Synth.h"
#include "ParamTable.h"
#include "Presets.h"
#include "Analysis.h"
#include "Patterns.h"
#include "Clip.h"

// Importiertes One-Shot (für Anzeige, Vorhören und Analyse)
struct ReferenceSound
{
    juce::String name;
    std::vector<float> samples;   // mono, normalisiert, führende Stille entfernt
    double sampleRate = 44100.0;
    int inst = 0;                 // Instrument, für das es geladen wurde
};

// Abspielbare Noten (aus Pattern oder MIDI-Clip) für das Instrument dieser Instanz
struct SeqEvent { double beat = 0, len = 0.25; int note = 60; float vel = 1.0f; };
struct SeqData
{
    std::vector<SeqEvent> events;   // nach beat sortiert, innerhalb [0, loopBeats)
    double loopBeats = 16.0;
    int inst = 0;
    bool fromClip = false;
};

// Verbindung zwischen PsyForge-Instanzen im selben Projekt: Kick-Instanzen melden ihre Anschläge,
// Bass-Instanzen lesen sie fürs Ducking.
struct KickLink
{
    static constexpr int N = 32;
    std::array<std::atomic<double>, N> ppq {}, ms {};
    std::atomic<uint32_t> written { 0 };
    std::atomic<double> lastMs { -1.0e12 };
    void publish (double ppqPos, double wallMs)
    {
        const uint32_t i = written.load() % N;
        ppq[i].store (ppqPos); ms[i].store (wallMs);
        written.fetch_add (1);
        lastMs.store (wallMs);
    }
    static KickLink& get() { static KickLink k; return k; }
};

class PsyForgeProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    PsyForgeProcessor();
    ~PsyForgeProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "PsyForge"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;

    pf::SynthParams readParams() const;                       // aktuelle Werte (jeder Thread)
    float getParam (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    int instrument() const { return juce::jlimit (0, pf::NumInstruments - 1, (int) getParam ("g_inst")); }
    void setParam (const juce::String& id, float value);      // Message-Thread, meldet an den Host
    void applyParams (const pf::SynthParams&, const char* prefix = nullptr, bool includeSession = false);

    double getPreviewBpm() const;                             // Host-Tempo wenn bekannt, sonst Vorschau-BPM
    double getPreviewSampleRate() const { return currentSampleRate > 0 ? currentSampleRate : 44100.0; }
    bool isHostPlaying() const { return hostPlaying.load(); }

    // Presets (pro Instrument)
    juce::String getPresetName() const { return presetName[(size_t) instrument()]; }
    void loadFactoryPreset (const pf::FactoryPreset&);
    static juce::File presetDirectory();
    juce::Array<juce::File> listUserPresets() const;        // nur für das aktuelle Instrument
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::File&);

    // Vorhören
    void audition() { padHit.store (true); }
    void auditionReference() { refPlayPos.store (0.0); refPlaying.store (true); }
    std::atomic<bool> previewOn { false };      // Pattern/Clip vorhören (ohne DAW-Play)

    // Sequenz (Pattern oder Clip) für Anzeige
    std::shared_ptr<const SeqData> getSequence() const;
    std::atomic<double> uiBeat { -1.0 };        // aktuelle Position in der Sequenz (Beats), -1 = steht

    // MIDI-Clip
    bool loadClip (const juce::File&, juce::String& error);
    void clearClip();
    std::shared_ptr<const pf::MidiClip> getClip() const { return clip; }
    int getClipFilter() const { return clipFilter; }
    void setClipFilter (int f) { clipFilter = f; seqDirty = true; }

    // Export
    enum WavWhat { WavHit = 0, WavLoop };
    juce::File renderWav (int what, const juce::File& target = {});
    juce::String suggestedWavName (int what) const;
    enum MidiWhat { MidiSection = 0, MidiFullTrack, MidiAllSection, MidiAllFullTrack, MidiAllSections, MidiClipOut };
    juce::File writeMidi (int what);            // schreibt in den Temp-Ordner (für Drag & Drop)
    bool exportAllSections (const juce::File& folder);

    // Referenz + Analyse
    bool loadReference (const juce::File&, juce::String& error);
    std::shared_ptr<const ReferenceSound> getReference() const;
    void clearReference();
    void startAnalysis (bool fine);
    void cancelAnalysis();
    bool isAnalysing() const { return analysing.load(); }
    float getAnalysisProgress() const { return analysisProgress.load(); }
    float getAnalysisLiveMatch() const { return analysisMatch.load(); }
    bool hasAnalysisResult() const;
    pf::AnalysisResult getAnalysisResult() const;
    int getAnalysisVersion() const { return analysisVersion.load(); }
    void applyAnalysisResult();
    bool canUndoAnalysis() const { return hasUndo; }
    void undoAnalysis();

    int getUiWidth() const { return uiWidth; }
    void setUiWidth (int w) { uiWidth = w; }

    std::atomic<float> uiLevel { 0.0f }, uiDuck { 0.0f };
    std::atomic<int> uiParamVersion { 0 };
    std::atomic<int> uiHits { 0 };

private:
    static BusesProperties makeBuses();
    void timerCallback() override;
    void rebuildSequence();
    std::shared_ptr<SeqData> buildSequence (const pf::SynthParams&, double bpm) const;
    pf::Pattern currentPattern (const pf::SynthParams&, bool fullTrack) const;
    pf::Groove currentGroove (const pf::SynthParams&, double bpm) const;
    void triggerNote (int note, float vel, int gate, int sampleIndex, double blockPpq, double blockMs, double spb);

    pf::Synth synth;
    std::vector<std::atomic<float>*> raw;    // parallel zu pf::paramTable()

    double currentSampleRate = 0.0;
    std::atomic<double> hostBpm { 0.0 };
    std::atomic<bool> hostPlaying { false };
    double freeBeat = 0.0;
    bool wasPlayingSeq = false;
    std::atomic<bool> padHit { false };

    // Sequenz
    std::shared_ptr<const SeqData> sequence;
    mutable juce::SpinLock seqLock;
    std::vector<std::shared_ptr<const SeqData>> retired;
    std::atomic<bool> seqDirty { true };
    pf::SynthParams lastSeqParams;
    double lastSeqBpm = 0.0;
    struct BlockEv { int pos; int note; float vel; int gate; };
    std::vector<BlockEv> blockEvents;

    // MIDI-Clip
    std::shared_ptr<const pf::MidiClip> clip;
    int clipFilter = pf::ClipAuto;

    // Ducking
    uint32_t linkRead = 0;
    struct DuckEv { int pos; int ago; };
    std::vector<DuckEv> duckEvents;
    float scEnv = 0.0f; bool scArmed = true;

    // Referenz-Wiedergabe
    std::shared_ptr<const ReferenceSound> reference;
    mutable juce::SpinLock refLock;
    std::atomic<bool> refPlaying { false };
    std::atomic<double> refPlayPos { 0.0 };

    // Analyse
    std::thread analysisThread;
    std::atomic<bool> analysing { false }, analysisCancel { false };
    std::atomic<float> analysisProgress { 0.0f }, analysisMatch { 0.0f };
    std::atomic<int> analysisVersion { 0 };
    mutable std::mutex resultMutex;
    std::unique_ptr<pf::AnalysisResult> analysisResult;
    pf::SynthParams undoParams;
    bool hasUndo = false;

    std::array<juce::String, pf::NumInstruments> presetName;
    int uiWidth = 1180;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PsyForgeProcessor)
};
