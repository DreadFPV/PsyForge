#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

namespace pfui
{
juce::Colour instColour (int inst);

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
    Section (PsyForgeProcessor&, const juce::String& title, const std::vector<const char*>& ids, juce::Colour);
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
// Wellenform eines Anschlags (mit Tonhöhe, Hüllkurve, Original zum Vergleich)
struct WaveView : juce::Component, juce::Timer, juce::SettableTooltipClient
{
    explicit WaveView (PsyForgeProcessor&);
    void paint (juce::Graphics&) override;
    void timerCallback() override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void refresh() { dirty = true; }

    PsyForgeProcessor& proc;
    bool showReference = true;
    bool dirty = true;
    bool dragStarted = false;
    int inst = 0;
    pf::RenderOut render;
    pf::SynthParams lastParams;
    double lastBpm = 0;
    std::shared_ptr<const ReferenceSound> lastRef;
    float hoverX = -1;
    juce::String info;
};

// Noten der aktuellen Sequenz (Pattern oder Clip) mit Abspielposition
struct NoteRoll : juce::Component, juce::Timer, juce::SettableTooltipClient
{
    explicit NoteRoll (PsyForgeProcessor& p) : proc (p) { startTimerHz (30); }
    void paint (juce::Graphics&) override;
    void timerCallback() override { repaint(); }
    PsyForgeProcessor& proc;
    bool showClipAll = false;     // MIDI-Clip-Seite: alle Noten der Datei zeigen, passende hervorheben
};

// Ziehbarer Chip: per Drag & Drop in die DAW, per Klick speichern
struct DragChip : juce::Component, juce::SettableTooltipClient
{
    DragChip (const juce::String& text, juce::Colour c, std::function<juce::File()> make, std::function<void()> click = {})
        : label (text), colour (c), makeFile (std::move (make)), onClick (std::move (click)) { setMouseCursor (juce::MouseCursor::DraggingHandCursor); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { dragging = false; }
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }
    juce::String label;
    juce::Colour colour;
    std::function<juce::File()> makeFile;
    std::function<void()> onClick;
    bool dragging = false;
};

//==============================================================================
struct ClipPage : juce::Component, juce::FileDragAndDropTarget
{
    explicit ClipPage (PsyForgeProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray&, int, int) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dragOver = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { dragOver = false; repaint(); }
    void loadFile (const juce::File&);
    void update();

    PsyForgeProcessor& proc;
    NoteRoll roll;
    juce::TextButton loadButton { juce::String::fromUTF8 ("MIDI-Datei laden …") }, clearButton { "Clip entfernen" };
    juce::ComboBox filterBox;
    juce::Label filterLabel;
    std::unique_ptr<ParamControl> sourceCtl;
    std::unique_ptr<DragChip> clipChip;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::String status;
    bool dragOver = false;
    const pf::MidiClip* lastClip = nullptr;
    int lastInst = -1;
    juce::Rectangle<int> dropArea;
};

struct AnalysePage : juce::Component, juce::FileDragAndDropTarget
{
    AnalysePage (PsyForgeProcessor&, std::function<void()> onReferenceChanged);
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

    static constexpr int baseW = 1180, baseH = 800;

private:
    struct Content : juce::Component
    {
        explicit Content (PsyForgeEditor& e) : ed (e) {}
        void paint (juce::Graphics&) override;
        void resized() override;
        PsyForgeEditor& ed;
    };

    void timerCallback() override;
    void showTab (int);
    void instrumentChanged();
    void buildPresetMenu();
    void presetChosen();
    void stepPreset (int delta);
    void savePresetDialog();
    void saveFileDialog (const juce::File& source, const juce::String& title, const juce::String& pattern);
    void showHelp();

    PsyForgeProcessor& proc;
    juce::SharedResourcePointer<pfui::PfLookAndFeel> lnf;
    Content content { *this };
    juce::TooltipWindow tooltips { this, 500 };

    juce::Label titleLabel, presetCaption, modeLabel;
    std::unique_ptr<pfui::ParamControl> instCtl;
    juce::ComboBox presetBox;
    juce::TextButton prevPreset { "<" }, nextPreset { ">" }, savePreset { "Speichern" }, helpButton { "?" };
    std::unique_ptr<pfui::ParamControl> keyCtl, a4Ctl, masterCtl, filterCtl, limitCtl;
    juce::TextButton tune440 { "440" }, tune432 { "432" };

    std::unique_ptr<pfui::WaveView> wave;

    // Pattern-Panel
    std::unique_ptr<pfui::ParamControl> styleCtl, sectionCtl, bassRhyCtl, varCtl, swingCtl, humanCtl, bpmCtl, noteCtl, seqCtl;
    juce::TextButton diceButton { juce::String::fromUTF8 ("Zufall") };
    pfui::NoteRoll roll;
    juce::TextButton hitButton { juce::String::fromUTF8 ("▶ Anschlag") }, previewButton { juce::String::fromUTF8 ("⟳ Vorhören") },
                     allSectionsButton { juce::String::fromUTF8 ("Alle Sections …") };
    juce::OwnedArray<pfui::DragChip> chips;
    juce::Label chipHint;

    juce::TextButton tabSound { "SOUND" }, tabClip { "MIDI-CLIP" }, tabAnalyse { "ANALYSE" };
    juce::OwnedArray<pfui::Page> soundPages;   // eine Seite pro Instrument
    std::unique_ptr<pfui::ClipPage> clipPage;
    std::unique_ptr<pfui::AnalysePage> analysePage;
    int tab = 0, shownInst = -1;

    std::unique_ptr<juce::FileChooser> chooser;
    juce::Array<juce::File> userPresets;
    std::vector<const pf::FactoryPreset*> presetList;
    juce::String lastPresetName;
    float meter = 0, duckLed = 0;
    juce::Rectangle<int> meterArea, patternPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PsyForgeEditor)
};
