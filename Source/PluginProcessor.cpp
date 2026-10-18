#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
juce::String formatValue (const pf::PDef& d, float v)
{
    if (d.kind == pf::KInt)
    {
        if (std::string (d.id) == "g_note")
            return juce::String (pf::noteName ((int) v)) + juce::String (pf::noteOctave ((int) v));
        return juce::String ((int) v);
    }
    const float range = d.max - d.min;
    int decimals = range <= 2.0f ? 2 : (range <= 30.0f ? 1 : 0);
    if (std::abs (v) < 10.0f && range > 30.0f) decimals = 1;
    if (std::abs (v) >= 1000.0f && d.suffix != nullptr && std::string (d.suffix) == " Hz")
        return juce::String (v / 1000.0f, 2) + " kHz";
    // Achtung: juce::String (float, 0) bedeutet "volle Genauigkeit" -> ganze Zahlen selbst runden
    return (decimals == 0 ? juce::String (juce::roundToInt (v)) : juce::String (v, decimals)) + d.suffix;
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (auto& d : pf::paramTable())
    {
        const juce::ParameterID pid { d.id, 1 };
        switch (d.kind)
        {
            case pf::KChoice:
            {
                juce::StringArray items;
                for (auto& c : pf::choiceList (d)) items.add (juce::String::fromUTF8 (c.c_str()));
                layout.add (std::make_unique<juce::AudioParameterChoice> (pid, juce::String::fromUTF8 (d.name), items, (int) d.def));
                break;
            }
            case pf::KBool:
                layout.add (std::make_unique<juce::AudioParameterBool> (pid, juce::String::fromUTF8 (d.name), d.def > 0.5f));
                break;
            case pf::KInt:
            {
                const pf::PDef* dp = &d;
                layout.add (std::make_unique<juce::AudioParameterInt> (pid, juce::String::fromUTF8 (d.name), (int) d.min, (int) d.max, (int) d.def,
                    juce::AudioParameterIntAttributes().withStringFromValueFunction ([dp] (int v, int) { return formatValue (*dp, (float) v); })));
                break;
            }
            default:
            {
                juce::NormalisableRange<float> range (d.min, d.max);
                if (d.centre > d.min && d.centre < d.max) range.setSkewForCentre (d.centre);
                const pf::PDef* dp = &d;
                layout.add (std::make_unique<juce::AudioParameterFloat> (pid, juce::String::fromUTF8 (d.name), range, d.def,
                    juce::AudioParameterFloatAttributes()
                        .withStringFromValueFunction ([dp] (float v, int) { return formatValue (*dp, v); })
                        .withValueFromStringFunction ([] (const juce::String& s) { return s.retainCharacters ("0123456789.,-").replaceCharacter (',', '.').getFloatValue()
                                                                                         * (s.containsIgnoreCase ("khz") ? 1000.0f : 1.0f); })));
                break;
            }
        }
    }
    return layout;
}
} // namespace

//==============================================================================
PsyForgeProcessor::BusesProperties PsyForgeProcessor::makeBuses()
{
    return BusesProperties()
        .withOutput ("Main", juce::AudioChannelSet::stereo(), true)
        .withOutput ("Kick", juce::AudioChannelSet::stereo(), false)
        .withOutput ("Bass", juce::AudioChannelSet::stereo(), false);
}

PsyForgeProcessor::PsyForgeProcessor()
    : AudioProcessor (makeBuses()),
      apvts (*this, nullptr, "PsyForge", createLayout())
{
    for (auto& d : pf::paramTable())
        raw.push_back (apvts.getRawParameterValue (d.id));
    undoParams = pf::defaultParams();
}

PsyForgeProcessor::~PsyForgeProcessor()
{
    cancelAnalysis();
    if (analysisThread.joinable()) analysisThread.join();
}

bool PsyForgeProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.outputBuses.isEmpty()) return false;
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    for (int i = 1; i < layouts.outputBuses.size(); ++i)
    {
        const auto& set = layouts.outputBuses.getReference (i);
        if (! set.isDisabled() && set != juce::AudioChannelSet::stereo()) return false;
    }
    return layouts.inputBuses.isEmpty();
}

void PsyForgeProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    loopEvents.reserve (512);
    engine.prepare (sampleRate);
    engine.setParams (readParams(), getPreviewBpm(), 0.0f);
}

pf::SynthParams PsyForgeProcessor::readParams() const
{
    pf::SynthParams p;
    const auto& t = pf::paramTable();
    for (size_t i = 0; i < t.size(); ++i) p.*(t[i].field) = raw[i]->load();
    return p;
}

