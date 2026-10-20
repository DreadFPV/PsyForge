#include "PluginEditor.h"
#include <map>

namespace pfui
{
const juce::Colour bgCol      { 0xff0b0714 };
const juce::Colour panelCol   { 0xff161026 };
const juce::Colour panelEdge  { 0xff2c2148 };
const juce::Colour textCol    { 0xffe8e0ff };
const juce::Colour dimText    { 0xff8d80b0 };
const juce::Colour accent     { 0xffff2bd6 };
const juce::Colour accent2    { 0xff20e3ff };
const juce::Colour kickCol    { 0xffb6ff3b };
const juce::Colour bassCol    { 0xffe01fff };
const juce::Colour refCol     { 0xffffa63d };
const juce::Colour loopCol    { 0xff20e3ff };
constexpr int cellW = 64, cellH = 92, headerH = 20;

static juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }

//==============================================================================
PfLookAndFeel::PfLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, panelCol);
    setColour (juce::ComboBox::textColourId, textCol);
    setColour (juce::ComboBox::outlineColourId, panelEdge);
    setColour (juce::ComboBox::arrowColourId, accent2);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1a1230));
    setColour (juce::PopupMenu::textColourId, textCol);
    setColour (juce::PopupMenu::headerTextColourId, accent2);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, accent.withAlpha (0.35f));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::TextButton::buttonColourId, panelCol);
    setColour (juce::TextButton::buttonOnColourId, accent);
    setColour (juce::TextButton::textColourOffId, textCol);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    setColour (juce::ToggleButton::textColourId, textCol);
    setColour (juce::ToggleButton::tickColourId, accent2);
    setColour (juce::Label::textColourId, textCol);
    setColour (juce::Slider::textBoxTextColourId, textCol);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, accent.withAlpha (0.4f));
    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff1a1230));
    setColour (juce::TextEditor::textColourId, textCol);
    setColour (juce::TextEditor::outlineColourId, panelEdge);
    setColour (juce::TextEditor::focusedOutlineColourId, accent2);
    setColour (juce::CaretComponent::caretColourId, accent2);
    setColour (juce::AlertWindow::backgroundColourId, panelCol);
    setColour (juce::AlertWindow::textColourId, textCol);
    setColour (juce::AlertWindow::outlineColourId, panelEdge);
    setColour (juce::ProgressBar::backgroundColourId, panelCol);
    setColour (juce::ProgressBar::foregroundColourId, accent);
    setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xf01a1230));
    setColour (juce::TooltipWindow::textColourId, textCol);
    setColour (juce::TooltipWindow::outlineColourId, panelEdge);
}

void PfLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider& s)
{
    const auto col = s.findColour (juce::Slider::rotarySliderFillColourId);
    const auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
    const float r = std::min (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float ang = start + pos * (end - start);

    juce::Path track; track.addCentredArc (c.x, c.y, r - 2.0f, r - 2.0f, 0.0f, start, end, true);
    g.setColour (panelEdge);
    g.strokePath (track, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    juce::Path val; val.addCentredArc (c.x, c.y, r - 2.0f, r - 2.0f, 0.0f, start, ang, true);
    g.setColour (col);
    g.strokePath (val, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour (juce::Colour (0xff1e1636));
    g.fillEllipse (c.x - r + 6.0f, c.y - r + 6.0f, (r - 6.0f) * 2.0f, (r - 6.0f) * 2.0f);
    const juce::Point<float> tip (c.x + (r - 8.0f) * std::sin (ang), c.y - (r - 8.0f) * std::cos (ang));
    g.setColour (textCol);
    g.drawLine (c.x + (r * 0.25f) * std::sin (ang), c.y - (r * 0.25f) * std::cos (ang), tip.x, tip.y, 2.0f);
}

void PfLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    auto base = on ? b.findColour (juce::TextButton::buttonOnColourId) : panelCol;
    if (over) base = base.brighter (0.12f);
    if (down) base = base.darker (0.2f);
    g.setColour (base);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (on ? base.brighter (0.4f) : panelEdge.brighter (over ? 0.4f : 0.0f));
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
}

void PfLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (panelCol);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (box.hasKeyboardFocus (true) ? accent2 : panelEdge);
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
    juce::Path arrow;
    const float ax = (float) w - 12.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (accent2);
    g.fillPath (arrow);
}

void PfLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool)
{
    auto r = b.getLocalBounds().toFloat();
    auto box = r.removeFromLeft (std::min (r.getHeight(), 18.0f)).withSizeKeepingCentre (14.0f, 14.0f);
    g.setColour (b.getToggleState() ? accent2 : panelCol.brighter (over ? 0.2f : 0.0f));
    g.fillRoundedRectangle (box, 3.0f);
    g.setColour (panelEdge.brighter (0.4f));
    g.drawRoundedRectangle (box, 3.0f, 1.0f);
    g.setColour (textCol);
    g.setFont (juce::FontOptions (12.5f));
    g.drawFittedText (b.getButtonText(), r.toNearestInt().withTrimmedLeft (4), juce::Justification::centredLeft, 2);
}

juce::Label* PfLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (juce::FontOptions (11.5f));
    l->setJustificationType (juce::Justification::centred);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::backgroundWhenEditingColourId, juce::Colour (0xff1a1230));
    l->setColour (juce::Label::outlineWhenEditingColourId, accent2);
    return l;
}

//==============================================================================
ParamControl::ParamControl (PsyForgeProcessor& p, const pf::PDef& d, juce::Colour c) : def (d), colour (c)
{
    label.setText (u8 (d.name), juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (11.5f, juce::Font::bold));
    label.setColour (juce::Label::textColourId, dimText.brighter (0.3f));
    label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (label);
    const auto tip = u8 (d.help);

    if (d.kind == pf::KChoice)
    {
        combo = std::make_unique<juce::ComboBox>();
        combo->setLookAndFeel (&lnf.get());
        int i = 1;
        for (auto& ch : pf::choiceList (d)) combo->addItem (juce::String::fromUTF8 (ch.c_str()), i++);
        combo->setTooltip (tip);
        addAndMakeVisible (*combo);
        cAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, d.id, *combo);
    }
    else if (d.kind == pf::KBool)
    {
        toggle = std::make_unique<juce::ToggleButton> ("");
        toggle->setLookAndFeel (&lnf.get());
        toggle->setTooltip (tip);
        addAndMakeVisible (*toggle);
        bAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, d.id, *toggle);
    }
    else
    {
        slider = std::make_unique<juce::Slider>();
        slider->setLookAndFeel (&lnf.get());
        slider->setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        slider->setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        slider->setColour (juce::Slider::textBoxTextColourId, textCol);
        if (d.kind == pf::KInt)
        {
            slider->setSliderStyle (juce::Slider::IncDecButtons);
            slider->setTextBoxStyle (juce::Slider::TextBoxLeft, false, 40, 20);
            slider->setIncDecButtonsMode (juce::Slider::incDecButtonsDraggable_Vertical);
        }
        else
        {
            slider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, cellW, 16);
        }
        slider->setColour (juce::Slider::rotarySliderFillColourId, c);
        slider->setTooltip (tip);
        slider->setPopupDisplayEnabled (false, false, nullptr);
        addAndMakeVisible (*slider);
        sAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, d.id, *slider);
        slider->setDoubleClickReturnValue (true, (double) d.def);
    }
}

ParamControl::~ParamControl()
{
    if (slider) slider->setLookAndFeel (nullptr);
    if (combo) combo->setLookAndFeel (nullptr);
    if (toggle) toggle->setLookAndFeel (nullptr);
}

void ParamControl::paint (juce::Graphics&) {}

void ParamControl::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (14));
    if (combo)       combo->setBounds (r.withSizeKeepingCentre (r.getWidth() - 2, std::min (24, r.getHeight())));
    else if (toggle) toggle->setBounds (r.withSizeKeepingCentre (22, 22));
    else if (slider)
    {
        if (def.kind == pf::KInt) slider->setBounds (r.withSizeKeepingCentre (r.getWidth() - 2, std::min (24, r.getHeight())));
        else slider->setBounds (r);
    }
}

//==============================================================================
Section::Section (PsyForgeProcessor& p, const juce::String& t, std::initializer_list<const char*> ids, juce::Colour c)
    : title (t), colour (c)
{
    for (auto* id : ids)
        if (auto* d = pf::findParam (id))
            addAndMakeVisible (controls.add (new ParamControl (p, *d, c)));
}

int Section::preferredWidth() const { return controls.size() * cellW + 16; }

void Section::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (panelCol);
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (panelEdge);
    g.drawRoundedRectangle (r, 8.0f, 1.0f);
    g.setColour (colour);
    g.fillRoundedRectangle (r.getX() + 10.0f, r.getY() + 7.0f, 3.0f, 10.0f, 1.5f);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText (title, (int) r.getX() + 18, (int) r.getY() + 3, (int) r.getWidth() - 20, 18, juce::Justification::centredLeft);
}

