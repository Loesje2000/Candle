#pragma once
#include <JuceHeader.h>

namespace CandleColours
{
    // ── BuckleyHaze chassis palette ────────────────────────────────────────
    const juce::Colour bg     { 0xff1a0f08 };
    const juce::Colour bgDeep { 0xff0e0703 };

    const juce::Colour panel       { 0xff1a0e07 };
    const juce::Colour panelHi     { 0xff211008 };
    const juce::Colour panelLo     { 0xff0e0703 };
    const juce::Colour panelBorder { 0xff78502d };
    const juce::Colour panelRule   { 0xffb4824b };

    // ── Text ─────────────────────────────────────────────────────────────────
    const juce::Colour cream       { 0xfff3ecd0 };
    const juce::Colour creamDim    { 0x8cc89664 };
    const juce::Colour creamBright { 0xffffdca0 };

    // ── Amber brand accent ────────────────────────────────────────────────────
    const juce::Colour amber       { 0xffe8b66a };
    const juce::Colour amberHi     { 0xfff3c878 };
    const juce::Colour amberLo     { 0xffc89860 };
    const juce::Colour amberGlow   { 0x42e8b66a };

    // ── Knob ─────────────────────────────────────────────────────────────────
    const juce::Colour knobBody    { 0xff1d0e08 };
    const juce::Colour knobBodyHi  { 0xff3a241a };
    const juce::Colour knobDot     { 0xfff3c878 };
    const juce::Colour knobTrack   { 0xff78502d };

    // ── Rubblesonic badge (BuckleyHaze sage) ────────────────────────────────
    const juce::Colour badgeHi     { 0xff7d8d72 };
    const juce::Colour badgeLo     { 0xff56644f };
    const juce::Colour badgeText   { 0xfff3ecd0 };

    // ── Toggle / button ───────────────────────────────────────────────────────
    const juce::Colour btnOff      { 0xff140b06 };
    const juce::Colour btnOffBd    { 0xff78502d };

    // ── LED / indicator ───────────────────────────────────────────────────────
    const juce::Colour ledOff      { 0xff2A1A08 };
    const juce::Colour ledOn       { 0xffFFE060 };

    // ── VU meter face ─────────────────────────────────────────────────────────
    const juce::Colour vuBezel     { 0xff1A1410 };
    const juce::Colour vuFace      { 0xffE8DDB8 };
    const juce::Colour vuNeedle    { 0xff1A1408 };
    const juce::Colour vuRed       { 0xffC83018 };

    // ── Segmented level meter ─────────────────────────────────────────────────
    const juce::Colour meterGreen  { 0xff60B840 };
    const juce::Colour meterAmber  { 0xffD4922A };
    const juce::Colour meterRed    { 0xffCC3820 };
    const juce::Colour meterOff    { 0xff181208 };
}

enum class KnobStyle { Arc, Stepped };

class CandleLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CandleLookAndFeel();
    ~CandleLookAndFeel() override = default;

    juce::Font getGeorgiaFont (float size, int style = juce::Font::plain) const;
    juce::Font getMonoFont    (float size) const;

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour&, bool over, bool down) override;
    void drawButtonText        (juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawToggleButton      (juce::Graphics&, juce::ToggleButton&, bool, bool) override;

    void drawComboBox         (juce::Graphics&, int w, int h, bool,
                               int ax, int ay, int aw, int ah, juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::PopupMenu::Options getOptionsForComboBoxPopupMenu (juce::ComboBox&, juce::Label&) override;

    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
    void drawPopupMenuItem       (juce::Graphics&, const juce::Rectangle<int>&,
                                  bool isSep, bool isActive, bool isHighlighted,
                                  bool isTicked, bool hasSubMenu,
                                  const juce::String&, const juce::String&,
                                  const juce::Drawable*, const juce::Colour*) override;
    int  getMenuWindowFlags() override { return 0; }

    juce::Font getLabelFont    (juce::Label&)   override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont()               override;
};
