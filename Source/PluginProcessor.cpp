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
    return (decimals == 0 ? juce::String (juce::roundToInt (v)) : juce::String (v, decimals)) + juce::String::fromUTF8 (d.suffix);
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (auto& d : pf::paramTable())
    {
        const juce::ParameterID pid { d.id, 1 };
        const auto name = juce::String::fromUTF8 (d.name);
        switch (d.kind)
        {
            case pf::KChoice:
            {
                juce::StringArray items;
                for (auto& c : pf::choiceList (d)) items.add (juce::String::fromUTF8 (c.c_str()));
                layout.add (std::make_unique<juce::AudioParameterChoice> (pid, name, items, (int) d.def));
                break;
            }
            case pf::KBool:
                layout.add (std::make_unique<juce::AudioParameterBool> (pid, name, d.def > 0.5f));
                break;
            case pf::KInt:
            {
                const pf::PDef* dp = &d;
                layout.add (std::make_unique<juce::AudioParameterInt> (pid, name, (int) d.min, (int) d.max, (int) d.def,
                    juce::AudioParameterIntAttributes().withStringFromValueFunction ([dp] (int v, int) { return formatValue (*dp, (float) v); })));
                break;
            }
            default:
            {
                juce::NormalisableRange<float> range (d.min, d.max);
                if (d.centre > d.min && d.centre < d.max) range.setSkewForCentre (d.centre);
                const pf::PDef* dp = &d;
                layout.add (std::make_unique<juce::AudioParameterFloat> (pid, name, range, d.def,
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
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
        .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false);
}

PsyForgeProcessor::PsyForgeProcessor()
    : AudioProcessor (makeBuses()),
      apvts (*this, nullptr, "PsyForge", createLayout())
{
    for (auto& d : pf::paramTable())
        raw.push_back (apvts.getRawParameterValue (d.id));
    undoParams = pf::defaultParams();
    for (int i = 0; i < pf::NumInstruments; ++i)
        for (auto& fp : pf::factoryPresets())
            if (fp.inst == i) { presetName[(size_t) i] = juce::String::fromUTF8 (fp.name); break; }
    blockEvents.reserve (256);
    duckEvents.reserve (64);
    rebuildSequence();
    startTimerHz (15);
}

PsyForgeProcessor::~PsyForgeProcessor()
{
    stopTimer();
    cancelAnalysis();
    if (analysisThread.joinable()) analysisThread.join();
}

bool PsyForgeProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono()) return false;
    for (const auto& in : layouts.inputBuses)
        if (! in.isDisabled() && in != juce::AudioChannelSet::stereo() && in != juce::AudioChannelSet::mono()) return false;
    return true;
}

void PsyForgeProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    synth.prepare (sampleRate);
    synth.setParams (readParams(), getPreviewBpm());
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
    return (double) getParam ("g_bpm");
}

//==============================================================================
// Sequenz (Pattern oder Clip) – wird im Message-Thread gebaut und im Audio-Thread nur gelesen
pf::Groove PsyForgeProcessor::currentGroove (const pf::SynthParams& p, double bpm) const
{
    pf::Groove g; g.swing = p.swing; g.humanize = p.humanize; g.bpm = bpm;
    return g;
}

pf::Pattern PsyForgeProcessor::currentPattern (const pf::SynthParams& p, bool fullTrack) const
{
    if (fullTrack) return pf::makeArrangement ((int) p.style, (int) p.bassRhy, (int) p.variation);
    return pf::makePattern ((int) p.style, (int) p.section, (int) p.bassRhy, (int) p.variation);
}