void Section::resized()
{
    auto r = getLocalBounds().reduced (8, 4);
    r.removeFromTop (headerH - 2);
    for (auto* c : controls) c->setBounds (r.removeFromLeft (cellW).withHeight (cellH));
}

void Page::resized()
{
    int x = 0, y = 0;
    const int h = cellH + headerH + 10, gap = 8;
    for (auto* s : sections)
    {
        const int w = s->preferredWidth();
        if (x > 0 && x + w > getWidth()) { x = 0; y += h + gap; }
        s->setBounds (x, y, w, h);
        x += w + gap;
    }
}

Section::Section (PsyForgeProcessor& p, const juce::String& t, const std::vector<const char*>& ids, juce::Colour c)
    : title (t), colour (c)
{
    for (auto* id : ids)
        if (auto* d = pf::findParam (id))
            addAndMakeVisible (controls.add (new ParamControl (p, *d, c)));
}

juce::Colour instColour (int inst)
{
    switch (inst)
    {
        case pf::InKick:  return kickCol;
        case pf::InBass:  return bassCol;
        case pf::InSnare: return juce::Colour (0xffffa63d);
        case pf::InClap:  return juce::Colour (0xffff6b6b);
        case pf::InCHat:  return juce::Colour (0xff20e3ff);
        case pf::InOHat:  return juce::Colour (0xff5fa8ff);
        case pf::InRide:  return juce::Colour (0xff9d7bff);
        case pf::InPerc:  return juce::Colour (0xffffd23f);
        case pf::InZap:   return juce::Colour (0xffff5ea8);
        case pf::InRiser: return juce::Colour (0xff4dffd2);
        default:          return accent;
    }
}

//==============================================================================
WaveView::WaveView (PsyForgeProcessor& p) : proc (p)
{
    setTooltip (u8 ("Ein Anschlag des Instruments. Blaue Linie = Tonhöhe (Hz rechts), gestrichelt = Hüllkurve, orange = importiertes Original.\n"
                    "Zum Exportieren einfach in die DAW ziehen."));
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    startTimerHz (20);
}

void WaveView::timerCallback()
{
    const auto p = proc.readParams();
    const double bpm = proc.getPreviewBpm();
    const auto ref = proc.getReference();
    if (std::memcmp (&p, &lastParams, sizeof (p)) != 0 || std::abs (bpm - lastBpm) > 0.01 || ref != lastRef) dirty = true;
    if (! dirty) return;
    dirty = false;
    lastParams = p; lastBpm = bpm; lastRef = ref;
    inst = juce::jlimit (0, pf::NumInstruments - 1, (int) p.inst);
    render = pf::renderInstrument (p, inst, 44100.0, bpm);
    const double ms = render.audio.size() / 44.1;
    switch (inst)
    {
        case pf::InKick:
        {
            const auto k = pf::makeKickShape (p, bpm);
            const double midi = pf::hzToMidi (k.endHz, p.a4);
            const int n = (int) std::lround (midi);
            info = u8 ("Grundton ") + juce::String (k.endHz, 1) + " Hz (" + pf::noteName (n) + juce::String (pf::noteOctave (n))
                 + juce::String ((midi - n) * 100.0 >= 0 ? " +" : " ") + juce::String ((int) std::lround ((midi - n) * 100.0)) + " ct)"
                 + u8 (" · Punch ") + juce::String ((int) p.kPunchHz) + u8 (" Hz · Länge ") + juce::String ((int) std::round (k.len * 1000.0f)) + " ms";
            break;
        }
        case pf::InBass:
            info = u8 ("Note ") + pf::noteName ((int) p.note) + juce::String (pf::noteOctave ((int) p.note)) + u8 (" · ")
                 + juce::String (pf::bassNoteHz (p, (int) p.note), 1) + u8 (" Hz · Note-Länge ") + juce::String ((int) std::round (15000.0 / bpm * 0.7)) + " ms (1/16)";
            break;
        default:
            info = juce::String (pf::instrumentName (inst)) + u8 (" · Länge ") + juce::String ((int) std::round (ms)) + " ms";
            break;
    }
    repaint();
}

void WaveView::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (panelCol);
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (panelEdge);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 8.0f, 1.0f);

    const bool pitched = inst == pf::InKick || inst == pf::InZap || inst == pf::InRiser;
    auto plot = bounds.reduced (12.0f, 0.0f).withTrimmedTop (26.0f).withTrimmedBottom (20.0f).withTrimmedRight (pitched ? 44.0f : 0.0f);
    const auto col = instColour (inst);
    const double sr = render.sr;
    const auto ref = (showReference && lastRef != nullptr && lastRef->inst == inst) ? lastRef : nullptr;
    double dur = render.audio.size() / sr;
    if (ref) dur = std::max (dur, ref->samples.size() / ref->sampleRate);
    dur = std::max (dur, 0.01);
    auto xOf = [&] (double t) { return plot.getX() + (float) (t / dur) * plot.getWidth(); };
    const float midY = plot.getCentreY(), halfH = plot.getHeight() * 0.46f;

    // Raster
    g.setFont (juce::FontOptions (10.0f));
    {
        const double step = dur > 3.0 ? 0.5 : dur > 1.2 ? 0.2 : dur > 0.6 ? 0.1 : (dur > 0.25 ? 0.05 : 0.025);
        for (double t = 0.0; t <= dur + 1e-9; t += step)
        {
            const float x = xOf (t);
            g.setColour (panelEdge);
            g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
            g.setColour (dimText);
            g.drawText (juce::String ((int) std::round (t * 1000.0)) + " ms", (int) x - 25, (int) plot.getBottom() + 3, 50, 14, juce::Justification::centred);
        }
        const double sixteenth = 15.0 / lastBpm;
        int k = 1;
        for (const char* lbl : { "1/16", "1/8", "3/16", "1/4" })
        {
            if (k * sixteenth >= dur || dur > 1.2 || (k > 2 && dur > 0.6)) break;
            const float x = xOf (k * sixteenth);
            g.setColour (accent.withAlpha (0.45f));
            for (float y = plot.getY(); y < plot.getBottom(); y += 6.0f) g.drawVerticalLine ((int) x, y, std::min (y + 3.0f, plot.getBottom()));
            g.drawText (lbl, (int) x + 3, (int) plot.getY() + 2, 40, 12, juce::Justification::centredLeft);
            ++k;
        }
    }
    g.setColour (panelEdge.brighter (0.3f));
    g.drawHorizontalLine ((int) midY, plot.getX(), plot.getRight());

    auto drawWave = [&] (const std::vector<float>& x, double rate, juce::Colour c, bool filled)
    {
        if (x.empty()) return;
        const int W = (int) plot.getWidth();
        float peak = 1e-6f; for (float v : x) peak = std::max (peak, std::abs (v));
        std::vector<std::pair<float, float>> mm ((size_t) W);
        for (int px = 0; px < W; ++px)
        {
            const double t0 = (double) px / W * dur, t1 = (double) (px + 1) / W * dur;
            const size_t a = (size_t) (t0 * rate), b = std::min (x.size(), (size_t) (t1 * rate) + 1);
            float lo = 0, hi = 0;
            for (size_t i = a; i < b; ++i) { lo = std::min (lo, x[i]); hi = std::max (hi, x[i]); }
            mm[(size_t) px] = { lo / peak, hi / peak };
        }
        juce::Path fill;
        for (int px = 0; px < W; ++px)
        {
            const float X = plot.getX() + (float) px, Y = midY - mm[(size_t) px].second * halfH;
            if (px == 0) fill.startNewSubPath (X, Y); else fill.lineTo (X, Y);
        }
        for (int px = W - 1; px >= 0; --px) fill.lineTo (plot.getX() + (float) px, midY - mm[(size_t) px].first * halfH);
        fill.closeSubPath();
        if (filled)
        {
            g.setGradientFill (juce::ColourGradient (c.withAlpha (0.85f), 0, plot.getY(), c.withAlpha (0.35f), 0, midY, false));
            g.fillPath (fill);
            g.setColour (c);
            g.strokePath (fill, juce::PathStrokeType (0.8f));
        }
        else
        {
            g.setColour (c.withAlpha (0.9f));
            g.strokePath (fill, juce::PathStrokeType (1.2f));
        }
    };
    drawWave (render.audio, sr, col, true);
    if (ref) drawWave (ref->samples, ref->sampleRate, refCol, false);

    // Hüllkurve
    if (! render.amp.empty())
    {
        juce::Path env;
        const int W = (int) plot.getWidth();
        float ampMax = 1e-6f; for (float a : render.amp) ampMax = std::max (ampMax, a);
        bool started = false;
        for (int px = 0; px < W; px += 2)
        {
            const size_t i = (size_t) ((double) px / W * dur * sr);
            if (i >= render.amp.size()) break;
            const float y = midY - render.amp[i] / ampMax * halfH;
            if (! started) { env.startNewSubPath (plot.getX() + (float) px, y); started = true; } else env.lineTo (plot.getX() + (float) px, y);
        }
        juce::Path dashed;
        const float dashes[] = { 4.0f, 4.0f };
        juce::PathStrokeType (1.0f).createDashedStroke (dashed, env, dashes, 2);
        g.setColour (textCol.withAlpha (0.55f));
        g.fillPath (dashed);
    }

    // Tonhöhe auf log. Skala rechts
    const float fLo = 20.0f, fHi = 16000.0f;
    auto yOfHz = [&] (float f) { return plot.getBottom() - (float) (std::log (f / fLo) / std::log (fHi / fLo)) * plot.getHeight(); };
    if (pitched && ! render.freq.empty())
    {
        g.setFont (juce::FontOptions (10.0f));
        for (float f : { 30.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 3000.0f, 8000.0f })
        {
            const float y = yOfHz (f);
            g.setColour (accent2.withAlpha (0.8f));
            g.drawText (f >= 1000.0f ? juce::String ((int) (f / 1000.0f)) + "k" : juce::String ((int) f), (int) plot.getRight() + 8, (int) y - 6, 36, 12,
                        juce::Justification::centredLeft);
        }
        juce::Path pc;
        const int W = (int) plot.getWidth();
        bool started = false;
        for (int px = 0; px < W; ++px)
        {
            const size_t i = (size_t) ((double) px / W * dur * sr);
            if (i >= render.freq.size() || render.freq[i] <= 0.0f) continue;
            const float y = juce::jlimit (plot.getY(), plot.getBottom(), yOfHz (render.freq[i]));
            if (! started) { pc.startNewSubPath (plot.getX() + (float) px, y); started = true; } else pc.lineTo (plot.getX() + (float) px, y);
        }
        g.setColour (accent2);
        g.strokePath (pc, juce::PathStrokeType (1.8f));
    }

    g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
    g.setColour (col);
    g.drawText (info, 14, 5, getWidth() - 260, 18, juce::Justification::centredLeft);
    g.setFont (juce::FontOptions (11.0f));
    g.setColour (dimText);
    juce::String right = u8 ("⇢ als WAV in die DAW ziehen");
    if (ref) right = u8 ("Original: ") + ref->name + u8 ("   ·   ") + right;
    g.drawText (right, getWidth() - 440, 5, 426, 18, juce::Justification::centredRight);

    if (hoverX >= plot.getX() && hoverX <= plot.getRight())
    {
        const double t = (hoverX - plot.getX()) / plot.getWidth() * dur;
        g.setColour (textCol.withAlpha (0.4f));
        g.drawVerticalLine ((int) hoverX, plot.getY(), plot.getBottom());
        juce::String s = juce::String (t * 1000.0, 1) + " ms";
        const size_t i = (size_t) (t * sr);
        if (pitched && i < render.freq.size() && render.freq[i] > 0.0f) s += u8 (" · ") + juce::String ((int) std::round (render.freq[i])) + " Hz";
        if (i < render.amp.size() && render.amp[i] > 0.0f) s += u8 (" · ") + juce::String (20.0f * std::log10 (render.amp[i]), 1) + " dB";
        g.setFont (juce::FontOptions (11.0f));
        const int tx = (int) juce::jlimit (plot.getX(), plot.getRight() - 160.0f, hoverX + 6.0f);
        g.setColour (juce::Colour (0xe01a1230));
        g.fillRoundedRectangle ((float) tx, plot.getBottom() - 20.0f, 160.0f, 16.0f, 3.0f);
        g.setColour (textCol);
        g.drawText (s, tx + 4, (int) plot.getBottom() - 20, 156, 16, juce::Justification::centredLeft);
    }
}