void PsyForgeProcessor::setParam (const juce::String& id, float value)
{
    if (auto* prm = apvts.getParameter (id))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 (value));
        prm->endChangeGesture();
    }
}

void PsyForgeProcessor::applyParams (const pf::SynthParams& p, const char* prefix, bool includeSession)
{
    for (auto& d : pf::paramTable())
    {
        if (prefix != nullptr && std::strncmp (d.id, prefix, std::strlen (prefix)) != 0) continue;
        if (! includeSession && pf::isSessionParam (d.id)) continue;
        setParam (d.id, p.*(d.field));
    }
    uiParamVersion.fetch_add (1);
}

double PsyForgeProcessor::getPreviewBpm() const
{
    const double h = hostBpm.load();
    if (h > 20.0) return h;
    return (double) apvts.getRawParameterValue ("g_bpm")->load();
}

//==============================================================================
void PsyForgeProcessor::handleMidi (const juce::MidiMessage& m, const pf::SynthParams& p)
{
    const int ch = m.getChannel();
    const bool kickCh = ch == (int) p.kickCh, bassCh = ch == (int) p.bassCh;
    if (m.isNoteOn())
    {
        if (kickCh) engine.kickOn (m.getFloatVelocity(), m.getNoteNumber());
        if (bassCh) engine.bassOn (m.getNoteNumber(), m.getFloatVelocity());
    }
    else if (m.isNoteOff())
    {
        if (bassCh) engine.bassOff (m.getNoteNumber());
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        engine.allOff();
    }
}

void PsyForgeProcessor::runLoop (double ppqStart, double ppqEnd, int numSamples, double bpm, int pattern, int note,
                                 std::vector<std::pair<int, int>>& events)
{
    if (ppqEnd <= ppqStart) return;
    const double spb = 60.0 / bpm * currentSampleRate;
    const auto steps = pf::bassStepsForPattern (pattern);
    for (double b = std::floor (ppqStart); b < ppqEnd; b += 1.0)
    {
        auto add = [&] (double pos, int type)
        {
            if (pos >= ppqStart && pos < ppqEnd)
                events.push_back ({ juce::jlimit (0, numSamples - 1, (int) ((pos - ppqStart) * spb)), type });
        };
        add (b, 0);
        for (auto& s : steps) { add (b + s.pos, 1 + (int) std::round (s.vel * 100.0f) * 10); add (b + s.pos + s.len, 2); }
    }
    (void) note;
}

void PsyForgeProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    // Tempo und Position
    double ppq = 0.0; bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto bpm = pos->getBpm()) hostBpm.store (*bpm);
            playing = pos->getIsPlaying();
            if (auto pq = pos->getPpqPosition()) ppq = *pq;
        }

    const pf::SynthParams p = readParams();
    const double bpm = getPreviewBpm();
    engine.setParams (p, bpm);

    // Ereignisse: MIDI + Vorschau-Schleife + Vorhör-Tasten
    loopEvents.clear();
    if (loopOn.load())
    {
        const double beats = numSamples * bpm / 60.0 / currentSampleRate;
        if (playing) runLoop (ppq, ppq + beats, numSamples, bpm, (int) p.pattern, (int) p.note, loopEvents);
        else { runLoop (freeBeat, freeBeat + beats, numSamples, bpm, (int) p.pattern, (int) p.note, loopEvents); freeBeat += beats; }
    }
    else freeBeat = 0.0;

    if (padKick.exchange (false)) loopEvents.push_back ({ 0, 0 });
    if (padBass.exchange (false))
    {
        loopEvents.push_back ({ 0, 1 + 100 * 10 });
        padBassOffLeft = (int) (15.0 / bpm * currentSampleRate * 0.7);
    }
    std::sort (loopEvents.begin(), loopEvents.end());

    auto* main = buffer.getWritePointer (0);
    auto* mainR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    auto kickBus = getBusBuffer (buffer, false, 1);
    auto bassBus = getBusBuffer (buffer, false, 2);
    const bool kickSeparate = getBusCount (false) > 1 && getBus (false, 1)->isEnabled() && kickBus.getNumChannels() > 0;
    const bool bassSeparate = getBusCount (false) > 2 && getBus (false, 2)->isEnabled() && bassBus.getNumChannels() > 0;

    auto midiIt = midi.begin();
    size_t li = 0;
    float kPeak = 0, bPeak = 0, oPeak = 0;

    // Referenz-Wiedergabe vorbereiten
    std::shared_ptr<const ReferenceSound> ref;
    {
        const juce::SpinLock::ScopedTryLockType lock (refLock);
        if (lock.isLocked() && refPlaying.load()) ref = reference;
    }
    double refPos = refPlayPos.load();
    const double refStep = ref ? ref->sampleRate / currentSampleRate : 1.0;

    for (int i = 0; i < numSamples; ++i)
    {
        while (midiIt != midi.end() && (*midiIt).samplePosition <= i)
        {
            handleMidi ((*midiIt).getMessage(), p);
            ++midiIt;
        }
        while (li < loopEvents.size() && loopEvents[li].first <= i)
        {
            const int type = loopEvents[li].second;
            if (type == 0) engine.kickOn (1.0f);
            else if (type == 2) { if (loopBassNote >= 0) engine.bassOff (loopBassNote); }
            else
            {
                loopBassNote = (int) p.note;
                engine.bassOn (loopBassNote, (float) ((type - 1) / 10) / 100.0f);
            }
            ++li;
        }
        if (padBassOffLeft > 0 && --padBassOffLeft == 0) engine.bassOff ((int) p.note);

        float k, b;
        engine.tick (k, b);
        k *= p.master; b *= p.master;
        kPeak = std::max (kPeak, std::abs (k)); bPeak = std::max (bPeak, std::abs (b));

        float m = (kickSeparate ? 0.0f : k) + (bassSeparate ? 0.0f : b);
        if (ref)
        {
            const size_t idx = (size_t) refPos;
            if (idx + 1 < ref->samples.size())
            {
                const float fr = (float) (refPos - (double) idx);
                m += (ref->samples[idx] * (1.0f - fr) + ref->samples[idx + 1] * fr) * 0.8f * p.master;
                refPos += refStep;
            }
            else { ref = nullptr; refPlaying.store (false); }
        }
        if (p.limiter > 0.5f)
        {
            const float a = std::abs (m);                     // linear bis 0,9, darüber weich gegen 1,0
            if (a > 0.9f) m = (m > 0.0f ? 1.0f : -1.0f) * (0.9f + 0.1f * std::tanh ((a - 0.9f) / 0.1f));
        }
        oPeak = std::max (oPeak, std::abs (m));
        main[i] = m;
        if (mainR) mainR[i] = m;
        if (kickSeparate) for (int c = 0; c < kickBus.getNumChannels(); ++c) kickBus.getWritePointer (c)[i] = k;
        if (bassSeparate) for (int c = 0; c < bassBus.getNumChannels(); ++c) bassBus.getWritePointer (c)[i] = b;
    }
    refPlayPos.store (refPos);

    uiKickLevel.store (std::max (kPeak, uiKickLevel.load() * 0.85f));
    uiBassLevel.store (std::max (bPeak, uiBassLevel.load() * 0.85f));
    uiOutLevel.store (std::max (oPeak, uiOutLevel.load() * 0.85f));
    midi.clear();
}

//==============================================================================
void PsyForgeProcessor::loadFactoryPreset (int index)
{
    const auto& list = pf::factoryPresets();
    if (index < 0 || index >= (int) list.size()) return;
    applyParams (pf::applyPreset (list[(size_t) index], readParams()));
    presetIndex = index;
    presetName = juce::String::fromUTF8 (list[(size_t) index].name);
}

juce::File PsyForgeProcessor::presetDirectory()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("PsyForge").getChildFile ("Presets");
}

juce::Array<juce::File> PsyForgeProcessor::listUserPresets() const
{
    auto files = presetDirectory().findChildFiles (juce::File::findFiles, false, "*.psyforge");
    files.sort();
    return files;
}

bool PsyForgeProcessor::saveUserPreset (const juce::String& name)
{
    auto dir = presetDirectory();
    if (! dir.createDirectory()) return false;
    juce::XmlElement xml ("PsyForgePreset");
    xml.setAttribute ("name", name);
    const auto p = readParams();
    for (auto& d : pf::paramTable())
        if (! pf::isSessionParam (d.id))
            xml.setAttribute (d.id, (double) (p.*(d.field)));
    const auto file = dir.getChildFile (juce::File::createLegalFileName (name) + ".psyforge");
    if (! xml.writeTo (file)) return false;
    presetIndex = -1; presetName = name;
    return true;
}