std::shared_ptr<SeqData> PsyForgeProcessor::buildSequence (const pf::SynthParams& p, double bpm) const
{
    auto d = std::make_shared<SeqData>();
    const int inst = juce::jlimit (0, pf::NumInstruments - 1, (int) p.inst);
    d->inst = inst;
    if (p.source > 0.5f && clip != nullptr)
    {
        d->fromClip = true;
        d->loopBeats = clip->lengthBeats;
        for (const auto& n : clip->notes)
            if (pf::clipNoteMatches (*clip, n, inst, clipFilter))
                d->events.push_back ({ n.start, n.length, n.note, n.vel });
    }
    else
    {
        const auto pat = currentPattern (p, false);
        const auto g = currentGroove (p, bpm);
        d->loopBeats = (double) pat.lengthTicks() / pf::PPQ;
        for (size_t i = 0; i < pat.events.size(); ++i)
        {
            const auto& e = pat.events[i];
            if (e.voice != inst) continue;
            float vel = e.vel;
            const double t = pf::grooveTick (pat, i, 0, g, vel);
            d->events.push_back ({ t / pf::PPQ, (double) e.length / pf::PPQ, pf::noteForEvent (e, (int) p.key), vel });
        }
    }
    std::sort (d->events.begin(), d->events.end(), [] (const SeqEvent& a, const SeqEvent& b) { return a.beat < b.beat; });
    return d;
}

void PsyForgeProcessor::rebuildSequence()
{
    const auto p = readParams();
    const double bpm = getPreviewBpm();
    auto d = buildSequence (p, bpm);
    std::shared_ptr<const SeqData> old;
    {
        const juce::SpinLock::ScopedLockType lock (seqLock);
        old = sequence;
        sequence = d;
    }
    if (old) retired.push_back (old);     // wird später freigegeben, nie im Audio-Thread
    lastSeqParams = p;
    lastSeqBpm = bpm;
    seqDirty = false;
}

std::shared_ptr<const SeqData> PsyForgeProcessor::getSequence() const
{
    const juce::SpinLock::ScopedLockType lock (seqLock);
    return sequence;
}

void PsyForgeProcessor::timerCallback()
{
    const auto p = readParams();
    const double bpm = getPreviewBpm();
    auto ne = [] (float a, float b) { return std::abs (a - b) > 1.0e-6f; };
    const auto& q = lastSeqParams;
    const bool changed = ne (p.inst, q.inst) || ne (p.style, q.style) || ne (p.section, q.section) || ne (p.bassRhy, q.bassRhy)
                      || ne (p.variation, q.variation) || ne (p.swing, q.swing) || ne (p.humanize, q.humanize) || ne (p.key, q.key)
                      || ne (p.source, q.source) || std::abs (bpm - lastSeqBpm) > 0.5;
    if (changed || seqDirty) rebuildSequence();
    // alte Sequenzen freigeben, sobald der Audio-Thread sie sicher nicht mehr hält
    if (retired.size() > 1) retired.erase (retired.begin(), retired.end() - 1);
}

//==============================================================================
void PsyForgeProcessor::triggerNote (int note, float vel, int gate, int, double, double, double)
{
    synth.noteOn (note, vel, gate);
}

void PsyForgeProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    if (numSamples == 0) return;

    // Sidechain lesen, bevor der Puffer gelöscht wird
    float scPeakBlock = 0.0f;
    std::array<float, 2> scDummy {};
    juce::ignoreUnused (scDummy);
    const pf::SynthParams p = readParams();
    const int inst = juce::jlimit (0, pf::NumInstruments - 1, (int) p.inst);
    const bool useSidechain = inst == pf::InBass && (int) p.bDuckSrc == 3 && getBusCount (true) > 0 && getBus (true, 0)->isEnabled();
    duckEvents.clear();
    if (useSidechain)
    {
        auto sc = getBusBuffer (buffer, true, 0);
        const float rel = std::exp (-1.0f / (0.02f * (float) currentSampleRate));
        for (int i = 0; i < numSamples; ++i)
        {
            float x = 0.0f;
            for (int c = 0; c < sc.getNumChannels(); ++c) x = std::max (x, std::abs (sc.getSample (c, i)));
            scEnv = std::max (x, scEnv * rel);
            scPeakBlock = std::max (scPeakBlock, scEnv);
            if (scArmed && scEnv > 0.2f) { duckEvents.push_back ({ i, 0 }); scArmed = false; }
            if (! scArmed && scEnv < 0.05f) scArmed = true;
        }
    }
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
    hostPlaying.store (playing);
    const double bpm = getPreviewBpm();
    synth.setParams (p, bpm);
    const double spb = 60.0 / bpm * currentSampleRate;               // Samples pro Beat
    const double beats = numSamples / spb;
    const double blockMs = juce::Time::getMillisecondCounterHiRes();

    // Ereignisse dieses Blocks sammeln: Spur-MIDI, Pattern/Clip, Vorhören
    blockEvents.clear();
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isNoteOn())
        {
            if ((int) p.midiFilter == 1 && ! pf::gmAccepts (inst, m.getNoteNumber())) continue;
            blockEvents.push_back ({ meta.samplePosition, m.getNoteNumber(), m.getFloatVelocity(), -1 });
        }
        else if (m.isNoteOff())
            blockEvents.push_back ({ meta.samplePosition, m.getNoteNumber(), -1.0f, 0 });
        else if (m.isAllNotesOff() || m.isAllSoundOff())
            blockEvents.push_back ({ meta.samplePosition, -1, -2.0f, 0 });
    }
    midi.clear();

    const bool seqOn = p.seqOn > 0.5f;
    const bool playSeq = (seqOn && playing) || previewOn.load();
    double seqPos = playing ? ppq : freeBeat;
    if (playSeq)
    {
        std::shared_ptr<const SeqData> seq;
        {
            const juce::SpinLock::ScopedTryLockType lock (seqLock);
            if (lock.isLocked()) seq = sequence;
        }
        if (seq != nullptr && seq->loopBeats > 0.0)
        {
            const double L = seq->loopBeats;
            const double b0 = seqPos, b1 = seqPos + beats;
            const auto k0 = (long long) std::floor (b0 / L), k1 = (long long) std::floor (b1 / L);
            for (auto k = k0; k <= k1; ++k)
                for (const auto& e : seq->events)
                {
                    const double at = e.beat + (double) k * L;
                    if (at < b0) continue;
                    if (at >= b1) break;
                    const int pos = juce::jlimit (0, numSamples - 1, (int) ((at - b0) * spb));
                    blockEvents.push_back ({ pos, e.note, e.vel, juce::jmax (1, (int) (e.len * spb)) });
                }
            uiBeat.store (std::fmod (b1, L));
        }
        if (! playing) freeBeat += beats;
        wasPlayingSeq = true;
    }
    else
    {
        if (wasPlayingSeq) { synth.allOff(); wasPlayingSeq = false; }
        freeBeat = 0.0;
        uiBeat.store (-1.0);
    }
    if (padHit.exchange (false))
        blockEvents.push_back ({ 0, pf::previewNote (p, inst), 1.0f, inst == pf::InBass ? (int) (spb / 4 * 0.7) : -1 });
    std::stable_sort (blockEvents.begin(), blockEvents.end(), [] (const BlockEv& a, const BlockEv& b) { return a.pos < b.pos; });

    // Bass-Ducking: Kick-Instanz (Link), Viertel-Raster oder Sidechain
    if (inst == pf::InBass && p.bDuck > 0.0f)
    {
        const int src = (int) p.bDuckSrc;
        auto& link = KickLink::get();
        const bool linkAlive = blockMs - link.lastMs.load() < 2000.0;
        if (src == 1 && linkAlive)
        {
            const uint32_t w = link.written.load();
            if (w - linkRead > (uint32_t) KickLink::N) linkRead = w - KickLink::N;
            for (; linkRead != w; ++linkRead)
            {
                const uint32_t i = linkRead % KickLink::N;
                const double kp = link.ppq[i].load(), km = link.ms[i].load();
                double pos = (playing && kp > -1.0e8) ? (kp - ppq) * spb : (km - blockMs) * currentSampleRate / 1000.0;
                if (pos < -currentSampleRate * 0.25 || pos >= numSamples) continue;
                duckEvents.push_back ({ juce::jmax (0, (int) pos), juce::jmax (0, (int) -pos) });
            }
        }
        else
        {
            linkRead = link.written.load();
            // Viertel-Raster (auch als Ersatz, solange keine Kick-Instanz spielt)
            if ((src == 2 || src == 1) && (playing || playSeq))
            {
                const double b0 = playing ? ppq : seqPos;
                for (double b = std::ceil (b0 - 1e-9); b < b0 + beats; b += 1.0)
                    duckEvents.push_back ({ juce::jlimit (0, numSamples - 1, (int) ((b - b0) * spb)), 0 });
            }
        }
        std::sort (duckEvents.begin(), duckEvents.end(), [] (const DuckEv& a, const DuckEv& b) { return a.pos < b.pos; });
    }

    // Rendern
    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    size_t ei = 0, di = 0;
    float peak = 0.0f;
    int hits = 0;
    std::shared_ptr<const ReferenceSound> ref;
    {
        const juce::SpinLock::ScopedTryLockType lock (refLock);
        if (lock.isLocked() && refPlaying.load()) ref = reference;
    }
    double refPos = refPlayPos.load();
    const double refStep = ref ? ref->sampleRate / currentSampleRate : 1.0;
    auto& link = KickLink::get();

    for (int i = 0; i < numSamples; ++i)
    {
        while (ei < blockEvents.size() && blockEvents[ei].pos <= i)
        {
            const auto& e = blockEvents[ei++];
            if (e.vel < -1.5f) synth.allOff();
            else if (e.vel < 0.0f) synth.noteOff (e.note);
            else
            {
                synth.noteOn (e.note, e.vel, e.gate);
                ++hits;
                if (inst == pf::InKick)
                    link.publish (playing ? ppq + i / spb : -1.0e9, blockMs + i * 1000.0 / currentSampleRate);
            }
        }
        while (di < duckEvents.size() && duckEvents[di].pos <= i) { synth.duckTrigger (duckEvents[di].ago); ++di; }

        float s = synth.tick() * p.master;
        if (ref)
        {
            const size_t idx = (size_t) refPos;
            if (idx + 1 < ref->samples.size())
            {
                const float fr = (float) (refPos - (double) idx);
                s += (ref->samples[idx] * (1.0f - fr) + ref->samples[idx + 1] * fr) * 0.8f * p.master;
                refPos += refStep;
            }
            else { ref = nullptr; refPlaying.store (false); }
        }
        if (p.limiter > 0.5f)
        {
            const float a = std::abs (s);                     // linear bis 0,9, darüber weich gegen 1,0
            if (a > 0.9f) s = (s > 0.0f ? 1.0f : -1.0f) * (0.9f + 0.1f * std::tanh ((a - 0.9f) / 0.1f));
        }
        peak = std::max (peak, std::abs (s));
        L[i] = s;
        if (R) R[i] = s;
    }
    refPlayPos.store (refPos);
    uiLevel.store (std::max (peak, uiLevel.load() * 0.85f));
    if (hits > 0) uiHits.fetch_add (hits);
    if (! duckEvents.empty()) uiDuck.store (1.0f); else uiDuck.store (uiDuck.load() * 0.8f);
    juce::ignoreUnused (scPeakBlock);
}