void WaveView::mouseMove (const juce::MouseEvent& e) { hoverX = (float) e.x; repaint(); }
void WaveView::mouseExit (const juce::MouseEvent&) { hoverX = -1; repaint(); }
void WaveView::mouseDown (const juce::MouseEvent&) { dragStarted = false; }

void WaveView::mouseDrag (const juce::MouseEvent& e)
{
    if (dragStarted || e.getDistanceFromDragStart() < 8) return;
    dragStarted = true;
    const auto file = proc.renderWav (PsyForgeProcessor::WavHit);
    if (file.existsAsFile())
        juce::DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this);
}

//==============================================================================
void NoteRoll::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (bgCol.brighter (0.05f));
    g.fillRoundedRectangle (b, 6.0f);
    g.setColour (panelEdge);
    g.drawRoundedRectangle (b.reduced (0.5f), 6.0f, 1.0f);
    const int inst = proc.instrument();
    const auto col = instColour (inst);

    // Was zeichnen? Sequenz (Pattern/Clip gefiltert) oder ganze Clip-Datei
    struct N { double beat, len; int note; float vel; bool on; };
    std::vector<N> notes;
    double loop = 16.0;
    auto clip = proc.getClip();
    if (showClipAll && clip != nullptr)
    {
        loop = clip->lengthBeats;
        for (const auto& n : clip->notes)
            notes.push_back ({ n.start, n.length, n.note, n.vel, pf::clipNoteMatches (*clip, n, inst, proc.getClipFilter()) });
    }
    else if (auto seq = proc.getSequence())
    {
        loop = seq->loopBeats;
        for (const auto& e : seq->events) notes.push_back ({ e.beat, e.len, e.note, e.vel, true });
    }
    loop = std::max (1.0, loop);
    auto area = b.reduced (6.0f, 5.0f);

    // Takt-/Beat-Linien
    for (int beat = 0; beat <= (int) loop; ++beat)
    {
        const float x = area.getX() + (float) (beat / loop) * area.getWidth();
        g.setColour (beat % 4 == 0 ? panelEdge.brighter (0.6f) : panelEdge.withAlpha (0.6f));
        if (loop <= 64 || beat % 4 == 0) g.drawVerticalLine ((int) x, area.getY(), area.getBottom());
    }

    if (notes.empty())
    {
        g.setColour (dimText);
        g.setFont (juce::FontOptions (11.5f));
        g.drawText (showClipAll ? u8 ("Kein MIDI-Clip geladen") : u8 ("Keine Noten für ") + pf::instrumentName (inst) + u8 (" in dieser Section"),
                    getLocalBounds(), juce::Justification::centred);
    }
    else
    {
        int lo = 127, hi = 0;
        for (auto& n : notes) { lo = std::min (lo, n.note); hi = std::max (hi, n.note); }
        const bool oneLane = hi - lo < 1;
        const float laneH = oneLane ? area.getHeight() : area.getHeight() / (float) (hi - lo + 1);
        for (auto& n : notes)
        {
            const float x = area.getX() + (float) (n.beat / loop) * area.getWidth();
            const float w = std::max (2.0f, (float) (n.len / loop) * area.getWidth() - 1.0f);
            if (oneLane)
            {
                const float h = area.getHeight() * (0.3f + 0.7f * n.vel);
                g.setColour ((n.on ? col : dimText).withAlpha (n.on ? 0.95f : 0.35f));
                g.fillRect (x, area.getBottom() - h, std::max (2.0f, std::min (w, 6.0f)), h);
            }
            else
            {
                const float y = area.getBottom() - (float) (n.note - lo + 1) * laneH;
                g.setColour ((n.on ? col : dimText).withAlpha (n.on ? (0.45f + 0.55f * n.vel) : 0.3f));
                g.fillRoundedRectangle (x, y + 0.5f, w, std::max (2.0f, laneH - 1.0f), 1.5f);
            }
        }
    }
    const double beat = proc.uiBeat.load();
    if (beat >= 0.0 && ! showClipAll)
    {
        const float x = area.getX() + (float) (std::fmod (beat, loop) / loop) * area.getWidth();
        g.setColour (textCol);
        g.drawVerticalLine ((int) x, b.getY() + 2.0f, b.getBottom() - 2.0f);
    }
}

//==============================================================================
void DragChip::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    const bool over = isMouseOver();
    g.setColour (colour.withAlpha (over ? 0.32f : 0.18f));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (colour.withAlpha (over ? 1.0f : 0.7f));
    g.drawRoundedRectangle (r, 6.0f, 1.2f);
    g.setColour (textCol);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawFittedText (u8 ("⇩ ") + label, getLocalBounds().reduced (6, 0), juce::Justification::centred, 1);
}

void DragChip::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 6) return;
    dragging = true;
    const auto f = makeFile();
    if (f.existsAsFile())
        juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
}

void DragChip::mouseUp (const juce::MouseEvent& e)
{
    if (! dragging && e.getDistanceFromDragStart() < 6 && onClick) onClick();
}

