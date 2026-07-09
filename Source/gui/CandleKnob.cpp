#include "CandleKnob.h"

CandleKnob::CandleKnob (const juce::String& label,
                         juce::AudioProcessorValueTreeState& state,
                         const juce::String& paramID,
                         bool isOnPanel)
    : labelText (label), onPanel (isOnPanel)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    slider.setPopupDisplayEnabled (true, true, nullptr);
    addAndMakeVisible (slider);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        state, paramID, slider);

    if (auto* param = state.getParameter (paramID))
        slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
}

void CandleKnob::resized()
{
    const int labelH = 14;
    slider.setBounds (0, 0, getWidth(), getHeight() - labelH);
}

void CandleKnob::paint (juce::Graphics& g)
{
    const int labelH = 14;
    const auto labelR = juce::Rectangle<int> (0, getHeight() - labelH, getWidth(), labelH);
    g.setFont (juce::Font (juce::FontOptions().withName ("Georgia")
                                              .withStyle ("Bold")
                                              .withHeight (9.5f)));
    g.setColour (CandleColours::amberLo);
    g.drawText (labelText, labelR, juce::Justification::centred);
}