//==============================================================================
void PsyForgeProcessor::loadFactoryPreset (const pf::FactoryPreset& fp)
{
    applyParams (pf::applyPreset (fp, readParams()), pf::instrumentPrefix (fp.inst));
    presetName[(size_t) juce::jlimit (0, pf::NumInstruments - 1, fp.inst)] = juce::String::fromUTF8 (fp.name);
    uiParamVersion.fetch_add (1);
}

juce::File PsyForgeProcessor::presetDirectory()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("PsyForge").getChildFile ("Presets");
}

juce::Array<juce::File> PsyForgeProcessor::listUserPresets() const
{
    auto dir = presetDirectory().getChildFile (pf::instrumentName (instrument()));
    auto files = dir.findChildFiles (juce::File::findFiles, false, "*.psyforge");
    files.sort();
    return files;
}

bool PsyForgeProcessor::saveUserPreset (const juce::String& name)
{
    const int inst = instrument();
    auto dir = presetDirectory().getChildFile (pf::instrumentName (inst));
    if (! dir.createDirectory()) return false;
    juce::XmlElement xml ("PsyForgePreset");
    xml.setAttribute ("name", name);
    xml.setAttribute ("instrument", pf::instrumentName (inst));
    const auto p = readParams();
    for (auto& d : pf::paramTable())
        if (pf::belongsTo (d.id, inst))
            xml.setAttribute (d.id, (double) (p.*(d.field)));
    const auto file = dir.getChildFile (juce::File::createLegalFileName (name) + ".psyforge");
    if (! xml.writeTo (file)) return false;
    presetName[(size_t) inst] = name;
    return true;
}

