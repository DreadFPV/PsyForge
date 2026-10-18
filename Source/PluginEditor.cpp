#include "PluginEditor.h"

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

//==============================================================================
WaveView::WaveView (PsyForgeProcessor& p) : proc (p)
{
    setTooltip (u8 ("Erzeugte Wellenform. Linie = Tonhöhe (rechts in Hz), gestrichelt = Hüllkurve, orange = importiertes Original.\n"
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
    const double sr = 44100.0;
    if (mode == 0)
    {
        render = pf::renderKick (p, sr, bpm, 1.0f, 0.01f);
        const auto k = pf::makeKickShape (p, bpm);
        const double midi = pf::hzToMidi (k.endHz, p.a4);
        const int n = (int) std::lround (midi);
        info = u8 ("Grundton ") + juce::String (k.endHz, 1) + " Hz (" + pf::noteName (n) + juce::String (pf::noteOctave (n))
             + juce::String ((midi - n) * 100.0 >= 0 ? " +" : " ") + juce::String ((int) std::lround ((midi - n) * 100.0)) + " ct)"
             + u8 (" · Punch ") + juce::String ((int) p.kPunchHz) + u8 (" Hz · Länge ") + juce::String ((int) std::round (k.len * 1000.0f)) + " ms";
    }
    else if (mode == 1)
    {
        render = pf::renderBass (p, sr, bpm, (int) p.note);
        const float hz = pf::bassNoteHz (p, (int) p.note);
        info = u8 ("Note ") + pf::noteName ((int) p.note) + juce::String (pf::noteOctave ((int) p.note)) + u8 (" · ")
             + juce::String (hz, 1) + u8 (" Hz · Note-Länge ") + juce::String ((int) std::round (15000.0 / bpm * 0.7)) + " ms (1/16)";
    }
    else
    {
        render = pf::renderLoop (p, sr, bpm, 2, (int) p.note, (int) p.pattern);
        const auto pats = pf::choiceList (*pf::findParam ("g_pattern"));
        info = u8 (pats[(size_t) std::clamp ((int) p.pattern, 0, 3)].c_str()) + u8 (" · ") + juce::String (bpm, 1) + " BPM";
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

    auto plot = bounds.reduced (12.0f, 0.0f).withTrimmedTop (26.0f).withTrimmedBottom (20.0f).withTrimmedRight (mode == 0 ? 44.0f : 0.0f);
    const auto col = mode == 0 ? kickCol : (mode == 1 ? bassCol : loopCol);
    const double sr = render.sr;
    const auto ref = (showReference && lastRef != nullptr && lastRef->type == mode) ? lastRef : nullptr;
    double dur = render.audio.size() / sr;
    if (ref) dur = std::max (dur, ref->samples.size() / ref->sampleRate);
    dur = std::max (dur, 0.01);
    auto xOf = [&] (double t) { return plot.getX() + (float) (t / dur) * plot.getWidth(); };
    const float midY = plot.getCentreY(), halfH = plot.getHeight() * 0.46f;

    // Raster
    g.setFont (juce::FontOptions (10.0f));
    if (mode == 2)
    {
        const double spb = 60.0 / lastBpm;
        for (int s = 0; s * spb / 4.0 <= dur; ++s)
        {
            const float x = xOf (s * spb / 4.0);
            g.setColour (s % 4 == 0 ? panelEdge.brighter (0.5f) : panelEdge);
            g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
        }
    }
    else
    {
        const double step = dur > 0.6 ? 0.1 : (dur > 0.25 ? 0.05 : 0.025);
        for (double t = 0.0; t <= dur + 1e-9; t += step)
        {
            const float x = xOf (t);
            g.setColour (panelEdge);
            g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
            g.setColour (dimText);
            g.drawText (juce::String ((int) std::round (t * 1000.0)) + " ms", (int) x - 25, (int) plot.getBottom() + 3, 50, 14, juce::Justification::centred);
        }
        // 1/16 und 1/8 im Tempo
        const double sixteenth = 15.0 / lastBpm;
        for (int k = 1; k <= 2 && k * sixteenth < dur; ++k)
        {
            const float x = xOf (k * sixteenth);
            g.setColour (accent.withAlpha (0.45f));
            for (float y = plot.getY(); y < plot.getBottom(); y += 6.0f) g.drawVerticalLine ((int) x, y, std::min (y + 3.0f, plot.getBottom()));
            g.drawText (k == 1 ? "1/16" : "1/8", (int) x + 3, (int) plot.getY() + 2, 40, 12, juce::Justification::centredLeft);
        }
    }
    g.setColour (panelEdge.brighter (0.3f));
    g.drawHorizontalLine ((int) midY, plot.getX(), plot.getRight());

    // Wellenform (min/max pro Pixel)
    auto drawWave = [&] (const std::vector<float>& x, double rate, juce::Colour c, bool filled)
    {
        if (x.empty()) return;
        juce::Path top, bottom;
        const int W = (int) plot.getWidth();
        bool first = true;
        juce::Path fill;
        std::vector<std::pair<float, float>> mm ((size_t) W);
        for (int px = 0; px < W; ++px)
        {
            const double t0 = (double) px / W * dur, t1 = (double) (px + 1) / W * dur;
            const size_t a = (size_t) (t0 * rate), b = std::min (x.size(), (size_t) (t1 * rate) + 1);
            float lo = 0, hi = 0;
            for (size_t i = a; i < b; ++i) { lo = std::min (lo, x[i]); hi = std::max (hi, x[i]); }
            mm[(size_t) px] = { lo, hi };
        }
        for (int px = 0; px < W; ++px)
        {
            const float X = plot.getX() + (float) px, Y = midY - mm[(size_t) px].second * halfH;
            if (first) { fill.startNewSubPath (X, Y); first = false; } else fill.lineTo (X, Y);
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

    float peak = 1e-6f;
    for (float v : render.audio) peak = std::max (peak, std::abs (v));
    if (mode < 2)
    {
        // für den Vergleich beide auf Spitze 1 normieren
        std::vector<float> norm (render.audio);
        for (auto& v : norm) v /= peak;
        drawWave (norm, sr, col, true);
    }
    else drawWave (render.audio, sr, col, true);
    if (ref) drawWave (ref->samples, ref->sampleRate, refCol, false);

    // Hüllkurve
    if (! render.amp.empty() && mode < 2)
    {
        juce::Path env;
        const int W = (int) plot.getWidth();
        float ampMax = 1e-6f; for (float a : render.amp) ampMax = std::max (ampMax, a);
        for (int px = 0; px < W; px += 2)
        {
            const size_t i = std::min (render.amp.size() - 1, (size_t) ((double) px / W * dur * sr));
            const float y = midY - render.amp[i] / ampMax * halfH;
            if (px == 0) env.startNewSubPath (plot.getX(), y); else env.lineTo (plot.getX() + (float) px, y);
        }
        juce::Path dashed;
        const float dashes[] = { 4.0f, 4.0f };
        juce::PathStrokeType (1.0f).createDashedStroke (dashed, env, dashes, 2);
        g.setColour (textCol.withAlpha (0.55f));
        g.fillPath (dashed);
    }

    // Tonhöhe (Kick) auf log. Skala rechts
    const float fLo = 20.0f, fHi = 12000.0f;
    auto yOfHz = [&] (float f) { return plot.getBottom() - (float) (std::log (f / fLo) / std::log (fHi / fLo)) * plot.getHeight(); };
    if (mode == 0 && ! render.freq.empty())
    {
        g.setFont (juce::FontOptions (10.0f));
        for (float f : { 30.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 3000.0f, 8000.0f })
        {
            const float y = yOfHz (f);
            g.setColour (accent2.withAlpha (0.18f));
            g.drawHorizontalLine ((int) y, plot.getRight(), plot.getRight() + 6.0f);
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

    // Kopfzeile
    g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
    g.setColour (col);
    g.drawText (info, 14, 5, getWidth() - 220, 18, juce::Justification::centredLeft);
    g.setFont (juce::FontOptions (11.0f));
    g.setColour (dimText);
    juce::String right = u8 ("⇢ in die DAW ziehen");
    if (ref) right = u8 ("Original: ") + ref->name + u8 ("   ·   ") + right;
    else if (lastRef != nullptr && showReference && mode < 2)
        right = u8 ("Original (") + (lastRef->type == 0 ? "Kick" : "Bass") + u8 (") in der anderen Ansicht   ·   ") + right;
    g.drawText (right, getWidth() - 420, 5, 406, 18, juce::Justification::centredRight);

    // Mauszeiger
    if (hoverX >= plot.getX() && hoverX <= plot.getRight())
    {
        const double t = (hoverX - plot.getX()) / plot.getWidth() * dur;
        g.setColour (textCol.withAlpha (0.4f));
        g.drawVerticalLine ((int) hoverX, plot.getY(), plot.getBottom());
        juce::String s = juce::String (t * 1000.0, 1) + " ms";
        const size_t i = (size_t) (t * sr);
        if (mode == 0 && i < render.freq.size() && render.freq[i] > 0.0f) s += u8 (" · ") + juce::String ((int) std::round (render.freq[i])) + " Hz";
        if (i < render.amp.size() && mode < 2 && render.amp[i] > 0.0f) s += u8 (" · ") + juce::String (20.0f * std::log10 (render.amp[i]), 1) + " dB";
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
    const auto file = proc.renderToFile (mode);
    if (file.existsAsFile())
        juce::DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this);
}

//==============================================================================
AnalysePage::AnalysePage (PsyForgeProcessor& p, std::function<void()> cb) : proc (p), onRefChanged (std::move (cb))
{
    for (auto* b : { &loadButton, &playRefButton, &analyseButton, &quickButton, &cancelButton, &applyButton, &undoButton })
        addAndMakeVisible (*b);
    typeBox.addItem ("Automatisch erkennen", 1);
    typeBox.addItem ("Ist eine Kick", 2);
    typeBox.addItem ("Ist ein Bass", 3);
    typeBox.setSelectedId (1, juce::dontSendNotification);
    addAndMakeVisible (typeBox);
    addAndMakeVisible (showRef);
    showRef.setToggleState (true, juce::dontSendNotification);
    addAndMakeVisible (progressBar);
    progressBar.setPercentageDisplay (false);

    analyseButton.setColour (juce::TextButton::buttonColourId, accent.withAlpha (0.35f));
    applyButton.setColour (juce::TextButton::buttonColourId, kickCol.withAlpha (0.3f));

    loadButton.setTooltip (u8 ("Ein One-Shot (einzelne Kick oder einzelne Bassnote) als WAV, AIFF oder FLAC laden."));
    analyseButton.setTooltip (u8 ("Analysiert das Original und passt danach alle Regler an, bis der Klang sehr nahe dran ist (ca. 10–30 s)."));
    quickButton.setTooltip (u8 ("Nur grobe Schätzung aus den Messwerten (sofort)."));
    applyButton.setTooltip (u8 ("Gefundene Einstellungen auf Kick bzw. Bass übertragen."));
    undoButton.setTooltip (u8 ("Zurück zu den Einstellungen vor dem Übernehmen."));

    loadButton.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("One-Shot laden", juce::File(), "*.wav;*.aif;*.aiff;*.flac");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc) { if (fc.getResult().existsAsFile()) loadFile (fc.getResult()); });
    };
    playRefButton.onClick = [this] { proc.auditionReference(); };
    auto start = [this] (bool fine)
    {
        const int sel = typeBox.getSelectedId();
        proc.startAnalysis (sel == 2 ? pf::SoundKick : (sel == 3 ? pf::SoundBass : pf::SoundAuto), fine);
        status = fine ? u8 ("Analysiere und passe an …") : u8 ("Analysiere …");
        update();
    };
    analyseButton.onClick = [start] { start (true); };
    quickButton.onClick = [start] { start (false); };
    cancelButton.onClick = [this] { proc.cancelAnalysis(); };
    applyButton.onClick = [this]
    {
        proc.applyAnalysisResult();
        status = u8 ("Übernommen. Mit ▶ Kick / ▶ Bass vergleichen.");
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
    for (auto& f : files)
        if (juce::File (f).hasFileExtension ("wav;aif;aiff;flac")) return true;
    return false;
}

void AnalysePage::filesDropped (const juce::StringArray& files, int, int)
{
    dragOver = false;
    for (auto& f : files)
        if (juce::File (f).hasFileExtension ("wav;aif;aiff;flac")) { loadFile (juce::File (f)); break; }
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
                   + u8 (". Mit „Übernehmen“ in Kick/Bass laden.");
        }
    }
    if (running)
        status = u8 ("Passe an … aktuelle Übereinstimmung ") + juce::String (juce::roundToInt (proc.getAnalysisLiveMatch())) + " %";
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
    auto inner = dropArea.reduced (16);
    g.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    g.setColour (textCol);
    g.drawText (ref ? ref->name : u8 ("Kick- oder Bass-One-Shot hierher ziehen"), inner.removeFromTop (24), juce::Justification::centredLeft);
    g.setFont (juce::FontOptions (12.5f));
    g.setColour (dimText);
    if (ref)
        g.drawText (juce::String ((double) ref->samples.size() / ref->sampleRate * 1000.0, 0) + " ms  ·  " + juce::String ((int) ref->sampleRate) + " Hz",
                    inner.removeFromTop (20), juce::Justification::centredLeft);
    else
        g.drawText (u8 ("WAV, AIFF oder FLAC – eine einzelne Kick oder eine einzelne Bassnote"), inner.removeFromTop (20), juce::Justification::centredLeft);
    inner.removeFromTop (10);
    g.setColour (textCol);
    g.setFont (juce::FontOptions (13.0f));
    g.drawFittedText (status, inner.removeFromTop (40), juce::Justification::topLeft, 3);
    g.setColour (kickCol);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawFittedText (summary, inner.removeFromTop (44), juce::Justification::topLeft, 3);

    // Tipps
    auto tips = getLocalBounds().withLeft (dropArea.getRight() + 280).reduced (8, 0);
    g.setColour (dimText);
    g.setFont (juce::FontOptions (12.0f));
    g.drawFittedText (u8 ("So kommst du am nächsten ran:\n"
                          "• Ein sauberes One-Shot ohne Hall/Delay verwenden.\n"
                          "• Eine Kick bzw. eine Bassnote allein – kein Loop.\n"
                          "• Nach dem Übernehmen mit „▶ Original“ und „▶ Kick/Bass“ vergleichen und von Hand nachregeln.\n"
                          "• Der Grundton wird exakt übernommen (Snap aus). Für deine Tonart Snap wieder einschalten.\n"
                          "• Bässe werden auf die nächste Note in deiner Stimmung (A4 oben) gesetzt – bei 432-Samples vorher A4 auf 432 stellen.\n"
                          "• Die Vorschau-Note wird auf die Note des Originals gesetzt."),
                      tips, juce::Justification::topLeft, 12);
}

void AnalysePage::resized()
{
    auto r = getLocalBounds();
    dropArea = r.removeFromLeft (560).withHeight (std::min (r.getHeight(), 250));
    auto col = r.withTrimmedLeft (16).withWidth (250);
    auto row = [&] (int h) { auto x = col.removeFromTop (h); col.removeFromTop (6); return x; };
    loadButton.setBounds (row (28));
    playRefButton.setBounds (row (28));
    typeBox.setBounds (row (26));
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
} // namespace pfui

//==============================================================================
using namespace pfui;

PsyForgeEditor::PsyForgeEditor (PsyForgeProcessor& p) : AudioProcessorEditor (&p), proc (p)
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

    content.addAndMakeVisible (presetBox);
    presetBox.onChange = [this] { presetChosen(); };
    for (auto* b : { &prevPreset, &nextPreset, &savePreset }) content.addAndMakeVisible (*b);
    prevPreset.onClick = [this] { stepPreset (-1); };
    nextPreset.onClick = [this] { stepPreset (+1); };
    savePreset.onClick = [this] { savePresetDialog(); };
    buildPresetMenu();

    auto mk = [this] (const char* id, juce::Colour c)
    {
        auto ctl = std::make_unique<ParamControl> (proc, *pf::findParam (id), c);
        content.addAndMakeVisible (*ctl);
        return ctl;
    };
    keyCtl = mk ("g_key", accent2);
    a4Ctl = mk ("g_a4", accent2);
    kickChCtl = mk ("g_kickch", kickCol);
    bassChCtl = mk ("g_bassch", bassCol);
    masterCtl = mk ("g_master", accent);
    limitCtl = mk ("g_limit", accent);
    for (auto* b : { &tune440, &tune432 }) content.addAndMakeVisible (*b);
    tune440.onClick = [this] { proc.setParam ("g_a4", 440.0f); };
    tune432.onClick = [this] { proc.setParam ("g_a4", 432.0f); };
    tune432.setTooltip (u8 ("Kammerton auf 432 Hz stellen"));
    tune440.setTooltip (u8 ("Kammerton auf 440 Hz stellen"));

    wave = std::make_unique<WaveView> (proc);
    content.addAndMakeVisible (*wave);
    for (auto* b : { &viewKick, &viewBass, &viewLoop })
    {
        content.addAndMakeVisible (*b);
        b->setClickingTogglesState (false);
    }
    viewKick.onClick = [this] { wave->setMode (0); };
    viewBass.onClick = [this] { wave->setMode (1); };
    viewLoop.onClick = [this] { wave->setMode (2); };

    for (auto* b : { &playKick, &playBass, &loopButton }) content.addAndMakeVisible (*b);
    playKick.setColour (juce::TextButton::buttonColourId, kickCol.withAlpha (0.25f));
    playBass.setColour (juce::TextButton::buttonColourId, bassCol.withAlpha (0.25f));
    playKick.onClick = [this] { proc.auditionKick(); };
    playBass.onClick = [this] { proc.auditionBass(); };
    loopButton.setClickingTogglesState (true);
    loopButton.setTooltip (u8 ("Vorschau-Schleife (Kick + Bass im gewählten Muster) – läuft mit der DAW synchron, wenn sie spielt."));
    loopButton.onClick = [this] { proc.loopOn.store (loopButton.getToggleState()); };
    loopButton.setToggleState (proc.loopOn.load(), juce::dontSendNotification);
    patternCtl = mk ("g_pattern", loopCol);
    noteCtl = mk ("g_note", bassCol);
    bpmCtl = mk ("g_bpm", loopCol);

    for (auto* b : { &expKick, &expBass, &expLoop }) content.addAndMakeVisible (*b);
    expKick.onClick = [this] { exportDialog (PsyForgeProcessor::ExportKick); };
    expBass.onClick = [this] { exportDialog (PsyForgeProcessor::ExportBass); };
    expLoop.onClick = [this] { exportDialog (PsyForgeProcessor::ExportLoop); };
    exportHint.setText (u8 ("WAV speichern (24 Bit) – oder die Wellenform direkt in die DAW ziehen."), juce::dontSendNotification);
    exportHint.setFont (juce::FontOptions (11.0f));
    exportHint.setColour (juce::Label::textColourId, dimText);
    content.addAndMakeVisible (exportHint);

    // Seiten
    kickPage.sections.add (new Section (proc, "PITCH", { "k_start", "k_punch", "k_end", "k_clickt", "k_sweep", "k_drift", "k_snap", "k_shape", "k_phase", "k_knock", "k_follow" }, kickCol));
    kickPage.sections.add (new Section (proc, "CLICK", { "k_clvl", "k_clhz", "k_cldec", "k_cltone" }, accent2));
    kickPage.sections.add (new Section (proc, u8 ("LAUTSTÄRKE / FISHTAIL"), { "k_level", "k_attack", "k_hold", "k_decay", "k_length", "k_sync", "k_fade", "k_fdepth", "k_fpos", "k_fwidth", "k_vel" }, kickCol));
    kickPage.sections.add (new Section (proc, "DRIVE", { "k_drive", "k_dtype", "k_trans" }, accent));
    kickPage.sections.add (new Section (proc, "EQ", { "k_lowcut", "k_holehz", "k_holedb", "k_holeq", "k_highcut" }, refCol));
    bassPage.sections.add (new Section (proc, "OSZILLATOR", { "b_level", "b_oct", "b_fine", "b_saw", "b_square", "b_pw", "b_detune", "b_sub", "b_suboct", "b_subinv", "b_retrig", "b_phase" }, bassCol));
    bassPage.sections.add (new Section (proc, u8 ("LAUTSTÄRKE"), { "b_attack", "b_decay", "b_sustain", "b_release", "b_vel", "b_offset" }, bassCol));
    bassPage.sections.add (new Section (proc, "FILTER", { "b_cutoff", "b_reso", "b_fenv", "b_fatt", "b_fdec", "b_key", "b_slope" }, accent2));
    bassPage.sections.add (new Section (proc, "PLUCK / DRIVE / DUCKING", { "b_pluck", "b_pldec", "b_drive", "b_dtype", "b_duck", "b_duckrel" }, accent));
    bassPage.sections.add (new Section (proc, "EQ", { "b_lowcut", "b_notch", "b_highcut" }, refCol));
    for (auto* s : kickPage.sections) kickPage.addAndMakeVisible (s);
    for (auto* s : bassPage.sections) bassPage.addAndMakeVisible (s);
    content.addChildComponent (kickPage);
    content.addChildComponent (bassPage);
    analysePage = std::make_unique<AnalysePage> (proc, [this]
    {
        wave->showReference = analysePage->showRef.getToggleState();
        const auto r = proc.getAnalysisResult();
        if (proc.getReference() && r.ok) wave->setMode (r.type == pf::SoundKick ? 0 : 1);
        wave->refresh();
    });
    content.addChildComponent (*analysePage);

    for (auto* b : { &tabKick, &tabBass, &tabAnalyse }) content.addAndMakeVisible (*b);
    tabKick.onClick = [this] { showPage (0); };
    tabBass.onClick = [this] { showPage (1); };
    tabAnalyse.onClick = [this] { showPage (2); };
    showPage (0);

    setResizable (true, true);
    const double ratio = (double) baseW / baseH;
    if (auto* c = getConstrainer()) { c->setFixedAspectRatio (ratio); c->setSizeLimits (baseW * 6 / 10, baseH * 6 / 10, baseW * 2, baseH * 2); }
    const int w = juce::jlimit (baseW * 6 / 10, baseW * 2, proc.getUiWidth());
    setSize (w, (int) std::round (w / ratio));
    startTimerHz (20);
}

PsyForgeEditor::~PsyForgeEditor()
{
    setLookAndFeel (nullptr);
}

void PsyForgeEditor::paint (juce::Graphics& g) { g.fillAll (bgCol); }

void PsyForgeEditor::resized()
{
    const float s = (float) getWidth() / (float) baseW;
    content.setBounds (0, 0, baseW, baseH);
    content.setTransform (juce::AffineTransform::scale (s));
    proc.setUiWidth (getWidth());
}

bool PsyForgeEditor::isInterestedInFileDrag (const juce::StringArray& files) { return analysePage->isInterestedInFileDrag (files); }

void PsyForgeEditor::filesDropped (const juce::StringArray& files, int x, int y)
{
    showPage (2);
    analysePage->filesDropped (files, x, y);
}

void PsyForgeEditor::Content::paint (juce::Graphics& g)
{
    g.fillAll (bgCol);
    // dezenter Verlauf oben
    g.setGradientFill (juce::ColourGradient (accent.withAlpha (0.10f), 0, 0, bgCol, 0, 70, false));
    g.fillRect (0, 0, getWidth(), 70);
    g.setColour (dimText);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText (u8 ("Goa · Psytrance · Full-On  —  Kick & Bass"), 16, 38, 300, 14, juce::Justification::centredLeft);

    // Pegelanzeige
    auto m = ed.meterArea.toFloat();
    if (! m.isEmpty())
    {
        auto bar = [&] (juce::Rectangle<float> r, float v, juce::Colour c, const char* name)
        {
            g.setColour (panelCol); g.fillRoundedRectangle (r, 2.0f);
            const float h = juce::jlimit (0.0f, 1.0f, (20.0f * std::log10 (std::max (v, 1e-4f)) + 48.0f) / 48.0f) * r.getHeight();
            g.setColour (v > 0.98f ? juce::Colours::red : c);
            g.fillRoundedRectangle (r.withTop (r.getBottom() - h), 2.0f);
            g.setColour (dimText);
            g.setFont (juce::FontOptions (9.0f));
            g.drawText (name, (int) r.getX() - 4, (int) r.getBottom() + 1, (int) r.getWidth() + 8, 10, juce::Justification::centred);
        };
        const float w = 8.0f;
        bar ({ m.getX(), m.getY(), w, m.getHeight() - 12 }, ed.meterK, kickCol, "K");
        bar ({ m.getX() + 12, m.getY(), w, m.getHeight() - 12 }, ed.meterB, bassCol, "B");
        bar ({ m.getX() + 24, m.getY(), w, m.getHeight() - 12 }, ed.meterO, accent, "M");
    }
}

void PsyForgeEditor::Content::resized()
{
    auto& e = ed;
    auto r = getLocalBounds().reduced (12, 0);
    auto top = r.removeFromTop (66);
    e.titleLabel.setBounds (top.getX() + 2, 8, 200, 30);
    auto pr = juce::Rectangle<int> (230, 8, 380, 52);
    e.presetCaption.setBounds (pr.removeFromTop (14));
    {
        auto row = pr.withHeight (28);
        e.prevPreset.setBounds (row.removeFromLeft (28)); row.removeFromLeft (4);
        e.savePreset.setBounds (row.removeFromRight (80)); row.removeFromRight (4);
        e.nextPreset.setBounds (row.removeFromRight (28)); row.removeFromRight (4);
        e.presetBox.setBounds (row);
    }
    int x = 630;
    auto place = [&] (juce::Component& c, int w) { c.setBounds (x, 4, w, 60); x += w + 6; };
    place (*e.keyCtl, 66);
    place (*e.a4Ctl, 64);
    e.tune440.setBounds (x, 12, 40, 20); e.tune432.setBounds (x, 36, 40, 20); x += 46;
    place (*e.kickChCtl, 74);
    place (*e.bassChCtl, 74);
    place (*e.limitCtl, 50);
    place (*e.masterCtl, 64);
    e.meterArea = { x + 4, 8, 40, 54 };

    r.removeFromTop (4);
    auto mid = r.removeFromTop (282);
    auto side = mid.removeFromRight (270);
    mid.removeFromRight (10);
    e.wave->setBounds (mid);

    auto sr = side;
    {
        auto row = sr.removeFromTop (28);
        const int w = (row.getWidth() - 8) / 3;
        e.viewKick.setBounds (row.removeFromLeft (w)); row.removeFromLeft (4);
        e.viewBass.setBounds (row.removeFromLeft (w)); row.removeFromLeft (4);
        e.viewLoop.setBounds (row);
    }
    sr.removeFromTop (8);
    {
        auto row = sr.removeFromTop (34);
        const int w = (row.getWidth() - 8) / 3;
        e.playKick.setBounds (row.removeFromLeft (w)); row.removeFromLeft (4);
        e.playBass.setBounds (row.removeFromLeft (w)); row.removeFromLeft (4);
        e.loopButton.setBounds (row);
    }
    sr.removeFromTop (6);
    {
        auto row = sr.removeFromTop (62);
        e.patternCtl->setBounds (row.removeFromLeft (110)); row.removeFromLeft (4);
        e.noteCtl->setBounds (row.removeFromLeft (80)); row.removeFromLeft (4);
        e.bpmCtl->setBounds (row);
    }
    sr.removeFromTop (10);
    {
        auto row = sr.removeFromTop (30);
        const int w = (row.getWidth() - 8) / 3;
        e.expKick.setBounds (row.removeFromLeft (w)); row.removeFromLeft (4);
        e.expBass.setBounds (row.removeFromLeft (w)); row.removeFromLeft (4);
        e.expLoop.setBounds (row);
    }
    e.exportHint.setBounds (sr.removeFromTop (34));

    r.removeFromTop (10);
    {
        auto tabs = r.removeFromTop (28);
        e.tabKick.setBounds (tabs.removeFromLeft (110)); tabs.removeFromLeft (4);
        e.tabBass.setBounds (tabs.removeFromLeft (110)); tabs.removeFromLeft (4);
        e.tabAnalyse.setBounds (tabs.removeFromLeft (130));
    }
    r.removeFromTop (8);
    e.kickPage.setBounds (r);
    e.bassPage.setBounds (r);
    e.analysePage->setBounds (r);
}

void PsyForgeEditor::showPage (int p)
{
    page = p;
    kickPage.setVisible (p == 0);
    bassPage.setVisible (p == 1);
    analysePage->setVisible (p == 2);
    tabKick.setToggleState (p == 0, juce::dontSendNotification);
    tabBass.setToggleState (p == 1, juce::dontSendNotification);
    tabAnalyse.setToggleState (p == 2, juce::dontSendNotification);
    if (p == 0) wave->setMode (0);
    if (p == 1) wave->setMode (1);
}

void PsyForgeEditor::timerCallback()
{
    const int mode = wave->getMode();
    viewKick.setToggleState (mode == 0, juce::dontSendNotification);
    viewBass.setToggleState (mode == 1, juce::dontSendNotification);
    viewLoop.setToggleState (mode == 2, juce::dontSendNotification);
    if (loopButton.getToggleState() != proc.loopOn.load()) loopButton.setToggleState (proc.loopOn.load(), juce::dontSendNotification);

    if (proc.getPresetName() != lastPresetName)
    {
        lastPresetName = proc.getPresetName();
        buildPresetMenu();
    }
    meterK = proc.uiKickLevel.load(); meterB = proc.uiBassLevel.load(); meterO = proc.uiOutLevel.load();
    content.repaint (meterArea);
    if (page == 2 || proc.isAnalysing()) analysePage->update();
}

//==============================================================================
void PsyForgeEditor::buildPresetMenu()
{
    presetBox.clear (juce::dontSendNotification);
    const auto& fp = pf::factoryPresets();
    juce::String lastCat;
    for (int i = 0; i < (int) fp.size(); ++i)
    {
        const auto cat = juce::String::fromUTF8 (fp[(size_t) i].category);
        if (cat != lastCat) { presetBox.addSectionHeading (cat); lastCat = cat; }
        presetBox.addItem (juce::String::fromUTF8 (fp[(size_t) i].name), i + 1);
    }
    userPresets = proc.listUserPresets();
    if (! userPresets.isEmpty())
    {
        presetBox.addSectionHeading ("Eigene Presets");
        for (int i = 0; i < userPresets.size(); ++i) presetBox.addItem (userPresets[i].getFileNameWithoutExtension(), 1000 + i);
    }
    if (proc.getPresetIndex() >= 0) presetBox.setSelectedId (proc.getPresetIndex() + 1, juce::dontSendNotification);
    else
    {
        bool found = false;
        for (int i = 0; i < userPresets.size(); ++i)
            if (userPresets[i].getFileNameWithoutExtension() == proc.getPresetName()) { presetBox.setSelectedId (1000 + i, juce::dontSendNotification); found = true; }
        if (! found) presetBox.setText (proc.getPresetName(), juce::dontSendNotification);
    }
}

void PsyForgeEditor::presetChosen()
{
    const int id = presetBox.getSelectedId();
    if (id <= 0) return;
    if (id >= 1000) { if (id - 1000 < userPresets.size()) proc.loadUserPreset (userPresets[id - 1000]); }
    else proc.loadFactoryPreset (id - 1);
    lastPresetName = proc.getPresetName();
    wave->refresh();
}

void PsyForgeEditor::stepPreset (int delta)
{
    std::vector<int> ids;
    for (int i = 0; i < (int) pf::factoryPresets().size(); ++i) ids.push_back (i + 1);
    for (int i = 0; i < userPresets.size(); ++i) ids.push_back (1000 + i);
    const int cur = presetBox.getSelectedId();
    auto it = std::find (ids.begin(), ids.end(), cur);
    int idx = it == ids.end() ? 0 : (int) (it - ids.begin());
    idx = (idx + delta + (int) ids.size()) % (int) ids.size();
    presetBox.setSelectedId (ids[(size_t) idx], juce::sendNotificationSync);
}

void PsyForgeEditor::savePresetDialog()
{
    auto* aw = new juce::AlertWindow ("Preset speichern", "Name des Presets:", juce::MessageBoxIconType::NoIcon, this);
    aw->addTextEditor ("name", proc.getPresetName().startsWith ("Analyse") ? proc.getPresetName().fromFirstOccurrenceOf (": ", false, false) : proc.getPresetName());
    aw->addButton ("Speichern", 1, juce::KeyPress (juce::KeyPress::returnKey));
    aw->addButton ("Abbrechen", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    aw->enterModalState (true, juce::ModalCallbackFunction::create ([this, aw] (int result)
    {
        if (result != 1) return;
        const auto name = aw->getTextEditorContents ("name").trim();
        if (name.isEmpty()) return;
        if (proc.saveUserPreset (name)) { lastPresetName.clear(); }
        else juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "PsyForge",
                                                     "Preset konnte nicht gespeichert werden:\n" + PsyForgeProcessor::presetDirectory().getFullPathName());
    }), true);
}

void PsyForgeEditor::exportDialog (int what)
{
    const auto start = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("PsyForge").getChildFile (proc.suggestedFileName (what));
    start.getParentDirectory().createDirectory();
    chooser = std::make_unique<juce::FileChooser> ("WAV speichern", start, "*.wav");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this, what] (const juce::FileChooser& fc)
    {
        auto f = fc.getResult();
        if (f == juce::File()) return;
        if (! f.hasFileExtension ("wav")) f = f.withFileExtension ("wav");
        if (proc.renderToFile (what, f) == juce::File())
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "PsyForge", "WAV konnte nicht gespeichert werden.");
    });
}