//==============================================================================
ClipPage::ClipPage (PsyForgeProcessor& p) : proc (p), roll (p)
{
    roll.showClipAll = true;
    roll.setTooltip (u8 ("Alle Noten der MIDI-Datei. Farbig = Noten, die dieses Instrument spielt; grau = andere."));
    addAndMakeVisible (roll);
    addAndMakeVisible (loadButton);
    addAndMakeVisible (clearButton);
    addAndMakeVisible (filterBox);
    filterLabel.setFont (juce::FontOptions (11.5f, juce::Font::bold));
    filterLabel.setColour (juce::Label::textColourId, dimText.brighter (0.3f));
    filterLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (filterLabel);
    sourceCtl = std::make_unique<ParamControl> (proc, *pf::findParam ("g_source"), accent2);
    addAndMakeVisible (*sourceCtl);
    filterBox.setTooltip (u8 ("Welche Noten der Datei dieses Instrument spielt: automatisch (Spurname oder Drum-Map), alle, eine Spur oder eine bestimmte Note."));
    filterBox.onChange = [this] { if (filterBox.getSelectedId() > 0) proc.setClipFilter (filterBox.getSelectedId() - 1); repaint(); };
    loadButton.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("MIDI-Datei laden", juce::File(), "*.mid;*.midi");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc) { if (fc.getResult().existsAsFile()) loadFile (fc.getResult()); });
    };
    clearButton.onClick = [this] { proc.clearClip(); status = u8 ("Clip entfernt – es spielt wieder das Pattern."); update(); };
    clipChip = std::make_unique<DragChip> (u8 ("Clip als MIDI (gefiltert)"), accent2, [this] { return proc.writeMidi (PsyForgeProcessor::MidiClipOut); });
    clipChip->setTooltip (u8 ("Die Noten dieses Instruments aus dem Clip – auf die Drum-Map-Note gesetzt – als MIDI in Cubase ziehen."));
    addAndMakeVisible (*clipChip);
    update();
}

void ClipPage::loadFile (const juce::File& f)
{
    juce::String err;
    if (proc.loadClip (f, err))
        status = u8 ("Geladen. Mit „Pattern spielen“ (oben) oder ⟳ Vorhören spielt PsyForge den Clip synchron zu Cubase.");
    else status = err;
    lastClip = nullptr;
    update();
}

bool ClipPage::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files) if (juce::File (f).hasFileExtension ("mid;midi")) return true;
    return false;
}

void ClipPage::filesDropped (const juce::StringArray& files, int, int)
{
    dragOver = false;
    for (auto& f : files) if (juce::File (f).hasFileExtension ("mid;midi")) { loadFile (juce::File (f)); break; }
    repaint();
}

void ClipPage::update()
{
    auto clip = proc.getClip();
    const int inst = proc.instrument();
    if (clip.get() != lastClip || inst != lastInst)
    {
        lastClip = clip.get(); lastInst = inst;
        filterBox.clear (juce::dontSendNotification);
        if (clip != nullptr)
        {
            filterBox.addItem (pf::clipFilterLabel (*clip, pf::ClipAuto, inst), pf::ClipAuto + 1);
            filterBox.addItem (u8 ("Alle Noten"), pf::ClipAll + 1);
            bool tracks = false;
            for (int t = 0; t < clip->trackNames.size(); ++t)
                if (clip->trackCounts[(size_t) t] > 0)
                {
                    if (! tracks) { filterBox.addSectionHeading ("Spur"); tracks = true; }
                    filterBox.addItem (pf::clipFilterLabel (*clip, pf::ClipTrackBase + t, inst) + "  (" + juce::String (clip->trackCounts[(size_t) t]) + ")",
                                       pf::ClipTrackBase + t + 1);
                }
            filterBox.addSectionHeading ("Note");
            for (const auto& [note, count] : clip->noteCounts)
                filterBox.addItem (pf::clipFilterLabel (*clip, pf::ClipNoteBase + note, inst) + "  (" + juce::String (count) + ")", pf::ClipNoteBase + note + 1);
            filterBox.setSelectedId (proc.getClipFilter() + 1, juce::dontSendNotification);
        }
    }
    filterLabel.setText (juce::String ("Noten für ") + pf::instrumentName (inst), juce::dontSendNotification);
    filterBox.setEnabled (clip != nullptr);
    clearButton.setEnabled (clip != nullptr);
    clipChip->setEnabled (clip != nullptr);
    clipChip->setAlpha (clip != nullptr ? 1.0f : 0.4f);
    repaint();
}

void ClipPage::paint (juce::Graphics& g)
{
    auto r = dropArea.toFloat();
    g.setColour (dragOver ? accent.withAlpha (0.15f) : panelCol);
    g.fillRoundedRectangle (r, 8.0f);
    juce::Path border; border.addRoundedRectangle (r.reduced (1.0f), 8.0f);
    juce::Path dashed; const float d[] = { 6.0f, 4.0f };
    juce::PathStrokeType (1.2f).createDashedStroke (dashed, border, d, 2);
    g.setColour (dragOver ? accent : panelEdge.brighter (0.5f));
    g.fillPath (dashed);

    auto clip = proc.getClip();
    auto inner = dropArea.reduced (16, 12);
    g.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    g.setColour (textCol);
    g.drawText (clip ? clip->name : u8 ("MIDI-Datei hierher ziehen"), inner.removeFromTop (22), juce::Justification::centredLeft);
    g.setFont (juce::FontOptions (12.5f));
    g.setColour (dimText);
    if (clip)
    {
        int count = 0;
        for (auto& n : clip->notes) if (pf::clipNoteMatches (*clip, n, proc.instrument(), proc.getClipFilter())) ++count;
        g.drawText (juce::String ((int) (clip->lengthBeats / 4)) + u8 (" Takte · ") + juce::String ((int) clip->notes.size()) + u8 (" Noten in der Datei · ")
                    + juce::String (count) + u8 (" davon spielt ") + pf::instrumentName (proc.instrument()),
                    inner.removeFromTop (18), juce::Justification::centredLeft);
    }
    else
        g.drawText (u8 ("z. B. aus dem MIDI-Generator, Midizer oder einer Cubase-Spur exportiert"), inner.removeFromTop (18), juce::Justification::centredLeft);
    g.setColour (textCol);
    g.drawFittedText (status, inner.removeFromBottom (36), juce::Justification::bottomLeft, 2);

    auto tips = getLocalBounds().withLeft (dropArea.getRight() + 290).reduced (8, 0);
    g.setColour (dimText);
    g.setFont (juce::FontOptions (12.0f));
    g.drawFittedText (u8 ("So geht's:\n"
                          "• MIDI-Datei hierher ziehen – sie wird ins Projekt gespeichert.\n"
                          "• „Noten“ wählt, was diese Instanz spielt (Spur, Note oder automatisch nach Spurname/Drum-Map).\n"
                          "• Quelle = MIDI-Clip: „Pattern spielen“ und ⟳ Vorhören spielen den Clip im Loop, synchron zu Cubase.\n"
                          "• Mit dem Chip gefiltert und auf die richtige Note gesetzt wieder in Cubase ziehen.\n"
                          "• Oder einfach die Datei direkt auf die Cubase-Spur legen: PsyForge spielt alle Noten der Spur."),
                      tips, juce::Justification::topLeft, 14);
}

void ClipPage::resized()
{
    auto r = getLocalBounds();
    dropArea = r.removeFromLeft (560);
    roll.setBounds (dropArea.reduced (16, 0).withTrimmedTop (64).withTrimmedBottom (52));
    auto col = r.withTrimmedLeft (16).withWidth (260);
    auto row = [&] (int h) { auto x = col.removeFromTop (h); col.removeFromTop (6); return x; };
    loadButton.setBounds (row (28));
    filterLabel.setBounds (row (14));
    filterBox.setBounds (row (26));
    sourceCtl->setBounds (row (44));
    clearButton.setBounds (row (26));
    clipChip->setBounds (row (32));
}