bool PsyForgeProcessor::loadUserPreset (const juce::File& file)
{
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || ! xml->hasTagName ("PsyForgePreset")) return false;
    const int inst = instrument();
    pf::SynthParams p = readParams();
    const auto def = pf::defaultParams();
    for (auto& d : pf::paramTable())
        if (pf::belongsTo (d.id, inst))
            p.*(d.field) = xml->hasAttribute (d.id) ? (float) xml->getDoubleAttribute (d.id) : def.*(d.field);
    applyParams (p, pf::instrumentPrefix (inst));
    presetName[(size_t) inst] = xml->getStringAttribute ("name", file.getFileNameWithoutExtension());
    return true;
}

//==============================================================================
juce::String PsyForgeProcessor::suggestedWavName (int what) const
{
    const auto p = readParams();
    const int inst = instrument();
    const int bpm = (int) std::round (getPreviewBpm());
    juce::String note;
    if (inst == pf::InKick)
    {
        const int n = (int) std::lround (pf::hzToMidi (pf::kickEndHz (p), p.a4));
        note = juce::String (pf::noteName (n)) + juce::String (pf::noteOctave (n));
    }
    else if (inst == pf::InBass) note = juce::String (pf::noteName ((int) p.note)) + juce::String (pf::noteOctave ((int) p.note));
    note = note.replace ("#", "s");
    auto preset = getPresetName().retainCharacters ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -_").replace (" ", "_");
    juce::String name = "PsyForge_" + juce::String (pf::instrumentName (inst)).removeCharacters (" ") + "_" + preset;
    if (note.isNotEmpty()) name += "_" + note;
    if (what == WavLoop)
    {
        const auto sec = juce::StringArray { "Intro", "Buildup", "Drop", "Offbeat", "FullEnergy", "Festival", "Breakdown", "Drop2", "End" };
        name += "_" + (p.source > 0.5f ? juce::String ("Clip") : sec[(int) p.section]) + "_" + juce::String (bpm) + "bpm";
    }
    return name + ".wav";
}

