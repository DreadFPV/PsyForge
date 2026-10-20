// PsyForge – Integrationstest mit JUCE (ohne DAW): Instrument-Wahl, Spur-MIDI, Drum-Map-Filter, Pattern-Wiedergabe,
// Kick->Bass-Ducking zwischen zwei Instanzen, MIDI-Clip-Import, MIDI/WAV-Export, Analyse und Bildschirmfotos.
// Aufruf: PsyForgeUiTest <ausgabeordner> [snare_referenz.wav]
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

// n Blöcke verarbeiten, Energie zurückgeben; events: {Block, Sample, MIDI}
static double run (PsyForgeProcessor& p, int blocks, const std::vector<std::tuple<int, int, juce::MidiMessage>>& events = {}, int bs = 480)
{
    juce::AudioBuffer<float> buf (2, bs);
    double e = 0;
    for (int b = 0; b < blocks; ++b)
    {
        juce::MidiBuffer midi;
        for (auto& [blk, pos, m] : events) if (blk == b) midi.addEvent (m, pos);
        buf.clear();
        p.processBlock (buf, midi);
        for (int i = 0; i < bs; ++i) e += buf.getSample (0, i) * buf.getSample (0, i);
    }
    return e;
}

static void setInst (PsyForgeProcessor& p, int inst) { p.setParam ("g_inst", (float) inst); }

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File out (argc > 1 ? argv[1] : "/tmp/psyforge-test");
    out.createDirectory();
    int failures = 0;
    auto check = [&] (bool ok, const char* what) { std::printf ("%s %s\n", ok ? "[OK]  " : "[FAIL]", what); if (! ok) ++failures; };
    const double sr = 48000.0; const int bs = 480;

    // 1) Kick-Instanz: jede Note auf jedem Kanal spielt die Kick
    PsyForgeProcessor kick;
    kick.setRateAndBufferSizeDetails (sr, bs); kick.prepareToPlay (sr, bs);
    setInst (kick, pf::InKick);
    check (kick.instrument() == pf::InKick, "Instrument Kick gewählt");
    const double eKick = run (kick, 20, { { 0, 0, juce::MidiMessage::noteOn (5, 60, (juce::uint8) 110) } });
    check (eKick > 1.0, "Kick klingt bei beliebiger Note/Kanal (Kanal 5, C3)");

    // 2) Snare-Instanz mit Drum-Map-Filter: C1 (Kick) still, D1 (Snare) klingt
    PsyForgeProcessor snare;
    snare.setRateAndBufferSizeDetails (sr, bs); snare.prepareToPlay (sr, bs);
    setInst (snare, pf::InSnare);
    snare.setParam ("g_midif", 1.0f);
    const double eWrong = run (snare, 20, { { 0, 0, juce::MidiMessage::noteOn (1, 36, (juce::uint8) 110) } });
    const double eRight = run (snare, 20, { { 0, 0, juce::MidiMessage::noteOn (1, 38, (juce::uint8) 110) } });
    check (eWrong < 1e-6 && eRight > 1.0, "Drum-Map-Filter: Snare ignoriert C1, spielt D1");

    // 3) Kick -> Bass-Ducking über zwei Instanzen (Link)
    PsyForgeProcessor bass;
    bass.setRateAndBufferSizeDetails (sr, bs); bass.prepareToPlay (sr, bs);
    setInst (bass, pf::InBass);
    bass.setParam ("b_ducksrc", 1.0f);
    {
        juce::AudioBuffer<float> kb (2, bs), bb (2, bs);
        bool ducked = false;
        for (int b = 0; b < 40; ++b)
        {
            juce::MidiBuffer km, bm;
            if (b % 10 == 0) km.addEvent (juce::MidiMessage::noteOn (1, 36, (juce::uint8) 120), 100);
            if (b % 10 == 1) bm.addEvent (juce::MidiMessage::noteOn (1, 30, (juce::uint8) 120), 0);
            if (b % 10 == 3) bm.addEvent (juce::MidiMessage::noteOff (1, 30), 0);
            kick.processBlock (kb, km);
            bass.processBlock (bb, bm);
            if (bass.uiDuck.load() > 0.5f) ducked = true;
        }
        check (ducked, "Bass-Instanz duckt auf die Kick-Instanz (Link)");
    }

    // 4) Pattern-Wiedergabe (Vorhören) spielt die Noten des Instruments
    {
        snare.setParam ("g_midif", 0.0f);
        snare.setParam ("g_section", (float) pf::Buildup);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (200);   // Timer baut die Sequenz
        auto seq = snare.getSequence();
        check (seq != nullptr && seq->inst == pf::InSnare && seq->events.size() > 20, "Sequenz: Snare-Roll im Buildup");
        const int before = snare.uiHits.load();
        snare.previewOn.store (true);
        run (snare, (int) (sr * 2.0 / bs));
        snare.previewOn.store (false);
        check (snare.uiHits.load() - before > 5, "Vorhören spielt das Pattern");
    }

    // 5) MIDI-Export
    {
        const auto f1 = bass.writeMidi (PsyForgeProcessor::MidiSection);
        const auto f2 = bass.writeMidi (PsyForgeProcessor::MidiAllSection);
        const auto f3 = bass.writeMidi (PsyForgeProcessor::MidiAllFullTrack);
        juce::MidiFile m2; { juce::FileInputStream in (f2); m2.readFrom (in); }
        check (f1.existsAsFile() && f1.getSize() > 100, "MIDI-Export: Section (Bass)");
        check (m2.getNumTracks() >= 6, "MIDI-Export: alle Instrumente als eigene Spuren");
        check (f3.existsAsFile() && f3.getSize() > f2.getSize(), "MIDI-Export: Full Track alle Instrumente");
        std::printf ("  %s\n  %s\n", f1.getFileName().toRawUTF8(), f2.getFileName().toRawUTF8());
    }

    // 6) MIDI-Clip: Datei mit Kick- und Bass-Spur; Bass-Instanz wählt automatisch die Bass-Spur
    {
        juce::MidiFile mf; mf.setTicksPerQuarterNote (960);
        juce::MidiMessageSequence k, b;
        auto nk = juce::MidiMessage::textMetaEvent (3, "Kick"); nk.setTimeStamp (0); k.addEvent (nk);
        auto nb = juce::MidiMessage::textMetaEvent (3, "Psy Bass"); nb.setTimeStamp (0); b.addEvent (nb);
        for (int beat = 0; beat < 8; ++beat)
        {
            k.addEvent (juce::MidiMessage::noteOn (1, 36, (juce::uint8) 120), beat * 960.0);
            k.addEvent (juce::MidiMessage::noteOff (1, 36), beat * 960.0 + 200);
            for (int s = 1; s < 4; ++s)
            {
                b.addEvent (juce::MidiMessage::noteOn (1, 30 + (s == 3 ? 12 : 0), (juce::uint8) 100), beat * 960.0 + s * 240);
                b.addEvent (juce::MidiMessage::noteOff (1, 30 + (s == 3 ? 12 : 0)), beat * 960.0 + s * 240 + 160);
            }
        }
        k.updateMatchedPairs(); b.updateMatchedPairs();
        mf.addTrack (k); mf.addTrack (b);
        const auto file = out.getChildFile ("clip_test.mid");
        file.deleteFile();
        { juce::FileOutputStream os (file); mf.writeTo (os); }
        juce::String err;
        check (bass.loadClip (file, err), "MIDI-Clip laden");
        auto seq = bass.getSequence();
        check (seq != nullptr && seq->fromClip && seq->events.size() == 24, "Clip: Bass-Instanz nimmt automatisch die Bass-Spur (24 Noten)");
        const auto fc = bass.writeMidi (PsyForgeProcessor::MidiClipOut);
        check (fc.existsAsFile(), "Clip als MIDI exportieren");
        // Clip bleibt im Projekt gespeichert
        juce::MemoryBlock mb; bass.getStateInformation (mb);
        PsyForgeProcessor b2; b2.setStateInformation (mb.getData(), (int) mb.getSize());
        check (b2.getClip() != nullptr && b2.getClip()->notes.size() == bass.getClip()->notes.size() && b2.instrument() == pf::InBass, "Clip + Instrument im Projekt gespeichert");
    }

    // 7) WAV-Export
    for (int inst : { pf::InKick, pf::InSnare, pf::InRide })
    {
        setInst (kick, inst);
        const auto f = kick.renderWav (PsyForgeProcessor::WavHit, out.getChildFile (juce::String ("hit_") + juce::String (inst) + ".wav"));
        const auto l = kick.renderWav (PsyForgeProcessor::WavLoop, out.getChildFile (juce::String ("loop_") + juce::String (inst) + ".wav"));
        check (f.getSize() > 1000 && l.getSize() > 10000, (juce::String ("WAV-Export Anschlag + Loop: ") + pf::instrumentName (inst)).toRawUTF8());
    }
    setInst (kick, pf::InKick);

    // 8) Analyse (Snare-Referenz)
    if (argc > 2)
    {
        juce::String err;
        check (snare.loadReference (juce::File (argv[2]), err), "Referenz-WAV laden (Snare)");
        snare.startAnalysis (true);
        const auto t0 = juce::Time::getMillisecondCounter();
        while (snare.isAnalysing()) juce::Thread::sleep (200);
        const auto res = snare.getAnalysisResult();
        std::printf ("  Analyse %s: %.1f %% (grob %.1f %%) in %.1f s\n", pf::instrumentName (res.inst), res.match, res.initialMatch,
                     (juce::Time::getMillisecondCounter() - t0) / 1000.0);
        check (res.ok && res.inst == pf::InSnare && res.match > 50.0f, "Analyse Snare liefert Ergebnis");
        snare.applyAnalysisResult();
        check (snare.canUndoAnalysis(), "Übernehmen + Rückgängig");
    }

    // 9) Oberfläche: je Instrument ein Bild + MIDI-CLIP und ANALYSE
    {
        std::unique_ptr<juce::AudioProcessorEditor> ed (bass.createEditor());
        ed->setSize (PsyForgeEditor::baseW, PsyForgeEditor::baseH);
        ed->setVisible (true);
        bass.setParam ("g_source", 0.0f);
        bass.setParam ("g_section", (float) pf::Drop);
        pump (500);
        snapshot (*ed, out.getChildFile ("ui_bass.png"));
        for (int inst : { pf::InKick, pf::InSnare, pf::InRide, pf::InPerc })
        {
            setInst (bass, inst);
            pump (400);
            snapshot (*ed, out.getChildFile (juce::String ("ui_") + juce::String (pf::instrumentName (inst)).removeCharacters (" ").toLowerCase() + ".png"));
        }
        setInst (bass, pf::InBass);
        bass.setParam ("g_source", 1.0f);
        if (auto* b = findButton (*ed, "MIDI-CLIP")) { b->triggerClick(); pump (400); }
        snapshot (*ed, out.getChildFile ("ui_clip.png"));
        if (auto* b = findButton (*ed, "ANALYSE")) { b->triggerClick(); pump (300); }
        snapshot (*ed, out.getChildFile ("ui_analyse.png"));
        ed->setSize (PsyForgeEditor::baseW * 7 / 10, PsyForgeEditor::baseH * 7 / 10);
        pump (300);
        snapshot (*ed, out.getChildFile ("ui_small.png"));
    }

    std::printf ("%s\n", failures == 0 ? "ALLE TESTS OK" : "FEHLER");
    return failures;
}