//==============================================================================
AnalysePage::AnalysePage (PsyForgeProcessor& p, std::function<void()> cb) : proc (p), onRefChanged (std::move (cb))
{
    for (auto* b : { &loadButton, &playRefButton, &analyseButton, &quickButton, &cancelButton, &applyButton, &undoButton })
        addAndMakeVisible (*b);
    addAndMakeVisible (showRef);
    showRef.setToggleState (true, juce::dontSendNotification);
    addAndMakeVisible (progressBar);
    progressBar.setPercentageDisplay (false);
    analyseButton.setColour (juce::TextButton::buttonColourId, accent.withAlpha (0.35f));
    applyButton.setColour (juce::TextButton::buttonColourId, kickCol.withAlpha (0.3f));
    loadButton.setTooltip (u8 ("Ein One-Shot dieses Instruments (z. B. eine einzelne Snare) als WAV, AIFF oder FLAC laden."));
    analyseButton.setTooltip (u8 ("Misst das Original und passt danach alle Regler des Instruments an, bis der Klang sehr nahe dran ist (ca. 10–45 s)."));
    quickButton.setTooltip (u8 ("Nur grobe Schätzung (sofort)."));
    applyButton.setTooltip (u8 ("Gefundene Einstellungen ins Instrument übernehmen."));
    undoButton.setTooltip (u8 ("Zurück zu den Einstellungen vor dem Übernehmen."));

    loadButton.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("One-Shot laden", juce::File(), "*.wav;*.aif;*.aiff;*.flac");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc) { if (fc.getResult().existsAsFile()) loadFile (fc.getResult()); });
    };
    playRefButton.onClick = [this] { proc.auditionReference(); };
    analyseButton.onClick = [this] { proc.startAnalysis (true); status = u8 ("Analysiere und passe an …"); update(); };
    quickButton.onClick = [this] { proc.startAnalysis (false); status = u8 ("Analysiere …"); update(); };
    cancelButton.onClick = [this] { proc.cancelAnalysis(); };
    applyButton.onClick = [this]
    {
        proc.applyAnalysisResult();
        status = u8 ("Übernommen. Mit ▶ Anschlag und ▶ Original vergleichen.");
        if (onRefChanged) onRefChanged();
        update();
    };
    undoButton.onClick = [this] { proc.undoAnalysis(); status = u8 ("Rückgängig gemacht."); update(); };
    showRef.onClick = [this] { if (onRefChanged) onRefChanged(); };
    update();
}

void AnalysePage::loadFile (const juce::File& f)
{
    juce::String err;
    if (proc.loadReference (f, err)) { status = err.isNotEmpty() ? err : u8 ("Geladen. Jetzt „Analysieren + Feinanpassung“."); summary.clear(); }
    else status = err;
    if (onRefChanged) onRefChanged();
    update();
}

bool AnalysePage::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files) if (juce::File (f).hasFileExtension ("wav;aif;aiff;flac")) return true;
    return false;
}

void AnalysePage::filesDropped (const juce::StringArray& files, int, int)
{
    dragOver = false;
    for (auto& f : files) if (juce::File (f).hasFileExtension ("wav;aif;aiff;flac")) { loadFile (juce::File (f)); break; }
    repaint();
}

void AnalysePage::update()
{
    const bool running = proc.isAnalysing();
    const auto ref = proc.getReference();
    progress = running ? (double) proc.getAnalysisProgress() : (proc.hasAnalysisResult() ? 1.0 : 0.0);
    const int v = proc.getAnalysisVersion();
    if (v != lastVersion && ! running)
    {
        lastVersion = v;
        if (proc.hasAnalysisResult())
        {
            const auto r = proc.getAnalysisResult();
            summary = u8 (r.summary.c_str());
            status = u8 ("Fertig – Übereinstimmung ") + juce::String (juce::roundToInt (r.match)) + " %"
                   + (r.initialMatch > 0 ? u8 (" (Grob-Schätzung ") + juce::String (juce::roundToInt (r.initialMatch)) + " %)" : juce::String())
                   + u8 (". Mit „Übernehmen“ laden.");
        }
    }
    if (running) status = u8 ("Passe an … aktuelle Übereinstimmung ") + juce::String (juce::roundToInt (proc.getAnalysisLiveMatch())) + " %";
    playRefButton.setEnabled (ref != nullptr);
    analyseButton.setEnabled (ref != nullptr && ! running);
    quickButton.setEnabled (ref != nullptr && ! running);
    cancelButton.setEnabled (running);
    applyButton.setEnabled (! running && proc.hasAnalysisResult());
    undoButton.setEnabled (proc.canUndoAnalysis());
    repaint();
}

void AnalysePage::paint (juce::Graphics& g)
{
    auto r = dropArea.toFloat();
    g.setColour (dragOver ? accent.withAlpha (0.15f) : panelCol);
    g.fillRoundedRectangle (r, 8.0f);
    juce::Path border; border.addRoundedRectangle (r.reduced (1.0f), 8.0f);
    juce::Path dashed; const float d[] = { 6.0f, 4.0f };
    juce::PathStrokeType (1.2f).createDashedStroke (dashed, border, d, 2);
    g.setColour (dragOver ? accent : panelEdge.brighter (0.5f));
    g.fillPath (dashed);

    const auto ref = proc.getReference();
    const int inst = proc.instrument();
    auto inner = dropArea.reduced (16);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.setColour (instColour (inst));
    g.drawText (u8 ("Analyse für: ") + pf::instrumentName (inst), inner.removeFromTop (18), juce::Justification::centredLeft);
    g.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    g.setColour (textCol);
    g.drawText (ref ? ref->name : u8 ("One-Shot hierher ziehen"), inner.removeFromTop (24), juce::Justification::centredLeft);
    g.setFont (juce::FontOptions (12.5f));
    g.setColour (dimText);
    if (ref)
        g.drawText (juce::String ((double) ref->samples.size() / ref->sampleRate * 1000.0, 0) + " ms  ·  " + juce::String ((int) ref->sampleRate) + " Hz",
                    inner.removeFromTop (20), juce::Justification::centredLeft);
    else
        g.drawText (u8 ("WAV, AIFF oder FLAC – ein einzelner Schlag des Instruments"), inner.removeFromTop (20), juce::Justification::centredLeft);
    inner.removeFromTop (10);
    g.setColour (textCol);
    g.setFont (juce::FontOptions (13.0f));
    g.drawFittedText (status, inner.removeFromTop (40), juce::Justification::topLeft, 3);
    g.setColour (kickCol);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawFittedText (summary, inner.removeFromTop (44), juce::Justification::topLeft, 3);

    auto tips = getLocalBounds().withLeft (dropArea.getRight() + 280).reduced (8, 0);
    g.setColour (dimText);
    g.setFont (juce::FontOptions (12.0f));
    g.drawFittedText (u8 ("So kommst du am nächsten ran:\n"
                          "• Die Analyse gilt für das Instrument oben – für eine Snare die Instanz auf Snare stellen.\n"
                          "• Ein sauberes One-Shot ohne Hall/Delay, ein Schlag allein.\n"
                          "• Nach dem Übernehmen mit „▶ Original“ und „▶ Anschlag“ vergleichen und nachregeln.\n"
                          "• Tonhöhen werden exakt übernommen (Snap/Key-Lock aus). Für deine Tonart wieder einschalten.\n"
                          "• Bässe landen auf der nächsten Note deiner Stimmung – bei 432-Samples vorher A4 auf 432 stellen."),
                      tips, juce::Justification::topLeft, 14);
}

void AnalysePage::resized()
{
    auto r = getLocalBounds();
    dropArea = r.removeFromLeft (560).withHeight (std::min (r.getHeight(), 260));
    auto col = r.withTrimmedLeft (16).withWidth (250);
    auto row = [&] (int h) { auto x = col.removeFromTop (h); col.removeFromTop (6); return x; };
    loadButton.setBounds (row (28));
    playRefButton.setBounds (row (28));
    analyseButton.setBounds (row (32));
    {
        auto x = row (26);
        quickButton.setBounds (x.removeFromLeft (x.getWidth() / 2 - 3));
        x.removeFromLeft (6);
        cancelButton.setBounds (x);
    }
    progressBar.setBounds (row (10));
    {
        auto x = row (30);
        applyButton.setBounds (x.removeFromLeft (x.getWidth() / 2 - 3));
        x.removeFromLeft (6);
        undoButton.setBounds (x);
    }
    showRef.setBounds (row (22));
}