juce::File PsyForgeProcessor::renderWav (int what, const juce::File& target)
{
    pf::SynthParams p = readParams();
    const int inst = instrument();
    const double sr = getPreviewSampleRate();
    const double bpm = getPreviewBpm();
    std::vector<float> audio;
    if (what == WavHit)
    {
        audio = pf::renderInstrument (p, inst, sr, bpm).audio;
        for (auto& s : audio) s *= p.master;
        const int fade = std::min ((int) audio.size(), (int) (sr * 0.002));
        for (int i = 0; i < fade; ++i) audio[audio.size() - 1 - (size_t) i] *= (float) i / (float) fade;
    }
    else
    {
        // Pattern/Clip als Loop (1 Durchgang, Ausklang wird an den Anfang gefaltet -> nahtlos loopbar)
        const auto seq = buildSequence (p, bpm);
        const double spb = 60.0 / bpm * sr;
        const int n = (int) (seq->loopBeats * spb);
        pf::Synth s; s.prepare (sr); s.setParams (p, bpm);
        std::vector<float> out ((size_t) n * 2, 0.0f);
        size_t ei = 0;
        for (int i = 0; i < 2 * n; ++i)
        {
            const double beat = std::fmod ((double) i / spb, seq->loopBeats);
            const int loop = i / n;
            while (loop == 0 && ei < seq->events.size() && seq->events[ei].beat * spb <= i)
            {
                const auto& e = seq->events[ei++];
                s.noteOn (e.note, e.vel, juce::jmax (1, (int) (e.len * spb)));
            }
            if (inst == pf::InBass && p.bDuck > 0.0f && p.bDuckSrc > 0.5f && std::fmod (beat, 1.0) * spb < 1.0) s.duckTrigger (0);
            out[(size_t) i] = s.tick() * p.master;
        }
        audio.assign (out.begin(), out.begin() + n);
        for (int i = 0; i < n; ++i) audio[(size_t) i] += out[(size_t) (n + i)];
    }
    float peak = 0; for (float s : audio) peak = std::max (peak, std::abs (s));
    if (peak > 0.98f) for (auto& s : audio) s *= 0.98f / peak;
    if (audio.empty()) audio.push_back (0.0f);

    juce::File file = target;
    if (file == juce::File())
        file = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("PsyForge").getChildFile (suggestedWavName (what));
    file.getParentDirectory().createDirectory();
    file.deleteFile();

    juce::AudioBuffer<float> buf (2, (int) audio.size());
    for (int c = 0; c < 2; ++c) buf.copyFrom (c, 0, audio.data(), (int) audio.size());
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

juce::File PsyForgeProcessor::writeMidi (int what)
{
    const auto p = readParams();
    const int inst = instrument();
    const double bpm = getPreviewBpm();
    const auto g = currentGroove (p, bpm);
    const auto styles = juce::StringArray { "OldschoolGoa", "NeoGoa", "FullOn", "Progressive", "Psygressive", "Darkpsy", "HiTech", "ProgBR" };
    const auto secs = juce::StringArray { "Intro", "Buildup", "Drop", "Offbeat", "FullEnergy", "Festival", "Breakdown", "Drop2", "End" };
    juce::MidiFile mf;
    juce::String name;
    const juce::String instName = juce::String (pf::instrumentName (inst)).removeCharacters (" ");
    switch (what)
    {
        case MidiClipOut:
            if (clip == nullptr) return {};
            mf = pf::clipToMidiFile (*clip, inst, clipFilter, bpm);
            name = "PsyForge_" + instName + "_" + clip->name;
            break;
        case MidiFullTrack:
            mf = pf::toMidiFile (currentPattern (p, true), (int) p.key, bpm, inst, g);
            name = "PsyForge_" + instName + "_" + styles[(int) p.style] + "_FullTrack";
            break;
        case MidiAllSection:
            mf = pf::toMidiFile (currentPattern (p, false), (int) p.key, bpm, -1, g);
            name = "PsyForge_Alle_" + styles[(int) p.style] + "_" + secs[(int) p.section];
            break;
        case MidiAllFullTrack:
            mf = pf::toMidiFile (currentPattern (p, true), (int) p.key, bpm, -1, g);
            name = "PsyForge_Alle_" + styles[(int) p.style] + "_FullTrack";
            break;
        case MidiSection:
        default:
            mf = pf::toMidiFile (currentPattern (p, false), (int) p.key, bpm, inst, g);
            name = "PsyForge_" + instName + "_" + styles[(int) p.style] + "_" + secs[(int) p.section];
            break;
    }
    auto file = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("PsyForge")
                    .getChildFile (juce::File::createLegalFileName (name) + ".mid");
    file.getParentDirectory().createDirectory();
    file.deleteFile();
    juce::FileOutputStream os (file);
    if (! os.openedOk() || ! mf.writeTo (os)) return {};
    return file;
}

bool PsyForgeProcessor::exportAllSections (const juce::File& folder)
{
    if (! folder.createDirectory()) return false;
    auto p = readParams();
    const double bpm = getPreviewBpm();
    const auto g = currentGroove (p, bpm);
    const auto secs = juce::StringArray { "Intro", "Buildup", "Drop", "Offbeat", "FullEnergy", "Festival", "Breakdown", "Drop2", "End" };
    for (int s = 0; s < pf::NumSections; ++s)
    {
        const auto pat = pf::makePattern ((int) p.style, s, (int) p.bassRhy, (int) p.variation);
        const auto mf = pf::toMidiFile (pat, (int) p.key, bpm, instrument(), g);
        auto f = folder.getChildFile (juce::String (s + 1).paddedLeft ('0', 2) + "_" + secs[s] + "_"
                                      + juce::String (pf::instrumentName (instrument())).removeCharacters (" ") + ".mid");
        f.deleteFile();
        juce::FileOutputStream os (f);
        if (! os.openedOk() || ! mf.writeTo (os)) return false;
    }
    return true;
}

//==============================================================================
bool PsyForgeProcessor::loadClip (const juce::File& file, juce::String& error)
{
    pf::MidiClip c;
    if (! pf::loadMidiClip (file, c, error)) return false;
    clip = std::make_shared<const pf::MidiClip> (std::move (c));
    clipFilter = pf::ClipAuto;
    setParam ("g_source", 1.0f);
    seqDirty = true;
    rebuildSequence();
    return true;
}

void PsyForgeProcessor::clearClip()
{
    clip.reset();
    setParam ("g_source", 0.0f);
    seqDirty = true;
    rebuildSequence();
}

//==============================================================================
bool PsyForgeProcessor::loadReference (const juce::File& file, juce::String& error)
{
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (file));
    if (reader == nullptr) { error = juce::String::fromUTF8 ("Datei kann nicht gelesen werden (WAV, AIFF oder FLAC verwenden)."); return false; }
    const auto maxLen = (juce::int64) (reader->sampleRate * 8.0);
    const int len = (int) std::min (reader->lengthInSamples, maxLen);
    if (len < 64) { error = juce::String::fromUTF8 ("Datei ist zu kurz."); return false; }
    juce::AudioBuffer<float> buf ((int) reader->numChannels, len);
    reader->read (&buf, 0, len, 0, true, true);
    std::vector<float> mono ((size_t) len, 0.0f);
    for (int c = 0; c < buf.getNumChannels(); ++c)
        for (int i = 0; i < len; ++i) mono[(size_t) i] += buf.getSample (c, i) / (float) buf.getNumChannels();

    const pf::Reference r = pf::makeReference (mono, reader->sampleRate);
    if (r.x.size() < 64) { error = juce::String::fromUTF8 ("Datei ist still oder zu kurz."); return false; }
    const int inst = instrument();
    error.clear();
    if ((double) r.x.size() / reader->sampleRate > 2.5 && inst != pf::InRiser && inst != pf::InRide)
        error = juce::String::fromUTF8 ("Hinweis: sehr lang – am besten ein einzelnes One-Shot laden.");
    if (inst == pf::InKick || inst == pf::InBass)
    {
        const auto t = pf::detectType (r);
        if ((t == pf::SoundKick) != (inst == pf::InKick))
            error = juce::String::fromUTF8 ("Hinweis: klingt eher nach ") + (t == pf::SoundKick ? "Kick" : "Bass")
                  + juce::String::fromUTF8 (" – diese Instanz ist aber auf ") + pf::instrumentName (inst) + " gestellt.";
    }

    auto rs = std::make_shared<ReferenceSound>();
    rs->name = file.getFileName();
    rs->samples = r.x;
    rs->sampleRate = reader->sampleRate;
    rs->inst = inst;
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

void PsyForgeProcessor::startAnalysis (bool fine)
{
    auto ref = getReference();
    if (ref == nullptr || analysing.load()) return;
    if (analysisThread.joinable()) analysisThread.join();
    analysisCancel.store (false);
    analysing.store (true);
    analysisProgress.store (0.0f);
    analysisMatch.store (0.0f);
    const pf::SynthParams base = readParams();
    const int inst = instrument();
    analysisThread = std::thread ([this, ref, fine, base, inst]
    {
        pf::Reference r; r.x = ref->samples; r.sr = ref->sampleRate;
        pf::AnalysisJob job;
        job.fine = fine;
        job.cancel = &analysisCancel;
        job.budget = 6000;
        job.progress = [this] (float prog, float match) { analysisProgress.store (prog); analysisMatch.store (match); };
        auto res = std::make_unique<pf::AnalysisResult> (pf::analyzeInstrument (r, base, inst, job));
        {
            std::lock_guard<std::mutex> g (resultMutex);
            analysisResult = std::move (res);
        }
        analysisProgress.store (1.0f);
        analysing.store (false);
        analysisVersion.fetch_add (1);
    });
}

void PsyForgeProcessor::cancelAnalysis() { analysisCancel.store (true); }

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
    const int inst = res.inst;
    pf::SynthParams p = res.params;
    // Pegel nicht verändern
    for (auto& d : pf::paramTable())
        if (pf::belongsTo (d.id, inst) && std::string (d.id).find ("_level") != std::string::npos)
            p.*(d.field) = undoParams.*(d.field);
    if (inst != instrument()) setParam ("g_inst", (float) inst);
    applyParams (p, pf::instrumentPrefix (inst));
    if (inst == pf::InBass && res.detectedNote >= 12 && res.detectedNote <= 72)
        setParam ("g_note", (float) res.detectedNote);    // Vorschau-Note = Note des Originals
    presetName[(size_t) inst] = "Analyse: " + (getReference() ? getReference()->name.upToLastOccurrenceOf (".", false, false) : juce::String());
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
    juce::StringArray names;
    for (auto& n : presetName) names.add (n);
    state.setProperty ("presetNames", names.joinIntoString ("\n"), nullptr);
    state.setProperty ("clipFilter", clipFilter, nullptr);
    if (clip != nullptr)
    {
        // Clip im Projekt speichern (als MIDI-Datei, Base64)
        juce::MidiFile mf;
        mf.setTicksPerQuarterNote (pf::ExportPPQ);
        std::map<int, juce::MidiMessageSequence> tracks;
        for (const auto& n : clip->notes)
        {
            auto& seq = tracks[n.track];
            seq.addEvent (juce::MidiMessage::noteOn (1, n.note, n.vel), n.start * pf::ExportPPQ);
            seq.addEvent (juce::MidiMessage::noteOff (1, n.note), (n.start + n.length) * pf::ExportPPQ - 1);
        }
        const int numTracks = clip->trackNames.size();
        for (int t = 0; t < numTracks; ++t)
        {
            juce::MidiMessageSequence seq = tracks[t];
            auto nm = juce::MidiMessage::textMetaEvent (3, clip->trackNames[t].isNotEmpty() ? clip->trackNames[t] : " ");
            nm.setTimeStamp (0); seq.addEvent (nm);
            auto end = juce::MidiMessage::endOfTrack(); end.setTimeStamp (clip->lengthBeats * pf::ExportPPQ); seq.addEvent (end);
            seq.sort(); seq.updateMatchedPairs();
            mf.addTrack (seq);
        }
        juce::MemoryOutputStream mo;
        mf.writeTo (mo);
        state.setProperty ("clipName", clip->name, nullptr);
        state.setProperty ("clipData", mo.getMemoryBlock().toBase64Encoding(), nullptr);
    }
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void PsyForgeProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            uiWidth = (int) tree.getProperty ("uiWidth", 1180);
            const auto names = juce::StringArray::fromLines (tree.getProperty ("presetNames", "").toString());
            for (int i = 0; i < pf::NumInstruments && i < names.size(); ++i) if (names[i].isNotEmpty()) presetName[(size_t) i] = names[i];
            clipFilter = (int) tree.getProperty ("clipFilter", (int) pf::ClipAuto);
            const auto clipData = tree.getProperty ("clipData", "").toString();
            if (clipData.isNotEmpty())
            {
                juce::MemoryBlock mb;
                if (mb.fromBase64Encoding (clipData))
                {
                    auto tmp = juce::File::createTempFile (".mid");
                    tmp.replaceWithData (mb.getData(), mb.getSize());
                    pf::MidiClip c; juce::String err;
                    if (pf::loadMidiClip (tmp, c, err))
                    {
                        c.name = tree.getProperty ("clipName", "Clip").toString();
                        clip = std::make_shared<const pf::MidiClip> (std::move (c));
                    }
                    tmp.deleteFile();
                }
            }
            tree.removeProperty ("clipData", nullptr);
            apvts.replaceState (tree);
            uiParamVersion.fetch_add (1);
            seqDirty = true;
        }
}

juce::AudioProcessorEditor* PsyForgeProcessor::createEditor() { return new PsyForgeEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PsyForgeProcessor(); }
