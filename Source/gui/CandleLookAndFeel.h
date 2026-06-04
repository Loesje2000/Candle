#pragma once
#include <JuceHeader.h>

namespace CandleColours
{
    // ── Chassis ──────────────────────────────────────────────────────────────
    const juce::Colour bg     { 0xff141210 };   // very dark warm black
    const juce::Colour bgDeep { 0xff0C0A08 };

    // ── Panel sections (dark warm gray — not cream) ───────────────────────
    const juce::Colour panel       { 0xff1E1A16 };
    const juce::Colour panelHi     { 0xff28221C };
    const juce::Colour panelLo     { 0xff161210 };
    const juce::Colour panelBorder { 0xff3C3028 };
    const juce::Colour panelRule   { 0xff4E4030 };

    // ── Text ─────────────────────────────────────────────────────────────────
    const juce::Colour cream       { 0xffEEE4C4 };
    const juce::Colour creamDim    { 0xff7A6E54 };
    const juce::Colour creamBright { 0xffFAF2DC };

    // ── Amber brand accent ────────────────────────────────────────────────────
    const juce::Colour amber       { 0xffC8882A };
    const juce::Colour amberHi     { 0xffFFD070 };
    const juce::Colour amberLo     { 0xffA06618 };
    const juce::Colour amberGlow   { 0x44C8882A };

    // ── Knob ─────────────────────────────────────────────────────────────────
    const juce::Colour knobBody    { 0xff242018 };
    const juce::Colour knobBodyHi  { 0xff383028 };
    const juce::Colour knobDot     { 0xffC8882A };
    const juce::Colour knobTrack   { 0xff181410 };

    // ── Rubblesonic badge (sage green — matches other RS plugins) ─────────────
    const juce::Colour badgeHi     { 0xff6F8068 };
    const juce::Colour badgeLo     { 0xff4D5C48 };
    const juce::Colour badgeText   { 0xffECE4CA };

    // ── Toggle / button ───────────────────────────────────────────────────────
    const juce::Colour btnOff      { 0xff1C1814 };
    const juce::Colour btnOffBd    { 0xff3C3028 };

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