//==============================================================================
// Klang-Seiten: Gruppen pro Instrument
static Page* makeSoundPage (PsyForgeProcessor& proc, int inst)
{
    auto* page = new Page();
    const auto c = instColour (inst);
    auto add = [&] (const juce::String& t, std::vector<const char*> ids, juce::Colour col) { page->sections.add (new Section (proc, t, ids, col)); };
    switch (inst)
    {
        case pf::InKick:
            add ("PITCH", { "k_start", "k_punch", "k_end", "k_clickt", "k_sweep", "k_drift", "k_snap", "k_shape", "k_phase", "k_knock", "k_follow" }, c);
            add ("CLICK", { "k_clvl", "k_clhz", "k_cldec", "k_cltone" }, accent2);
            add (u8 ("LAUTSTÄRKE / FISHTAIL"), { "k_level", "k_attack", "k_hold", "k_decay", "k_length", "k_sync", "k_fade", "k_fdepth", "k_fpos", "k_fwidth", "k_vel" }, c);
            add ("DRIVE", { "k_drive", "k_dtype", "k_trans" }, accent);
            add ("EQ", { "k_lowcut", "k_holehz", "k_holedb", "k_holeq", "k_highcut" }, refCol);
            break;
        case pf::InBass:
            add ("OSZILLATOR", { "b_level", "b_oct", "b_fine", "b_saw", "b_square", "b_pw", "b_detune", "b_sub", "b_suboct", "b_subinv", "b_retrig", "b_phase" }, c);
            add (u8 ("LAUTSTÄRKE"), { "b_attack", "b_decay", "b_sustain", "b_release", "b_vel", "b_offset" }, c);
            add ("FILTER", { "b_cutoff", "b_reso", "b_fenv", "b_fatt", "b_fdec", "b_key", "b_slope" }, accent2);
            add ("PLUCK / DRIVE / DUCKING", { "b_pluck", "b_pldec", "b_drive", "b_dtype", "b_duck", "b_duckrel", "b_ducksrc" }, accent);
            add ("EQ", { "b_lowcut", "b_notch", "b_highcut" }, refCol);
            break;
        default:
        {
            // automatisch aus der Tabelle: Gruppen in Tabellen-Reihenfolge
            static const std::map<std::string, const char*> titles = {
                { "s_body", "KÖRPER" }, { "s_noise", "RAUSCHEN (TEPPICH)" }, { "s_snap", "SNAP / DRIVE / EQ" },
                { "c_main", "SCHLÄGE" }, { "c_filter", "FILTER / DRIVE" },
                { "h_tone", "TON" }, { "h_filter", "FILTER" }, { "h_amp", "HÜLLKURVE" },
                { "o_tone", "TON" }, { "o_filter", "FILTER" }, { "o_amp", "HÜLLKURVE" },
                { "r_tone", "TON" }, { "r_filter", "FILTER" }, { "r_amp", "HÜLLKURVE" },
                { "p_osc", "OSZILLATOR" }, { "p_env", "HÜLLKURVE / FILTER" },
                { "z_main", "SWEEP" }, { "z_tone", "KLANG" },
                { "x_main", "RISER" }, { "x_tone", "FILTER / TON" } };
            std::vector<std::string> order;
            std::map<std::string, std::vector<const char*>> groups;
            for (auto& d : pf::paramTable())
                if (pf::belongsTo (d.id, inst))
                {
                    if (groups.find (d.group) == groups.end()) order.push_back (d.group);
                    groups[d.group].push_back (d.id);
                }
            int i = 0;
            for (auto& gname : order)
            {
                auto it = titles.find (gname);
                add (u8 (it != titles.end() ? it->second : gname.c_str()), groups[gname], i == 0 ? c : (i == 1 ? accent2 : accent));
                ++i;
            }
            break;
        }
    }
    for (auto* s : page->sections) page->addAndMakeVisible (s);
    return page;
}
} // namespace pfui

//==============================================================================
using namespace pfui;

PsyForgeEditor::PsyForgeEditor (PsyForgeProcessor& p) : AudioProcessorEditor (&p), proc (p), roll (p)
{
    setLookAndFeel (&lnf.get());
    addAndMakeVisible (content);

    titleLabel.setText ("PSYFORGE", juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, accent);
    content.addAndMakeVisible (titleLabel);
    presetCaption.setText ("Preset", juce::dontSendNotification);
    presetCaption.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    presetCaption.setColour (juce::Label::textColourId, dimText);
    content.addAndMakeVisible (presetCaption);
    modeLabel.setFont (juce::FontOptions (11.0f));
    modeLabel.setColour (juce::Label::textColourId, dimText);
    content.addAndMakeVisible (modeLabel);

    auto mk = [this] (const char* id, juce::Colour c)
    {
        auto ctl = std::make_unique<ParamControl> (proc, *pf::findParam (id), c);
        content.addAndMakeVisible (*ctl);
        return ctl;
    };
    instCtl = mk ("g_inst", accent);
    instCtl->combo->onChange = [this, old = instCtl->combo->onChange]
    {
        if (old) old();
        instrumentChanged();
    };

    content.addAndMakeVisible (presetBox);
    presetBox.onChange = [this] { presetChosen(); };
    for (auto* b : { &prevPreset, &nextPreset, &savePreset, &helpButton }) content.addAndMakeVisible (*b);
    prevPreset.onClick = [this] { stepPreset (-1); };
    nextPreset.onClick = [this] { stepPreset (+1); };
    savePreset.onClick = [this] { savePresetDialog(); };
    helpButton.onClick = [this] { showHelp(); };
    helpButton.setTooltip (u8 ("Kurzanleitung"));

    keyCtl = mk ("g_key", accent2);
    a4Ctl = mk ("g_a4", accent2);
    filterCtl = mk ("g_midif", accent2);
    masterCtl = mk ("g_master", accent);
    limitCtl = mk ("g_limit", accent);
    for (auto* b : { &tune440, &tune432 }) content.addAndMakeVisible (*b);
    tune440.onClick = [this] { proc.setParam ("g_a4", 440.0f); };
    tune432.onClick = [this] { proc.setParam ("g_a4", 432.0f); };

    wave = std::make_unique<WaveView> (proc);
    content.addAndMakeVisible (*wave);

    // Pattern-Panel
    styleCtl = mk ("g_style", accent2);
    sectionCtl = mk ("g_section", accent2);
    bassRhyCtl = mk ("g_bassrhy", bassCol);
    varCtl = mk ("g_var", accent2);
    swingCtl = mk ("g_swing", accent2);
    humanCtl = mk ("g_human", accent2);
    bpmCtl = mk ("g_bpm", accent2);
    noteCtl = mk ("g_note", bassCol);
    seqCtl = mk ("g_seq", accent);
    content.addAndMakeVisible (diceButton);
    diceButton.setTooltip (u8 ("Zufällige Groove-Variante (1–12)."));
    diceButton.onClick = [this] { proc.setParam ("g_var", (float) juce::Random::getSystemRandom().nextInt ({ 1, 13 })); };
    roll.setTooltip (u8 ("Noten, die diese Instanz im Pattern (bzw. Clip) spielt. Weisse Linie = Abspielposition."));
    content.addAndMakeVisible (roll);
    for (auto* b : { &hitButton, &previewButton, &allSectionsButton }) content.addAndMakeVisible (*b);
    hitButton.onClick = [this] { proc.audition(); };
    hitButton.setTooltip (u8 ("Einen Anschlag vorhören."));
    previewButton.setClickingTogglesState (true);
    previewButton.setTooltip (u8 ("Pattern bzw. Clip im Loop vorhören – auch ohne Play in Cubase. Läuft Cubase, spielt es synchron mit."));
    previewButton.onClick = [this] { proc.previewOn.store (previewButton.getToggleState()); };
    allSectionsButton.setTooltip (u8 ("Alle 9 Sections dieses Instruments als einzelne MIDI-Dateien in einen Ordner speichern."));
    allSectionsButton.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Ordner für alle Sections", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory));
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories, [this] (const juce::FileChooser& fc)
        {
            const auto dir = fc.getResult();
            if (dir == juce::File()) return;
            const bool ok = proc.exportAllSections (dir.getChildFile ("PsyForge_" + juce::String (pf::instrumentName (proc.instrument())).removeCharacters (" ")));
            juce::AlertWindow::showMessageBoxAsync (ok ? juce::MessageBoxIconType::InfoIcon : juce::MessageBoxIconType::WarningIcon, "PsyForge",
                                                    ok ? u8 ("Alle Sections gespeichert.") : u8 ("Speichern fehlgeschlagen."));
        });
    };

    auto chip = [this] (const juce::String& text, juce::Colour c, int midiWhat, int wavWhat, const juce::String& tip)
    {
        auto make = [this, midiWhat, wavWhat] { return midiWhat >= 0 ? proc.writeMidi (midiWhat) : proc.renderWav (wavWhat); };
        auto* ch = chips.add (new DragChip (text, c, make, [this, make, midiWhat]
        {
            const auto f = make();
            if (f.existsAsFile()) saveFileDialog (f, midiWhat >= 0 ? "MIDI speichern" : "WAV speichern", midiWhat >= 0 ? "*.mid" : "*.wav");
        }));
        ch->setTooltip (tip + u8 ("\nZiehen = in Cubase ablegen · Klicken = speichern unter …"));
        content.addAndMakeVisible (ch);
    };
    chip (u8 ("MIDI: Section"), accent, PsyForgeProcessor::MidiSection, -1, u8 ("Die gewählte Section (4 Takte) für dieses Instrument."));
    chip (u8 ("MIDI: Full Track"), accent, PsyForgeProcessor::MidiFullTrack, -1, u8 ("Ein kompletter Song-Ablauf (~120 Takte) für dieses Instrument."));
    chip (u8 ("Alle Instr.: Section"), accent2, PsyForgeProcessor::MidiAllSection, -1,
          u8 ("Alle Instrumente als eigene Spuren (Kick, Bass, Snare …). Cubase legt beim Ablegen pro Instrument eine MIDI-Spur an – "
              "danach jede Spur auf eine PsyForge-Instanz mit dem passenden Instrument leiten."));
    chip (u8 ("Alle Instr.: Full Track"), accent2, PsyForgeProcessor::MidiAllFullTrack, -1, u8 ("Kompletter Song, alle Instrumente als eigene Spuren."));
    chip (u8 ("WAV: Anschlag"), kickCol, -1, PsyForgeProcessor::WavHit, u8 ("Ein Anschlag als 24-Bit-WAV."));
    chip (u8 ("WAV: Loop"), kickCol, -1, PsyForgeProcessor::WavLoop, u8 ("Das Pattern bzw. den Clip dieses Instruments als nahtlosen Loop."));

    // Tabs
    for (int i = 0; i < pf::NumInstruments; ++i)
    {
        auto* pg = soundPages.add (makeSoundPage (proc, i));
        content.addChildComponent (pg);
    }
    clipPage = std::make_unique<ClipPage> (proc);
    content.addChildComponent (*clipPage);
    analysePage = std::make_unique<AnalysePage> (proc, [this]
    {
        wave->showReference = analysePage->showRef.getToggleState();
        wave->refresh();
    });
    content.addChildComponent (*analysePage);
    for (auto* b : { &tabSound, &tabClip, &tabAnalyse }) content.addAndMakeVisible (*b);
    tabSound.onClick = [this] { showTab (0); };
    tabClip.onClick = [this] { showTab (1); };
    tabAnalyse.onClick = [this] { showTab (2); };

    instrumentChanged();
    showTab (0);

    setResizable (true, true);
    const double ratio = (double) baseW / baseH;
    if (auto* c = getConstrainer()) { c->setFixedAspectRatio (ratio); c->setSizeLimits (baseW * 6 / 10, baseH * 6 / 10, baseW * 2, baseH * 2); }
    const int w = juce::jlimit (baseW * 6 / 10, baseW * 2, proc.getUiWidth());
    setSize (w, (int) std::round (w / ratio));
    startTimerHz (20);
}

