// PsyForge – Integrationstest mit JUCE (ohne DAW): Prozessor, MIDI-Kanäle, Ausgänge, Export,
// Referenz-Analyse und Bildschirmfotos der Oberfläche.  Aufruf: PsyForgeUiTest <ausgabeordner> [referenz.wav]
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

static void pump (int ms) { juce::MessageManager::getInstance()->runDispatchLoopUntil (ms); }

static juce::Button* findButton (juce::Component& root, const juce::String& text)
{
    for (auto* c : root.getChildren())
    {
        if (auto* b = dynamic_cast<juce::Button*> (c)) if (b->getButtonText() == text) return b;
        if (auto* r = findButton (*c, text)) return r;
    }
    return nullptr;
}

static void snapshot (juce::Component& c, const juce::File& f)
{
    auto img = c.createComponentSnapshot (c.getLocalBounds(), true, 1.0f);
    f.deleteFile();
    juce::FileOutputStream os (f);
    juce::PNGImageFormat().writeImageToStream (img, os);
    std::printf ("  Bild: %s\n", f.getFullPathName().toRawUTF8());
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File out (argc > 1 ? argv[1] : "/tmp/psyforge-test");
    out.createDirectory();
    int failures = 0;
    auto check = [&] (bool ok, const char* what) { std::printf ("%s %s\n", ok ? "[OK]  " : "[FAIL]", what); if (! ok) ++failures; };

    PsyForgeProcessor p;
    // Main + Kick + Bass als getrennte Ausgänge
    {
        auto layout = p.getBusesLayout();
        layout.outputBuses.getReference (1) = juce::AudioChannelSet::stereo();
        layout.outputBuses.getReference (2) = juce::AudioChannelSet::stereo();
        check (p.setBusesLayout (layout), "Mehrfach-Ausgänge (Main/Kick/Bass) aktivierbar");
    }
    const double sr = 48000.0; const int bs = 480;
    p.setRateAndBufferSizeDetails (sr, bs);
    p.prepareToPlay (sr, bs);
    p.loadFactoryPreset (1);
    check (p.getPresetName() == "FO Tight Rolling", "Werks-Preset laden");

    // 1 Takt KBBB per MIDI: Kick Kanal 1, Bass Kanal 2
    const int total = (int) (sr * 60.0 / 145.0 * 4);
    juce::AudioBuffer<float> buf (6, bs);
    double eK = 0, eB = 0, eM = 0, eMbeforeSplit = 0;
    const double spb = 60.0 / 145.0 * sr;
    for (int pos = 0; pos < total; pos += bs)
    {
        juce::MidiBuffer midi;
        for (int beat = 0; beat < 4; ++beat)
        {
            auto add = [&] (double at, const juce::MidiMessage& m) { const int s = (int) at; if (s >= pos && s < pos + bs) midi.addEvent (m, s - pos); };
            add (beat * spb, juce::MidiMessage::noteOn (1, 36, (juce::uint8) 120));
            for (int k = 1; k < 4; ++k)
            {
                add (beat * spb + k * spb / 4, juce::MidiMessage::noteOn (2, 30, (juce::uint8) 110));
                add (beat * spb + k * spb / 4 + spb / 4 * 0.7, juce::MidiMessage::noteOff (2, 30));
            }
        }
        buf.clear();
        p.processBlock (buf, midi);
        for (int i = 0; i < bs; ++i)
        {
            eM += buf.getSample (0, i) * buf.getSample (0, i);
            eK += buf.getSample (2, i) * buf.getSample (2, i);
            eB += buf.getSample (4, i) * buf.getSample (4, i);
        }
    }
    (void) eMbeforeSplit;
    std::printf ("  Energie Main %.3f  Kick-Out %.3f  Bass-Out %.3f\n", eM, eK, eB);
    check (eK > 1.0 && eB > 1.0, "Kick (Kanal 1) und Bass (Kanal 2) klingen auf ihren Ausgängen");
    check (eM < 0.01, "Main ist leer, wenn Kick/Bass-Ausgänge aktiv sind");

    // Export
    for (int w = 0; w < 3; ++w)
    {
        const auto f = p.renderToFile (w, out.getChildFile (p.suggestedFileName (w)));
        check (f.existsAsFile() && f.getSize() > 1000, ("WAV-Export " + juce::String (w == 0 ? "Kick" : w == 1 ? "Bass" : "Loop")).toRawUTF8());
    }

    // Referenz laden und analysieren
    if (argc > 2)
    {
        juce::String err;
        check (p.loadReference (juce::File (argv[2]), err), "Referenz-WAV laden");
        p.startAnalysis (pf::SoundAuto, true);
        const auto t0 = juce::Time::getMillisecondCounter();
        while (p.isAnalysing()) { juce::Thread::sleep (200); }
        const auto res = p.getAnalysisResult();
        std::printf ("  Analyse: %s  %.1f %%  (%.1f s)\n  %s\n", res.type == pf::SoundKick ? "Kick" : "Bass", res.match,
                     (juce::Time::getMillisecondCounter() - t0) / 1000.0, res.summary.c_str());
        check (res.ok && res.match > 50.0f, "Analyse liefert ein Ergebnis");
        p.applyAnalysisResult();
        check (p.canUndoAnalysis(), "Übernehmen + Rückgängig verfügbar");
    }

    // Oberfläche
    std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
    ed->setSize (PsyForgeEditor::baseW, PsyForgeEditor::baseH);
    ed->setVisible (true);
    pump (600);
    snapshot (*ed, out.getChildFile ("ui_kick.png"));
    if (auto* b = findButton (*ed, "BASS")) { b->triggerClick(); pump (400); }
    snapshot (*ed, out.getChildFile ("ui_bass.png"));
    if (auto* b = findButton (*ed, juce::String::fromUTF8 ("∿ Loop"))) { b->triggerClick(); pump (400); }
    snapshot (*ed, out.getChildFile ("ui_loop.png"));
    if (auto* b = findButton (*ed, "ANALYSE")) { b->triggerClick(); pump (400); }
    snapshot (*ed, out.getChildFile ("ui_analyse.png"));
    ed->setSize (PsyForgeEditor::baseW * 7 / 10, PsyForgeEditor::baseH * 7 / 10);
    pump (300);
    snapshot (*ed, out.getChildFile ("ui_small.png"));

    // Status speichern / laden
    juce::MemoryBlock mb; p.getStateInformation (mb);
    PsyForgeProcessor p2; p2.setStateInformation (mb.getData(), (int) mb.getSize());
    const auto a = p.readParams(), b = p2.readParams();
    check (std::abs (a.kPunchHz - b.kPunchHz) < 0.01f && std::abs (a.bCutoff - b.bCutoff) < 0.01f, "Parameter nach Neuladen gleich");

    ed.reset();
    std::printf ("%s\n", failures == 0 ? "ALLE TESTS OK" : "FEHLER");
    return failures;
}
