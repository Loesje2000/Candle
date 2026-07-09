#include "CandleLookAndFeel.h"

CandleLookAndFeel::CandleLookAndFeel()
{
    setColour (juce::Slider::rotarySliderFillColourId,    CandleColours::amber);
    setColour (juce::Slider::rotarySliderOutlineColourId, CandleColours::panelBorder.withAlpha (0.40f));
    setColour (juce::Slider::textBoxTextColourId,         CandleColours::amberLo);
    setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId,   juce::Colours::transparentBlack);

    setColour (juce::ComboBox::backgroundColourId,   CandleColours::btnOff.withAlpha (0.65f));
    setColour (juce::ComboBox::outlineColourId,      CandleColours::panelBorder.withAlpha (0.45f));
    setColour (juce::ComboBox::textColourId,         CandleColours::amber);
    setColour (juce::ComboBox::arrowColourId,        CandleColours::amber);
    setColour (juce::PopupMenu::backgroundColourId,              CandleColours::bg);
    setColour (juce::PopupMenu::textColourId,                    CandleColours::amberLo);
    setColour (juce::PopupMenu::highlightedBackgroundColourId,   juce::Colour (0xff2b2018));
    setColour (juce::PopupMenu::highlightedTextColourId,         CandleColours::amber);
}

juce::Font CandleLookAndFeel::getGeorgiaFont (float size, int style) const
{
    const auto styleName = (style & juce::Font::bold) != 0 && (style & juce::Font::italic) != 0 ? "Bold Italic"
                         : (style & juce::Font::bold) != 0 ? "Bold"
                         : (style & juce::Font::italic) != 0 ? "Italic"
                         : "Regular";
    return juce::Font (juce::FontOptions()
        .withName ("Georgia")
        .withStyle (styleName)
        .withHeight (size));
}

juce::Font CandleLookAndFeel::getMonoFont (float size) const
{
    return juce::Font (juce::FontOptions().withName ("Courier New").withStyle ("Regular").withHeight (size));
}

// ── Rotary knob ──────────────────────────────────────────────────────────────
void CandleLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                           float sliderPos, float startAngle, float endAngle,
                                           juce::Slider&)
{
    const float size = (float) juce::jmin (w, h);
    const float radius = size * 0.5f - 2.0f;
    const float cx = (float) x + (float) w * 0.5f;
    const float cy = (float) y + (float) h * 0.5f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    const float strokeW = juce::jmax (2.0f, size * 0.032f);
    const float dotR = juce::jmax (2.5f, size * 0.035f);
    const float arcR = radius - strokeW * 0.5f - 2.0f;

    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.fillEllipse (cx - radius + 1.0f, cy - radius + 1.0f, radius * 2.0f - 2.0f, radius * 2.0f - 2.0f);

    juce::Path track;
    track.addCentredArc (cx, cy, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (CandleColours::knobTrack.withAlpha (0.55f));
    g.strokePath (track, juce::PathStrokeType (strokeW, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc (cx, cy, arcR, arcR, 0.0f, startAngle, angle, true);
    g.setColour (CandleColours::amberGlow);
    g.strokePath (value, juce::PathStrokeType (strokeW + size * 0.035f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
    g.setColour (CandleColours::amber);
    g.strokePath (value, juce::PathStrokeType (strokeW, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));

    const float capR = radius * 0.76f;
    juce::Rectangle<float> cap (cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);
    juce::ColourGradient body (CandleColours::knobBodyHi,
                               cap.getX() + cap.getWidth() * 0.35f,
                               cap.getY() + cap.getHeight() * 0.30f,
                               CandleColours::bgDeep, cap.getRight(), cap.getBottom(), true);
    body.addColour (0.55, CandleColours::knobBody);
    g.setGradientFill (body);
    g.fillEllipse (cap);

    juce::ColourGradient gloss (juce::Colour (0x2effdcaa), cx - radius * 0.35f, cy - radius * 0.52f,
                                juce::Colours::transparentBlack, cx - radius * 0.35f, cy - radius * 0.05f, false);
    gloss.addColour (0.40, juce::Colour (0x0affdcaa));
    g.setGradientFill (gloss);
    g.fillEllipse (cx - radius * 0.58f, cy - radius * 0.58f, radius * 0.90f, radius * 0.60f);

    juce::ColourGradient rim (juce::Colours::transparentBlack, cx, cy,
                              juce::Colours::black.withAlpha (0.85f), cx + capR, cy, true);
    rim.addColour (0.80, juce::Colours::transparentBlack);
    rim.addColour (0.98, juce::Colours::black.withAlpha (0.55f));
    g.setGradientFill (rim);
    g.fillEllipse (cap);

    const float dotX = cx + std::sin (angle) * radius * 0.55f;
    const float dotY = cy - std::cos (angle) * radius * 0.55f;
    g.setColour (CandleColours::amber.withAlpha (0.33f));
    g.fillEllipse (dotX - dotR - size * 0.025f, dotY - dotR - size * 0.025f,
                   (dotR + size * 0.025f) * 2.0f, (dotR + size * 0.025f) * 2.0f);
    g.setColour (CandleColours::knobDot);
    g.fillEllipse (dotX - dotR, dotY - dotR, dotR * 2.0f, dotR * 2.0f);
}

// ── Buttons ──────────────────────────────────────────────────────────────────
void CandleLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& btn,
                                               const juce::Colour&, bool over, bool down)
{
    const auto  r  = btn.getLocalBounds().toFloat().reduced (0.5f);
    const bool  on = btn.getToggleState();
    if (on)
    {
        g.setColour (juce::Colour (0x423c220d));
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (CandleColours::amber.withAlpha (0.13f));
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (CandleColours::amber.withAlpha (0.70f));
        g.drawRoundedRectangle (r.reduced (-1.0f), 4.0f, 1.4f);
    }
    else
    {
        g.setColour (CandleColours::btnOff.withAlpha (0.55f));
        g.fillRoundedRectangle (r, 3.0f);
    }

    if (over || down)
    {
        g.setColour (CandleColours::amber.withAlpha (0.13f));
        g.fillRoundedRectangle (r, 3.0f);
    }

    g.setColour (on ? CandleColours::amber.withAlpha (0.70f) : CandleColours::panelRule.withAlpha (0.45f));
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
}

void CandleLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& btn, bool, bool)
{
    const float fitByHeight = (float) btn.getHeight() * 0.50f;
    const float fitByWidth = (float) btn.getWidth() / juce::jmax (2.5f, (float) btn.getButtonText().length() * 0.62f);
    g.setFont (getGeorgiaFont (juce::jlimit (7.0f, 12.0f, juce::jmin (fitByHeight, fitByWidth))));
    g.setColour (btn.getToggleState() ? CandleColours::creamBright : CandleColours::amberLo);
    g.drawText (btn.getButtonText(), btn.getLocalBounds().reduced (6, 2), juce::Justification::centred);
}

void CandleLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& btn,
                                           bool over, bool)
{
    const bool  on = btn.getToggleState();
    const auto  r  = btn.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (on ? juce::Colour (0x423c220d) : CandleColours::btnOff.withAlpha (0.55f));
    g.fillRoundedRectangle (r, 3.0f);
    if (on || over)
    {
        g.setColour (CandleColours::amber.withAlpha (on ? 0.18f : 0.11f));
        g.fillRoundedRectangle (r, 3.0f);
    }
    g.setColour (on ? CandleColours::amber.withAlpha (0.70f) : CandleColours::panelRule.withAlpha (0.45f));
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
    g.setFont (getGeorgiaFont (10.0f, juce::Font::bold));
    g.setColour (on ? CandleColours::creamBright : CandleColours::amberLo);
    g.drawText (btn.getButtonText(), btn.getLocalBounds().reduced (4, 1), juce::Justification::centred);
}

// ── Combo box ────────────────────────────────────────────────────────────────
void CandleLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool,
                                       int ax, int ay, int aw, int ah, juce::ComboBox&)
{
    const auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h);
    g.setColour (CandleColours::btnOff.withAlpha (0.65f));
    g.fillRoundedRectangle (r.reduced (0.5f), 3.0f);
    g.setColour (CandleColours::panelBorder.withAlpha (0.45f));
    g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);

    juce::Path arrow;
    const float acx = ax + aw * 0.5f;
    const float acy = ay + ah * 0.38f;
    const float asz = aw * 0.35f;
    arrow.startNewSubPath (acx - asz, acy);
    arrow.lineTo (acx, acy + asz * 0.75f);
    arrow.lineTo (acx + asz, acy);
    g.setColour (CandleColours::amber);
    g.strokePath (arrow, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
}

void CandleLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (8, 1, box.getWidth() - 32, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

// ── Popup menu ───────────────────────────────────────────────────────────────

juce::PopupMenu::Options CandleLookAndFeel::getOptionsForComboBoxPopupMenu (juce::ComboBox& box,
                                                                        juce::Label& label)
{
    return juce::LookAndFeel_V4::getOptionsForComboBoxPopupMenu (box, label)
        .withParentComponent (box.getTopLevelComponent());
}

void CandleLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.setColour (CandleColours::bg);
    g.fillRect (0, 0, w, h);
    g.setColour (CandleColours::panelBorder.withAlpha (0.45f));
    g.drawRect (0, 0, w, h, 1);
}

void CandleLookAndFeel::drawPopupMenuItem (juce::Graphics& g,
                                            const juce::Rectangle<int>& area,
                                            bool isSeparator, bool isActive, bool isHighlighted,
                                            bool isTicked, bool,
                                            const juce::String& text, const juce::String&,
                                            const juce::Drawable*, const juce::Colour*)
{
    if (isSeparator) { g.setColour (CandleColours::panelBorder.withAlpha (0.35f));
                       g.fillRect (area.getX() + 6, area.getCentreY(), area.getWidth() - 12, 1); return; }
    if (isHighlighted) { g.setColour (juce::Colour (0xff2b2018)); g.fillRect (area); }
    g.setFont (getPopupMenuFont());
    g.setColour (isActive ? CandleColours::amberLo : CandleColours::creamDim);
    g.drawText (text, area.getX() + 12, area.getY(), area.getWidth() - 16, area.getHeight(),
                juce::Justification::centredLeft);
    if (isTicked) { g.setColour (CandleColours::amber);
                    g.drawText ("*", area.getX() + 3, area.getY(), 10, area.getHeight(), juce::Justification::centred); }
}

juce::Font CandleLookAndFeel::getLabelFont (juce::Label&)     { return getGeorgiaFont (11.0f); }
juce::Font CandleLookAndFeel::getComboBoxFont (juce::ComboBox& box)
{
    const float fitByHeight = (float) box.getHeight() * 0.46f;
    return getGeorgiaFont (juce::jlimit (7.0f, 13.0f, fitByHeight));
}
juce::Font CandleLookAndFeel::getPopupMenuFont()               { return getGeorgiaFont (11.5f); }