PsyForgeEditor::~PsyForgeEditor() { setLookAndFeel (nullptr); }

void PsyForgeEditor::paint (juce::Graphics& g) { g.fillAll (bgCol); }

void PsyForgeEditor::resized()
{
    const float s = (float) getWidth() / (float) baseW;
    content.setBounds (0, 0, baseW, baseH);
    content.setTransform (juce::AffineTransform::scale (s));
    proc.setUiWidth (getWidth());
}

bool PsyForgeEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    return analysePage->isInterestedInFileDrag (files) || clipPage->isInterestedInFileDrag (files);
}

void PsyForgeEditor::filesDropped (const juce::StringArray& files, int x, int y)
{
    if (clipPage->isInterestedInFileDrag (files)) { showTab (1); clipPage->filesDropped (files, x, y); }
    else { showTab (2); analysePage->filesDropped (files, x, y); }
}

void PsyForgeEditor::Content::paint (juce::Graphics& g)
{
    g.fillAll (bgCol);
    const auto c = instColour (ed.proc.instrument());
    g.setGradientFill (juce::ColourGradient (c.withAlpha (0.12f), 0, 0, bgCol, 0, 70, false));
    g.fillRect (0, 0, getWidth(), 70);

    // Pattern-Panel
    auto pp = ed.patternPanel.toFloat();
    g.setColour (panelCol);
    g.fillRoundedRectangle (pp, 8.0f);
    g.setColour (panelEdge);
    g.drawRoundedRectangle (pp.reduced (0.5f), 8.0f, 1.0f);
    g.setColour (accent2);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText (u8 ("PATTERN & MIDI"), (int) pp.getX() + 12, (int) pp.getY() + 5, 200, 16, juce::Justification::centredLeft);

    // Pegel + Duck-Anzeige
    auto m = ed.meterArea.toFloat();
    if (! m.isEmpty())
    {
        auto bar = m.withWidth (10.0f).withTrimmedBottom (12.0f);
        g.setColour (panelCol); g.fillRoundedRectangle (bar, 2.0f);
        const float h = juce::jlimit (0.0f, 1.0f, (20.0f * std::log10 (std::max (ed.meter, 1e-4f)) + 48.0f) / 48.0f) * bar.getHeight();
        g.setColour (ed.meter > 0.98f ? juce::Colours::red : c);
        g.fillRoundedRectangle (bar.withTop (bar.getBottom() - h), 2.0f);
        g.setColour (dimText);
        g.setFont (juce::FontOptions (9.0f));
        g.drawText ("OUT", (int) bar.getX() - 6, (int) bar.getBottom() + 1, 22, 10, juce::Justification::centred);
        if (ed.proc.instrument() == pf::InBass)
        {
            auto led = juce::Rectangle<float> (m.getX() + 16.0f, m.getY() + 4.0f, 10.0f, 10.0f);
            g.setColour (accent.withAlpha (0.2f + 0.8f * ed.duckLed));
            g.fillEllipse (led);
            g.setColour (dimText);
            g.drawText ("DUCK", (int) led.getX() - 8, (int) led.getBottom() + 2, 28, 10, juce::Justification::centred);
        }
    }
}

void PsyForgeEditor::Content::resized()
{
    auto& e = ed;
    e.titleLabel.setBounds (14, 6, 170, 30);
    e.helpButton.setBounds (150, 12, 24, 22);
    e.instCtl->setBounds (190, 4, 170, 46);
    e.instCtl->label.setText (u8 ("Instrument dieser Spur"), juce::dontSendNotification);
    e.modeLabel.setBounds (14, 46, 560, 18);
    {
        auto pr = juce::Rectangle<int> (372, 6, 330, 50);
        e.presetCaption.setBounds (pr.removeFromTop (14));
        auto row = pr.withHeight (28);
        e.prevPreset.setBounds (row.removeFromLeft (26)); row.removeFromLeft (4);
        e.savePreset.setBounds (row.removeFromRight (78)); row.removeFromRight (4);
        e.nextPreset.setBounds (row.removeFromRight (26)); row.removeFromRight (4);
        e.presetBox.setBounds (row);
    }
    int x = 714;
    auto place = [&] (juce::Component& c, int w) { c.setBounds (x, 4, w, 58); x += w + 6; };
    place (*e.keyCtl, 62);
    place (*e.a4Ctl, 62);
    e.tune440.setBounds (x, 12, 38, 20); e.tune432.setBounds (x, 36, 38, 20); x += 44;
    place (*e.filterCtl, 112);
    place (*e.limitCtl, 46);
    place (*e.masterCtl, 60);
    e.meterArea = { x, 8, 34, 54 };

    // Mitte: Waveform + Pattern-Panel
    const int midY = 70, midH = 292;
    e.wave->setBounds (12, midY, 640, midH);
    e.patternPanel = { 662, midY, 506, midH };
    auto pp = e.patternPanel.reduced (10, 0).withTrimmedTop (24);
    {
        auto row = pp.removeFromTop (46);
        e.styleCtl->setBounds (row.removeFromLeft (116)); row.removeFromLeft (6);
        e.sectionCtl->setBounds (row.removeFromLeft (98)); row.removeFromLeft (6);
        e.diceButton.setBounds (row.removeFromRight (54).withTrimmedTop (16).withHeight (24)); row.removeFromRight (4);
        e.varCtl->setBounds (row.removeFromRight (66)); row.removeFromRight (6);
        e.bassRhyCtl->setBounds (row);
    }
    pp.removeFromTop (6);
    e.roll.setBounds (pp.removeFromTop (62));
    pp.removeFromTop (6);
    {
        auto row = pp.removeFromTop (56);
        e.seqCtl->setBounds (row.removeFromLeft (92)); row.removeFromLeft (4);
        auto btns = row.removeFromLeft (120);
        e.hitButton.setBounds (btns.removeFromTop (26)); btns.removeFromTop (4);
        e.previewButton.setBounds (btns.removeFromTop (26));
        row.removeFromLeft (6);
        e.swingCtl->setBounds (row.removeFromLeft (58));
        e.humanCtl->setBounds (row.removeFromLeft (64));
        e.bpmCtl->setBounds (row.removeFromLeft (70));
        e.noteCtl->setBounds (row.removeFromLeft (76));
    }
    pp.removeFromTop (8);
    {
        const int cw = (pp.getWidth() - 12) / 3;
        auto row1 = pp.removeFromTop (28);
        for (int i = 0; i < 3 && i < e.chips.size(); ++i) { e.chips[i]->setBounds (row1.removeFromLeft (cw)); row1.removeFromLeft (6); }
        pp.removeFromTop (6);
        auto row2 = pp.removeFromTop (28);
        for (int i = 3; i < 6 && i < e.chips.size(); ++i) { e.chips[i]->setBounds (row2.removeFromLeft (cw)); row2.removeFromLeft (6); }
        e.allSectionsButton.setBounds (e.patternPanel.getRight() - 128, e.patternPanel.getY() + 3, 118, 20);
    }

    // Tabs
    auto tabs = juce::Rectangle<int> (12, midY + midH + 10, 600, 28);
    e.tabSound.setBounds (tabs.removeFromLeft (110)); tabs.removeFromLeft (4);
    e.tabClip.setBounds (tabs.removeFromLeft (110)); tabs.removeFromLeft (4);
    e.tabAnalyse.setBounds (tabs.removeFromLeft (110));
    const auto area = juce::Rectangle<int> (12, midY + midH + 46, getWidth() - 24, getHeight() - (midY + midH + 46) - 8);
    for (auto* pg : e.soundPages) pg->setBounds (area);
    e.clipPage->setBounds (area);
    e.analysePage->setBounds (area);
}

