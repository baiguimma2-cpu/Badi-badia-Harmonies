#include "PluginEditor.h"

namespace
{
    const juce::Colour kGold   { 0xfff5c451 };
    const juce::Colour kPink   { 0xffec4899 };
    const juce::Colour kViolet { 0xff8b5cf6 };
    const juce::Colour kCard   { 0xff1d1a35 };
    const juce::Colour kLine   { 0xff2a2645 };

    juce::Font F (float h, bool bold = false)
    {
        return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
    }

    const char* const kVoiceNames[HarmonyProcessor::numVoices] =
        { "Unisson", "Tierce ↑", "Quinte ↑", "Octave ↑", "Tierce ↓", "Octave ↓" };

    struct Preset { const char* name; int mask; float wet; };
    const Preset kPresets[5] = {
        { "Duo",          1 << 1,                                   0.70f },
        { "Trio",         (1 << 1) | (1 << 2),                      0.75f },
        { "Chœur gospel", (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4), 0.85f },
        { "Ange",         (1 << 3) | (1 << 1) | (1 << 2),           0.80f },
        { "Profond",      (1 << 5) | (1 << 4) | (1 << 0),           0.75f }
    };

    // Logo « BH » (coordonnées 100x100)
    void drawLogo (juce::Graphics& g, juce::Rectangle<float> r)
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a1458), r.getX(), r.getY(),
                                                 kPink, r.getRight(), r.getBottom(), false));
        g.fillRoundedRectangle (r, r.getWidth() * 0.26f);
        g.setColour (juce::Colours::white.withAlpha (0.25f));
        g.drawRoundedRectangle (r.reduced (0.5f), r.getWidth() * 0.26f, 1.f);

        const float pi = juce::MathConstants<float>::pi;
        juce::Path b, h;
        b.startNewSubPath (24.f, 71.f);
        b.lineTo (24.f, 30.f);
        b.lineTo (39.f, 30.f);
        b.addCentredArc (39.f, 40.25f, 10.25f, 10.25f, 0.f, 0.f, pi, false);
        b.lineTo (24.f, 50.5f);
        b.startNewSubPath (39.f, 50.5f);
        b.addCentredArc (39.f, 60.75f, 10.25f, 10.25f, 0.f, 0.f, pi, false);
        b.lineTo (24.f, 71.f);

        h.startNewSubPath (61.f, 30.f);  h.lineTo (61.f, 71.f);
        h.startNewSubPath (80.f, 30.f);  h.lineTo (80.f, 71.f);
        juce::Path wave;
        wave.startNewSubPath (61.f, 50.5f);
        wave.quadraticTo (65.75f, 41.5f, 70.5f, 50.5f);
        wave.quadraticTo (75.25f, 59.5f, 80.f, 50.5f);

        const float s = r.getWidth() / 100.f;
        const auto tf = juce::AffineTransform::scale (s).translated (r.getX(), r.getY());
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfffff3c4), 0.f, r.getY() + 30.f * s,
                                                 kGold, 0.f, r.getY() + 71.f * s, false));
        g.strokePath (b, juce::PathStrokeType (6.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded), tf);
        g.strokePath (h, juce::PathStrokeType (6.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded), tf);
        g.strokePath (wave, juce::PathStrokeType (4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded), tf);
    }
}

// ---------------------------------------------------------------- Look & feel
BBLookAndFeel::BBLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, kCard);
    setColour (juce::ComboBox::textColourId, juce::Colours::white);
    setColour (juce::ComboBox::outlineColourId, kLine);
    setColour (juce::ComboBox::arrowColourId, kGold);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff151326));
    setColour (juce::PopupMenu::textColourId, juce::Colours::white);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, kViolet);
    setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
}

void BBLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hl, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.f);
    const bool on = b.getToggleState();
    if (on)
        g.setGradientFill (juce::ColourGradient (kViolet.withAlpha (0.35f), r.getX(), r.getY(),
                                                 kPink.withAlpha (0.35f), r.getRight(), r.getBottom(), false));
    else
        g.setColour (hl ? kCard.brighter (0.1f) : kCard);
    g.fillRoundedRectangle (r, 14.f);
    g.setColour (on ? kViolet : kLine);
    g.drawRoundedRectangle (r, 14.f, 1.4f);

    g.setColour (juce::Colours::white);
    g.setFont (F (15.f));
    g.drawText (b.getButtonText(), r.reduced (14.f, 0.f), juce::Justification::centredLeft);
    g.setColour (kGold);
    g.setFont (F (14.f, true));
    g.drawText (b.getProperties()["note"].toString(), r.reduced (14.f, 0.f), juce::Justification::centredRight);
}

void BBLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                      float start, float end, juce::Slider&)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (8.f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float tr = r - 5.f;
    const float ang = start + pos * (end - start);
    const juce::PathStrokeType st (6.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    juce::Path track, val;
    track.addCentredArc (c.x, c.y, tr, tr, 0.f, start, end, true);
    val.addCentredArc (c.x, c.y, tr, tr, 0.f, start, ang, true);
    g.setColour (kLine);
    g.strokePath (track, st);
    g.setGradientFill (juce::ColourGradient (kGold, c.x - tr, c.y, kPink, c.x + tr, c.y, false));
    g.strokePath (val, st);

    g.setColour (kCard);
    g.fillEllipse (juce::Rectangle<float> (tr * 1.25f, tr * 1.25f).withCentre (c));
    juce::Path p;
    p.addRoundedRectangle (-1.5f, -tr * 0.62f, 3.f, tr * 0.36f, 1.5f);
    g.setColour (juce::Colours::white);
    g.fillPath (p, juce::AffineTransform::rotation (ang).translated (c.x, c.y));
}

void BBLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                          bool hl, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.f);
    g.setColour (down ? kViolet.withAlpha (0.5f) : (hl ? kCard.brighter (0.15f) : kCard));
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (hl ? kGold : kLine);
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.2f);
}

// ---------------------------------------------------------------- Éditeur
HarmonyEditor::HarmonyEditor (HarmonyProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    setSize (560, 572);

    addAndMakeVisible (keyBox);
    addAndMakeVisible (modeBox);
    keyAtt = std::make_unique<CA> (proc.apvts, "key", keyBox);
    modeAtt = std::make_unique<CA> (proc.apvts, "mode", modeBox);

    for (int i = 0; i < HarmonyProcessor::numVoices; ++i)
    {
        voiceBtn[i].setButtonText (juce::String::fromUTF8 (kVoiceNames[i]));
        addAndMakeVisible (voiceBtn[i]);
        voiceAtt[i] = std::make_unique<BA> (proc.apvts, "v" + juce::String (i), voiceBtn[i]);
    }

    auto setupKnob = [this] (juce::Slider& s)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
        addAndMakeVisible (s);
    };
    setupKnob (dryS);
    setupKnob (wetS);
    setupKnob (widthS);
    dryAtt = std::make_unique<SA> (proc.apvts, "dry", dryS);
    wetAtt = std::make_unique<SA> (proc.apvts, "wet", wetS);
    widthAtt = std::make_unique<SA> (proc.apvts, "width", widthS);

    for (int i = 0; i < 5; ++i)
    {
        presetBtn[i].setButtonText (juce::String::fromUTF8 (kPresets[i].name));
        addAndMakeVisible (presetBtn[i]);
        presetBtn[i].onClick = [this, i]
        {
            auto setP = [this] (const juce::String& id, float v)
            {
                if (auto* prm = proc.apvts.getParameter (id))
                {
                    prm->beginChangeGesture();
                    prm->setValueNotifyingHost (v);
                    prm->endChangeGesture();
                }
            };
            for (int v = 0; v < HarmonyProcessor::numVoices; ++v)
                setP ("v" + juce::String (v), (kPresets[i].mask >> v) & 1 ? 1.f : 0.f);
            setP ("wet", kPresets[i].wet);
        };
    }

    startTimerHz (24);
}

