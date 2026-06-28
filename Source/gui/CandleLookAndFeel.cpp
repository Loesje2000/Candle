#include "CandleLookAndFeel.h"

CandleLookAndFeel::CandleLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId,   CandleColours::btnOff);
    setColour (juce::ComboBox::outlineColourId,      CandleColours::panelBorder);
    setColour (juce::ComboBox::textColourId,         CandleColours::cream);
    setColour (juce::ComboBox::arrowColourId,        CandleColours::amber);
    setColour (juce::PopupMenu::backgroundColourId,              CandleColours::bg);
    setColour (juce::PopupMenu::textColourId,                    CandleColours::cream);
    setColour (juce::PopupMenu::highlightedBackgroundColourId,   CandleColours::amber.withAlpha (0.30f));
    setColour (juce::PopupMenu::highlightedTextColourId,         CandleColours::creamBright);
}

juce::Font CandleLookAndFeel::getGeorgiaFont (float size, int style) const
{
    return juce::Font (juce::FontOptions()
        .withName ("Georgia")
        .withStyle ((style & juce::Font::bold)   ? "Bold"
                  : (style & juce::Font::italic) ? "Italic" : "Regular")
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
    const float cx = x + w * 0.5f;
    const float cy = y + h * 0.5f;
    const float rOuter = juce::jmin (w, h) * 0.43f;
    const float rInner = rOuter * 0.63f;
    const float trackR = rOuter + 3.0f;

    // Track groove
    juce::Path track;
    track.addCentredArc (cx, cy, trackR, trackR, 0.0f, startAngle, endAngle, true);
    g.setColour (CandleColours::knobTrack.withAlpha (0.7f));
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));

    // Amber fill arc
    const float curAngle = startAngle + sliderPos * (endAngle - startAngle);
    juce::Path fill;
    fill.addCentredArc (cx, cy, trackR, trackR, 0.0f, startAngle, curAngle, true);
    g.setColour (CandleColours::amber.withAlpha (0.80f));
    g.strokePath (fill, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    // Outer ring (gradient)
    juce::ColourGradient outer (CandleColours::knobBodyHi, cx - rOuter * 0.5f, cy - rOuter * 0.5f,
                                CandleColours::bgDeep,     cx + rOuter * 0.5f, cy + rOuter * 0.5f, true);
    g.setGradientFill (outer);
    g.fillEllipse (cx - rOuter, cy - rOuter, rOuter * 2.0f, rOuter * 2.0f);

    // Inner cap
    juce::ColourGradient cap (CandleColours::knobBodyHi, cx, cy - rInner,
                              CandleColours::bgDeep,     cx, cy + rInner, false);
    g.setGradientFill (cap);
    g.fillEllipse (cx - rInner, cy - rInner, rInner * 2.0f, rInner * 2.0f);

    // Specular rim
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawEllipse (cx - rInner, cy - rInner, rInner * 2.0f, rInner * 2.0f, 0.8f);

    // Amber indicator dot
    const float dotR  = rInner * 0.17f;
    const float dotD  = rInner * 0.67f;
    const float dotX  = cx + dotD * std::sin (curAngle);
    const float dotY  = cy - dotD * std::cos (curAngle);
    g.setColour (CandleColours::amberHi);
    g.fillEllipse (dotX - dotR, dotY - dotR, dotR * 2.0f, dotR * 2.0f);
    g.setColour (CandleColours::amber.withAlpha (0.45f));
    g.fillEllipse (dotX - dotR * 1.7f, dotY - dotR * 1.7f, dotR * 3.4f, dotR * 3.4f);
}

// ── Buttons ──────────────────────────────────────────────────────────────────
void CandleLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& btn,
                                               const juce::Colour&, bool over, bool down)
{
    const auto  r  = btn.getLocalBounds().toFloat().reduced (0.5f);
    const bool  on = btn.getToggleState();
    juce::Colour fill = on  ? CandleColours::amber.withAlpha (down ? 1.0f : 0.88f)
                            : CandleColours::btnOff.withAlpha (over ? 0.9f : 0.75f);
    g.setColour (fill);
    g.fillRoundedRectangle (r, 3.5f);
    g.setColour (on ? CandleColours::amberLo : CandleColours::panelBorder.withAlpha (0.7f));
    g.drawRoundedRectangle (r, 3.5f, 1.0f);
}

void CandleLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& btn, bool, bool)
{
    g.setFont (getGeorgiaFont (10.5f, juce::Font::bold));
    g.setColour (btn.getToggleState() ? CandleColours::bg : CandleColours::cream);
    g.drawText (btn.getButtonText(), btn.getLocalBounds(), juce::Justification::centred);
}

void CandleLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& btn,
                                           bool over, bool)
{
    const bool  on = btn.getToggleState();
    const auto  r  = btn.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (on ? CandleColours::amber : CandleColours::btnOff.withAlpha (over ? 0.85f : 0.7f));
    g.fillRoundedRectangle (r, r.getHeight() * 0.42f);
    g.setColour (on ? CandleColours::amberLo : CandleColours::panelBorder.withAlpha (0.6f));
    g.drawRoundedRectangle (r, r.getHeight() * 0.42f, 1.0f);
    g.setFont (getGeorgiaFont (10.0f, juce::Font::bold));
    g.setColour (on ? CandleColours::bg : CandleColours::cream);
    g.drawText (btn.getButtonText(), btn.getLocalBounds(), juce::Justification::centred);
}

// ── Combo box ────────────────────────────────────────────────────────────────
void CandleLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool,
                                       int ax, int ay, int aw, int ah, juce::ComboBox&)
{
    const auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h);
    g.setColour (CandleColours::btnOff);
    g.fillRoundedRectangle (r, 3.5f);
    g.setColour (CandleColours::panelBorder.withAlpha (0.8f));
    g.drawRoundedRectangle (r.reduced (0.5f), 3.5f, 1.0f);

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
    label.setBounds (6, 1, box.getWidth() - 26, box.getHeight() - 2);
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
    g.setColour (CandleColours::panelBorder.withAlpha (0.6f));
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
    if (isHighlighted) { g.setColour (CandleColours::amber.withAlpha (0.22f)); g.fillRect (area); }
    g.setFont (getPopupMenuFont());
    g.setColour (isActive ? CandleColours::cream : CandleColours::creamDim);
    g.drawText (text, area.getX() + 12, area.getY(), area.getWidth() - 16, area.getHeight(),
                juce::Justification::centredLeft);
    if (isTicked) { g.setColour (CandleColours::amber);
                    g.drawText ("*", area.getX() + 3, area.getY(), 10, area.getHeight(), juce::Justification::centred); }
}

juce::Font CandleLookAndFeel::getLabelFont (juce::Label&)     { return getGeorgiaFont (10.5f); }
juce::Font CandleLookAndFeel::getComboBoxFont (juce::ComboBox&) { return getGeorgiaFont (10.5f); }
juce::Font CandleLookAndFeel::getPopupMenuFont()               { return getGeorgiaFont (11.5f); }