void PsyForgeEditor::showTab (int t)
{
    tab = t;
    const int inst = proc.instrument();
    for (int i = 0; i < soundPages.size(); ++i) soundPages[i]->setVisible (t == 0 && i == inst);
    clipPage->setVisible (t == 1);
    analysePage->setVisible (t == 2);
    tabSound.setToggleState (t == 0, juce::dontSendNotification);
    tabClip.setToggleState (t == 1, juce::dontSendNotification);
    tabAnalyse.setToggleState (t == 2, juce::dontSendNotification);
    if (t == 1) clipPage->update();
    if (t == 2) analysePage->update();
}

void PsyForgeEditor::instrumentChanged()
{
    const int inst = proc.instrument();
    if (inst == shownInst) return;
    shownInst = inst;
    const auto c = instColour (inst);
    tabSound.setColour (juce::TextButton::buttonOnColourId, c.darker (0.3f));
    tabSound.setButtonText (juce::String ("SOUND: ") + juce::String (pf::instrumentName (inst)).toUpperCase());
    bassRhyCtl->setVisible (inst == pf::InBass);
    noteCtl->setVisible (inst == pf::InBass);
    showTab (tab);
    buildPresetMenu();
    wave->refresh();
    content.repaint();
}

void PsyForgeEditor::timerCallback()
{
    instrumentChanged();
    if (previewButton.getToggleState() != proc.previewOn.load()) previewButton.setToggleState (proc.previewOn.load(), juce::dontSendNotification);
    if (proc.getPresetName() != lastPresetName) buildPresetMenu();
    meter = proc.uiLevel.load();
    duckLed = proc.uiDuck.load();
    content.repaint (meterArea);

    // Statuszeile: was spielt gerade?
    const auto p = proc.readParams();
    juce::String mode;
    const bool seqOn = p.seqOn > 0.5f;
    const juce::String src = p.source > 0.5f ? u8 ("MIDI-Clip") : u8 ("Pattern");
    if (seqOn) mode = u8 ("Spielt: ") + src + u8 (" synchron zu Cubase + Noten der Spur");
    else mode = u8 ("Spielt: ") + ((int) p.midiFilter == 1 ? u8 ("nur passende Drum-Map-Noten der Spur") : u8 ("alle Noten der Cubase-Spur"));
    if (proc.instrument() == pf::InBass && p.bDuck > 0.0f)
        mode += u8 ("  ·  Ducking: ") + juce::StringArray { "aus", "Kick-Instanz", "Viertel-Raster", "Sidechain" }[(int) p.bDuckSrc];
    modeLabel.setText (mode, juce::dontSendNotification);

    if (tab == 2 || proc.isAnalysing()) analysePage->update();
}

//==============================================================================
void PsyForgeEditor::buildPresetMenu()
{
    lastPresetName = proc.getPresetName();
    presetBox.clear (juce::dontSendNotification);
    presetList.clear();
    const int inst = proc.instrument();
    juce::String lastCat;
    int selected = 0;
    for (auto& fp : pf::factoryPresets())
    {
        if (fp.inst != inst) continue;
        const auto cat = juce::String::fromUTF8 (fp.category);
        if (cat != lastCat) { presetBox.addSectionHeading (cat); lastCat = cat; }
        presetList.push_back (&fp);
        const int id = (int) presetList.size();
        presetBox.addItem (juce::String::fromUTF8 (fp.name), id);
        if (juce::String::fromUTF8 (fp.name) == lastPresetName) selected = id;
    }
    userPresets = proc.listUserPresets();
    if (! userPresets.isEmpty())
    {
        presetBox.addSectionHeading ("Eigene Presets");
        for (int i = 0; i < userPresets.size(); ++i)
        {
            presetBox.addItem (userPresets[i].getFileNameWithoutExtension(), 1000 + i);
            if (userPresets[i].getFileNameWithoutExtension() == lastPresetName) selected = 1000 + i;
        }
    }
    if (selected > 0) presetBox.setSelectedId (selected, juce::dontSendNotification);
    else presetBox.setText (lastPresetName, juce::dontSendNotification);
}

void PsyForgeEditor::presetChosen()
{
    const int id = presetBox.getSelectedId();
    if (id <= 0) return;
    if (id >= 1000) { if (id - 1000 < userPresets.size()) proc.loadUserPreset (userPresets[id - 1000]); }
    else if (id - 1 < (int) presetList.size()) proc.loadFactoryPreset (*presetList[(size_t) id - 1]);
    lastPresetName = proc.getPresetName();
    wave->refresh();
}

void PsyForgeEditor::stepPreset (int delta)
{
    std::vector<int> ids;
    for (int i = 0; i < (int) presetList.size(); ++i) ids.push_back (i + 1);
    for (int i = 0; i < userPresets.size(); ++i) ids.push_back (1000 + i);
    if (ids.empty()) return;
    const int cur = presetBox.getSelectedId();
    auto it = std::find (ids.begin(), ids.end(), cur);
    int idx = it == ids.end() ? (delta > 0 ? -1 : 0) : (int) (it - ids.begin());
    idx = (idx + delta + (int) ids.size()) % (int) ids.size();
    presetBox.setSelectedId (ids[(size_t) idx], juce::sendNotificationSync);
}

void PsyForgeEditor::savePresetDialog()
{
    auto* aw = new juce::AlertWindow ("Preset speichern", juce::String ("Name des ") + pf::instrumentName (proc.instrument()) + "-Presets:",
                                      juce::MessageBoxIconType::NoIcon, this);
    const auto cur = proc.getPresetName();
    aw->addTextEditor ("name", cur.startsWith ("Analyse") ? cur.fromFirstOccurrenceOf (": ", false, false) : cur);
    aw->addButton ("Speichern", 1, juce::KeyPress (juce::KeyPress::returnKey));
    aw->addButton ("Abbrechen", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    aw->enterModalState (true, juce::ModalCallbackFunction::create ([this, aw] (int result)
    {
        if (result != 1) return;
        const auto name = aw->getTextEditorContents ("name").trim();
        if (name.isEmpty()) return;
        if (proc.saveUserPreset (name)) buildPresetMenu();
        else juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "PsyForge",
                                                     "Preset konnte nicht gespeichert werden:\n" + PsyForgeProcessor::presetDirectory().getFullPathName());
    }), true);
}

void PsyForgeEditor::saveFileDialog (const juce::File& source, const juce::String& title, const juce::String& pattern)
{
    const auto start = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("PsyForge").getChildFile (source.getFileName());
    start.getParentDirectory().createDirectory();
    chooser = std::make_unique<juce::FileChooser> (title, start, pattern);
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
                          [source] (const juce::FileChooser& fc)
    {
        auto f = fc.getResult();
        if (f == juce::File()) return;
        if (! f.hasFileExtension (source.getFileExtension())) f = f.withFileExtension (source.getFileExtension());
        if (! source.copyFileTo (f))
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "PsyForge", "Datei konnte nicht gespeichert werden.");
    });
}

void PsyForgeEditor::showHelp()
{
    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "PsyForge – so geht's", u8 (
        "1. Pro Instrument eine Cubase-Instrumentenspur mit PsyForge anlegen und oben das Instrument wählen (Kick, Bass, Snare …).\n\n"
        "2. MIDI holen: Stil + Section wählen und den Chip „MIDI: Section“ auf die Spur ziehen. "
        "Oder „Alle Instr.“ ziehen – Cubase legt dann pro Instrument eine MIDI-Spur an; diese Spuren auf die passenden PsyForge-Instanzen leiten.\n\n"
        "3. Abspielen: PsyForge spielt alle Noten seiner Spur (Kanal egal). Liegen mehrere Instrumente auf einer Spur, oben „Nur Drum-Map“ wählen.\n"
        "   Alternativ „Pattern spielen“ einschalten: dann spielt PsyForge das Pattern selbst, synchron zu Cubase.\n\n"
        "4. Eigene MIDI-Dateien in den Tab MIDI-CLIP ziehen – sie werden gefiltert, gespielt und wieder exportiert.\n\n"
        "5. Bass-Ducking: Eine Bass-Instanz hört automatisch auf die Kick-Instanz im selben Projekt (Duck-Quelle „Kick-Instanz“). "
        "Ohne Kick-Instanz duckt sie auf jeder Zählzeit.\n\n"
        "6. ANALYSE: ein One-Shot des Instruments laden – PsyForge stellt die Regler so ein, dass es möglichst gleich klingt."));
}