bool PsyForgeProcessor::loadUserPreset (const juce::File& file)
{
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || ! xml->hasTagName ("PsyForgePreset")) return false;
    pf::SynthParams p = pf::defaultParams();
    const auto cur = readParams();
    for (auto& d : pf::paramTable())
    {
        if (pf::isSessionParam (d.id)) p.*(d.field) = cur.*(d.field);
        else if (xml->hasAttribute (d.id)) p.*(d.field) = (float) xml->getDoubleAttribute (d.id);
    }
    applyParams (p);
    presetIndex = -1;
    presetName = xml->getStringAttribute ("name", file.getFileNameWithoutExtension());
    return true;
}

//==============================================================================
juce::String PsyForgeProcessor::suggestedFileName (int what) const
{
    const auto p = readParams();
    const int bpm = (int) std::round (getPreviewBpm());
    juce::String note;
    if (what == ExportKick)
    {
        const double midi = pf::hzToMidi (pf::kickEndHz (p), p.a4);
        const int n = (int) std::lround (midi);
        note = juce::String (pf::noteName (n)) + juce::String (pf::noteOctave (n));
    }
    else note = juce::String (pf::noteName ((int) p.note)) + juce::String (pf::noteOctave ((int) p.note));
    note = note.replace ("#", "s");
    const char* kind = what == ExportKick ? "Kick" : (what == ExportBass ? "Bass" : "Loop");
    auto preset = presetName.retainCharacters ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -_").replace (" ", "_");
    return "PsyForge_" + juce::String (kind) + "_" + preset + "_" + note + "_" + juce::String (bpm) + "bpm.wav";
}

juce::File PsyForgeProcessor::renderToFile (int what, const juce::File& target)
{
    const auto p = readParams();
    const double sr = getPreviewSampleRate();
    const double bpm = getPreviewBpm();
    pf::RenderOut r;
    if (what == ExportKick)      r = pf::renderKick (p, sr, bpm, 1.0f, 0.005f);
    else if (what == ExportBass) r = pf::renderBass (p, sr, bpm, (int) p.note);
    else                         r = pf::renderLoop (p, sr, bpm, 4, (int) p.note, (int) p.pattern);

    if (what != ExportLoop)
    {
        for (auto& s : r.audio) s *= p.master;
        // sauberes Ende
        const int fade = std::min ((int) r.audio.size(), (int) (sr * 0.002));
        for (int i = 0; i < fade; ++i) r.audio[r.audio.size() - 1 - (size_t) i] *= (float) i / (float) fade;
    }
    float peak = 0; for (float s : r.audio) peak = std::max (peak, std::abs (s));
    if (peak > 0.98f) for (auto& s : r.audio) s *= 0.98f / peak;

    juce::File file = target;
    if (file == juce::File())
        file = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("PsyForge").getChildFile (suggestedFileName (what));
    file.getParentDirectory().createDirectory();
    file.deleteFile();

    juce::AudioBuffer<float> buf (2, (int) r.audio.size());
    for (int c = 0; c < 2; ++c) buf.copyFrom (c, 0, r.audio.data(), (int) r.audio.size());
    juce::WavAudioFormat wav;
    auto os = file.createOutputStream();
    if (os == nullptr) return {};
    std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (os.get(), sr, 2, 24, {}, 0));
    if (writer == nullptr) return {};
    os.release();   // gehört jetzt dem Writer
    writer->writeFromAudioSampleBuffer (buf, 0, buf.getNumSamples());
    writer.reset();
    return file;
}

//==============================================================================
bool PsyForgeProcessor::loadReference (const juce::File& file, juce::String& error)
{
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (file));
    if (reader == nullptr) { error = "Datei kann nicht gelesen werden (WAV, AIFF oder FLAC verwenden)."; return false; }
    const auto maxLen = (juce::int64) (reader->sampleRate * 4.0);
    const int len = (int) std::min (reader->lengthInSamples, maxLen);
    if (len < 64) { error = "Datei ist zu kurz."; return false; }
    juce::AudioBuffer<float> buf ((int) reader->numChannels, len);
    reader->read (&buf, 0, len, 0, true, true);
    std::vector<float> mono ((size_t) len, 0.0f);
    for (int c = 0; c < buf.getNumChannels(); ++c)
        for (int i = 0; i < len; ++i) mono[(size_t) i] += buf.getSample (c, i) / (float) buf.getNumChannels();

    const pf::Reference r = pf::makeReference (mono, reader->sampleRate);
    if (r.x.size() < 64) { error = "Datei ist still oder zu kurz."; return false; }
    if ((double) r.x.size() / reader->sampleRate > 1.9) error = "Hinweis: nur die ersten 2 s werden verwendet – am besten ein einzelnes One-Shot laden.";

    auto rs = std::make_shared<ReferenceSound>();
    rs->name = file.getFileName();
    rs->samples = r.x;
    rs->sampleRate = reader->sampleRate;
    rs->type = pf::detectType (r) == pf::SoundKick ? 0 : 1;
    {
        const juce::SpinLock::ScopedLockType lock (refLock);
        reference = rs;
    }
    refPlaying.store (false);
    {
        std::lock_guard<std::mutex> g (resultMutex);
        analysisResult.reset();
    }
    analysisVersion.fetch_add (1);
    return true;
}