HarmonyEditor::~HarmonyEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void HarmonyEditor::paint (juce::Graphics& g)
{
    const float w = (float) getWidth();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1a0d38), 0.f, 0.f,
                                             juce::Colour (0xff0a0912), 0.f, (float) getHeight(), false));
    g.fillAll();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0x44ec4899), w, 0.f,
                                             juce::Colours::transparentBlack, w - 320.f, 220.f, true));
    g.fillRect (getLocalBounds());

    drawLogo (g, { 20.f, 16.f, 64.f, 64.f });
    g.setGradientFill (juce::ColourGradient (kGold, 96.f, 0.f, kPink, 340.f, 0.f, false));
    g.setFont (F (28.f, true));
    g.drawText ("Badi-badia Harmony", 96, 16, 440, 36, juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xff9a95bd));
    g.setFont (F (14.f));
    g.drawText (juce::String::fromUTF8 ("Une seule voix. Un chœur entier."), 98, 50, 440, 22,
                juce::Justification::centredLeft);

    // Halo qui pulse avec le niveau + note détectée
    const float cx = (float) noteArea.getCentreX(), cy = (float) noteArea.getCentreY() - 6.f;
    const float rad = 52.f + lvl * 40.f;
    g.setGradientFill (juce::ColourGradient (kPink.withAlpha (0.25f + 0.4f * lvl), cx, cy,
                                             juce::Colours::transparentBlack, cx + rad, cy, true));
    g.fillEllipse (cx - rad, cy - rad, rad * 2.f, rad * 2.f);

    g.setGradientFill (juce::ColourGradient (kGold, cx - 60.f, cy, kPink, cx + 60.f, cy, false));
    g.setFont (F (64.f, true));
    g.drawText (noteText, noteArea.withTrimmedBottom (20), juce::Justification::centred);
    g.setColour (juce::Colour (0xff9a95bd));
    g.setFont (F (13.f));
    g.drawText (juce::String::fromUTF8 ("Note détectée en direct"), noteArea.withTop (noteArea.getBottom() - 22),
                juce::Justification::centred);

    g.setColour (juce::Colours::white);
    g.setFont (F (14.f, true));
    const int ky = 440;
    g.drawText ("Ma voix", 20, ky - 4, 173, 18, juce::Justification::centred);
    g.drawText ("Harmonies", 193, ky - 4, 173, 18, juce::Justification::centred);
    g.drawText ("Largeur stéréo", 366, ky - 4, 173, 18, juce::Justification::centred);
}

void HarmonyEditor::resized()
{
    keyBox.setBounds (20, 204, 255, 32);
    modeBox.setBounds (285, 204, 255, 32);

    const int pw = (520 - 4 * 8) / 5;
    for (int i = 0; i < 5; ++i)
        presetBtn[i].setBounds (20 + i * (pw + 8), 250, pw, 30);

    for (int i = 0; i < HarmonyProcessor::numVoices; ++i)
        voiceBtn[i].setBounds (20 + (i % 2) * 264, 294 + (i / 2) * 48, 256, 40);

    dryS.setBounds (20, 458, 173, 100);
    wetS.setBounds (193, 458, 173, 100);
    widthS.setBounds (366, 458, 173, 100);
}

void HarmonyEditor::timerCallback()
{
    const float m = proc.detectedMidi.load();
    const juce::String s = m < 0.f ? juce::String::fromUTF8 ("—") : HarmonyProcessor::noteName (m);
    bool dirty = false;
    if (s != noteText) { noteText = s; dirty = true; }

    lvl = juce::jmax (proc.level.load(), lvl * 0.88f);
    if (std::abs (lvl - lastLvl) > 0.01f) { lastLvl = lvl; dirty = true; }

    for (int i = 0; i < HarmonyProcessor::numVoices; ++i)
    {
        const juce::String n = (voiceBtn[i].getToggleState() && m >= 0.f)
                                   ? HarmonyProcessor::noteName (proc.voiceMidi[i].load())
                                   : juce::String();
        if (n != voiceBtn[i].getProperties()["note"].toString())
        {
            voiceBtn[i].getProperties().set ("note", n);
            voiceBtn[i].repaint();
        }
    }
    if (dirty) repaint (noteArea);
}
