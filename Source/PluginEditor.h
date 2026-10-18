#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

namespace pfui
{
struct PfLookAndFeel : juce::LookAndFeel_V4
{
    PfLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::FontOptions (13.0f); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return juce::FontOptions (13.0f, juce::Font::bold); }
    juce::Label* createSliderTextBox (juce::Slider&) override;
};

//==============================================================================
// Ein Regler aus der Parameter-Tabelle (Drehknopf, Auswahl oder Schalter)
struct ParamControl : juce::Component
{
    ParamControl (PsyForgeProcessor&, const pf::PDef&, juce::Colour);
    ~ParamControl() override;
    void resized() override;
    void paint (juce::Graphics&) override;

    juce::SharedResourcePointer<PfLookAndFeel> lnf;
    const pf::PDef& def;
    juce::Colour colour;
    juce::Label label;
    std::unique_ptr<juce::Slider> slider;
    std::unique_ptr<juce::ComboBox> combo;
    std::unique_ptr<juce::ToggleButton> toggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bAtt;
};

// Gruppe mit Titel
struct Section : juce::Component
{
    Section (PsyForgeProcessor&, const juce::String& title, std::initializer_list<const char*> ids, juce::Colour);
    void paint (juce::Graphics&) override;
    void resized() override;
    int preferredWidth() const;
    juce::String title;
    juce::Colour colour;
    juce::OwnedArray<ParamControl> controls;
};

// Seite mit mehreren Gruppen, die zeilenweise umbrechen
struct Page : juce::Component
{
    void resized() override;
    juce::OwnedArray<Section> sections;
};

//==============================================================================
struct WaveView : juce::Component, juce::Timer, juce::SettableTooltipClient
{
    explicit WaveView (PsyForgeProcessor&);
    void paint (juce::Graphics&) override;
    void timerCallback() override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void setMode (int m) { mode = m; dirty = true; repaint(); }
    int getMode() const { return mode; }
    void refresh() { dirty = true; }

    PsyForgeProcessor& proc;
    int mode = 0;               // 0 Kick, 1 Bass, 2 Loop
    bool showReference = true;
    bool dirty = true;
    bool dragStarted = false;
    pf::RenderOut render;
    pf::SynthParams lastParams;
    double lastBpm = 0;
    std::shared_ptr<const ReferenceSound> lastRef;
    float hoverX = -1;
    juce::String info;
};

//==============================================================================
struct AnalysePage : juce::Component, juce::FileDragAndDropTarget
{
    explicit AnalysePage (PsyForgeProcessor&, std::function<void()> onReferenceChanged);
    void paint (juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray&, int, int) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dragOver = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { dragOver = false; repaint(); }
    void loadFile (const juce::File&);
    void update();

    PsyForgeProcessor& proc;
    std::function<void()> onRefChanged;
    juce::TextButton loadButton { juce::String::fromUTF8 ("WAV laden …") }, playRefButton { juce::String::fromUTF8 ("▶ Original") },
                     analyseButton { "Analysieren + Feinanpassung" }, quickButton { "Nur Grob-Analyse" },
                     cancelButton { "Abbrechen" }, applyButton { juce::String::fromUTF8 ("Übernehmen") }, undoButton { juce::String::fromUTF8 ("Rückgängig") };
    juce::ComboBox typeBox;
    juce::ToggleButton showRef { "Original in der Waveform zeigen" };
    double progress = 0.0;
    juce::ProgressBar progressBar { progress };
    juce::String status, summary;
    std::unique_ptr<juce::FileChooser> chooser;
    bool dragOver = false;
    int lastVersion = -1;
    juce::Rectangle<int> dropArea;
};

} // namespace pfui

//==============================================================================
class PsyForgeEditor : public juce::AudioProcessorEditor, private juce::Timer, public juce::FileDragAndDropTarget
{
public:
    explicit PsyForgeEditor (PsyForgeProcessor&);
    ~PsyForgeEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray&, int, int) override;

    static constexpr int baseW = 1180, baseH = 790;

private:
    struct Content : juce::Component
    {
        explicit Content (PsyForgeEditor& e) : ed (e) {}
        void paint (juce::Graphics&) override;
        void resized() override;
        PsyForgeEditor& ed;
    };

    void timerCallback() override;
    void showPage (int);
    void buildPresetMenu();
    void presetChosen();
    void stepPreset (int delta);
    void savePresetDialog();
    void exportDialog (int what);

    PsyForgeProcessor& proc;
    juce::SharedResourcePointer<pfui::PfLookAndFeel> lnf;
    Content content { *this };
    juce::TooltipWindow tooltips { this, 500 };

    juce::Label titleLabel, presetCaption;
    juce::ComboBox presetBox;
    juce::TextButton prevPreset { "<" }, nextPreset { ">" }, savePreset { "Speichern" };
    std::unique_ptr<pfui::ParamControl> keyCtl, a4Ctl, masterCtl, kickChCtl, bassChCtl, limitCtl;
    juce::TextButton tune440 { "440" }, tune432 { "432" };

    std::unique_ptr<pfui::WaveView> wave;
    juce::TextButton viewKick { juce::String::fromUTF8 ("∿ Kick") }, viewBass { juce::String::fromUTF8 ("∿ Bass") }, viewLoop { juce::String::fromUTF8 ("∿ Loop") };
    juce::TextButton playKick { juce::String::fromUTF8 ("▶ Kick") }, playBass { juce::String::fromUTF8 ("▶ Bass") }, loopButton { juce::String::fromUTF8 ("⟳ Loop") };
    std::unique_ptr<pfui::ParamControl> patternCtl, noteCtl, bpmCtl;
    juce::TextButton expKick { "Kick-WAV" }, expBass { "Bass-WAV" }, expLoop { "Loop-WAV" };
    juce::Label exportHint;

    juce::TextButton tabKick { "KICK" }, tabBass { "BASS" }, tabAnalyse { "ANALYSE" };
    pfui::Page kickPage, bassPage;
    std::unique_ptr<pfui::AnalysePage> analysePage;
    int page = 0;

    std::unique_ptr<juce::FileChooser> chooser;
    juce::Array<juce::File> userPresets;
    int lastParamVersion = -1;
    juce::String lastPresetName;
    float meterK = 0, meterB = 0, meterO = 0;
    juce::Rectangle<int> meterArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PsyForgeEditor)
};