std::shared_ptr<const ReferenceSound> PsyForgeProcessor::getReference() const
{
    const juce::SpinLock::ScopedLockType lock (refLock);
    return reference;
}

void PsyForgeProcessor::clearReference()
{
    cancelAnalysis();
    const juce::SpinLock::ScopedLockType lock (refLock);
    reference.reset();
    refPlaying.store (false);
}

void PsyForgeProcessor::startAnalysis (int forceType, bool fine)
{
    auto ref = getReference();
    if (ref == nullptr || analysing.load()) return;
    if (analysisThread.joinable()) analysisThread.join();
    analysisCancel.store (false);
    analysing.store (true);
    analysisProgress.store (0.0f);
    analysisMatch.store (0.0f);
    const pf::SynthParams base = readParams();
    analysisThread = std::thread ([this, ref, forceType, fine, base]
    {
        pf::Reference r; r.x = ref->samples; r.sr = ref->sampleRate;
        pf::AnalysisJob job;
        job.forceType = forceType;
        job.fine = fine;
        job.cancel = &analysisCancel;
        job.budget = 6000;
        job.progress = [this] (float prog, float match) { analysisProgress.store (prog); analysisMatch.store (match); };
        auto res = std::make_unique<pf::AnalysisResult> (pf::analyzeAndFit (r, base, job));
        if (res->ok)
        {
            auto updated = std::make_shared<ReferenceSound> (*ref);
            updated->type = res->type == pf::SoundKick ? 0 : 1;
            const juce::SpinLock::ScopedLockType lock (refLock);
            if (reference == ref) reference = updated;
        }
        {
            std::lock_guard<std::mutex> g (resultMutex);
            analysisResult = std::move (res);
        }
        analysisProgress.store (1.0f);
        analysing.store (false);
        analysisVersion.fetch_add (1);
    });
}

void PsyForgeProcessor::cancelAnalysis()
{
    analysisCancel.store (true);
}

bool PsyForgeProcessor::hasAnalysisResult() const
{
    std::lock_guard<std::mutex> g (resultMutex);
    return analysisResult != nullptr && analysisResult->ok;
}

pf::AnalysisResult PsyForgeProcessor::getAnalysisResult() const
{
    std::lock_guard<std::mutex> g (resultMutex);
    return analysisResult != nullptr ? *analysisResult : pf::AnalysisResult();
}

void PsyForgeProcessor::applyAnalysisResult()
{
    const auto res = getAnalysisResult();
    if (! res.ok) return;
    undoParams = readParams();
    hasUndo = true;
    const char* prefix = res.type == pf::SoundKick ? "k_" : "b_";
    pf::SynthParams p = res.params;
    // Pegel nicht verändern
    if (res.type == pf::SoundKick) p.kLevel = undoParams.kLevel; else p.bLevel = undoParams.bLevel;
    applyParams (p, prefix);
    if (res.type == pf::SoundBass && res.detectedNote >= 12 && res.detectedNote <= 60)
        setParam ("g_note", (float) res.detectedNote);    // Vorschau-Note = Note des Originals
    presetIndex = -1;
    presetName = "Analyse: " + (getReference() ? getReference()->name.upToLastOccurrenceOf (".", false, false) : juce::String());
}

void PsyForgeProcessor::undoAnalysis()
{
    if (! hasUndo) return;
    applyParams (undoParams, nullptr, false);
    setParam ("g_note", undoParams.note);
    hasUndo = false;
}

//==============================================================================
void PsyForgeProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("uiWidth", uiWidth, nullptr);
    state.setProperty ("presetName", presetName, nullptr);
    state.setProperty ("presetIndex", presetIndex, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void PsyForgeProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            uiWidth = (int) tree.getProperty ("uiWidth", 1180);
            presetName = tree.getProperty ("presetName", "Init").toString();
            presetIndex = (int) tree.getProperty ("presetIndex", 0);
            apvts.replaceState (tree);
            uiParamVersion.fetch_add (1);
        }
}

juce::AudioProcessorEditor* PsyForgeProcessor::createEditor() { return new PsyForgeEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PsyForgeProcessor(); }
