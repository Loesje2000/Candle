#pragma once
#include <JuceHeader.h>
#include "CandleLookAndFeel.h"

// Labelled knob component for the Candle plugin.
// Draws a small label below the slider; the slider itself is rendered
// by CandleLookAndFeel::drawRotarySlider.
class CandleKnob : public juce::Component
{
public:
    CandleKnob (const juce::String& labelText,
                juce::AudioProcessorValueTreeState& state,
                const juce::String& paramID,
                bool onPanel = true);  // onPanel: true = dark ink labels, false = cream

    void resized() override;
    void paint (juce::Graphics&) override;

    void setTooltip (const juce::String& tip) { slider.setTooltip (tip); }

private:
    juce::Slider slider;
    juce::String labelText;
    bool onPanel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};
