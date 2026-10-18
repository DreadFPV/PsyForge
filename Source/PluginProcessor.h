#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <atomic>
#include <memory>
#include <thread>
#include <mutex>
#include "Engine.h"
#include "ParamTable.h"
#include "Presets.h"
#include "Analysis.h"

// Importiertes One-Shot (für Anzeige, Vorhören und Analyse)
struct ReferenceSound
{
    juce::String name;
    std::vector<float> samples;   // mono, normalisiert, führende Stille entfernt
    double sampleRate = 44100.0;
    int type = 0;                 // 0 = Kick, 1 = Bass (erkannt bzw. aus der Analyse)
};

class PsyForgeProcessor : public juce::AudioProcessor
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
    void setParam (const juce::String& id, float value);      // Message-Thread, meldet an den Host
    void applyParams (const pf::SynthParams&, const char* prefix = nullptr, bool includeSession = false);

    double getPreviewBpm() const;                             // Host-Tempo wenn bekannt, sonst Vorschau-BPM
    double getPreviewSampleRate() const { return currentSampleRate > 0 ? currentSampleRate : 44100.0; }

    // Presets
    int getPresetIndex() const { return presetIndex; }
    juce::String getPresetName() const { return presetName; }
    void loadFactoryPreset (int index);
    static juce::File presetDirectory();
    juce::Array<juce::File> listUserPresets() const;
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::File&);

    // Vorhören
    void auditionKick()  { padKick.store (true); }
    void auditionBass()  { padBass.store (true); }
    void auditionReference() { refPlayPos.store (0.0); refPlaying.store (true); }
    std::atomic<bool> loopOn { false };

    // Export
    enum ExportWhat { ExportKick = 0, ExportBass, ExportLoop };
    juce::File renderToFile (int what, const juce::File& target = {});
    juce::String suggestedFileName (int what) const;

    // Referenz + Analyse
    bool loadReference (const juce::File&, juce::String& error);
    std::shared_ptr<const ReferenceSound> getReference() const;
    void clearReference();
    void startAnalysis (int forceType, bool fine);
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

    // Fenstergröße
    int getUiWidth() const { return uiWidth; }
    void setUiWidth (int w) { uiWidth = w; }

    // für die Anzeige
    std::atomic<float> uiKickLevel { 0.0f }, uiBassLevel { 0.0f }, uiOutLevel { 0.0f };
    std::atomic<int> uiParamVersion { 0 };

private:
    static BusesProperties makeBuses();
    void handleMidi (const juce::MidiMessage&, const pf::SynthParams&);
    void runLoop (double ppqStart, double ppqEnd, int numSamples, double bpm, int pattern, int note, std::vector<std::pair<int, int>>& events);

    pf::Engine engine;
    std::vector<std::atomic<float>*> raw;    // parallel zu pf::paramTable()

    double currentSampleRate = 0.0;
    std::atomic<double> hostBpm { 0.0 };
    double freeBeat = 0.0;
    std::vector<std::pair<int, int>> loopEvents;   // Ereignisse der Vorschau-Schleife (vorab reserviert)
    int loopBassNote = -1;
    int padBassOffLeft = 0;
    std::atomic<bool> padKick { false }, padBass { false };

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

    int presetIndex = 0;
    juce::String presetName { "Init" };
    int uiWidth = 1180;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PsyForgeProcessor)
};
