#pragma once

#include <JuceHeader.h>
#include "dsp/CassetteTape.h"
#include "dsp/MicStage.h"

class CandleAudioProcessor final : public juce::AudioProcessor
{
public:
    using APVTS = juce::AudioProcessorValueTreeState;

    CandleAudioProcessor();
    ~CandleAudioProcessor() override = default;

    void prepareToPlay  (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    APVTS apvts;
    static APVTS::ParameterLayout createParameterLayout();

    void applyFactoryPreset (int index);
    juce::StringArray getPresetNames() const;

    juce::File getUserPresetDirectory() const;
    juce::StringArray getUserPresetNames() const;
    bool saveUserPreset (const juce::String& name);
    bool loadUserPresetByName (const juce::String& name);
    bool deleteUserPreset (const juce::String& name);
    bool renameUserPreset (const juce::String& oldName, const juce::String& newName);

    float getVULevel()     const noexcept { return tape.getVULevel(); }
    float getInputMeterDb()  const noexcept { return inputMeterDb.load(); }
    float getOutputMeterDb() const noexcept { return outputMeterDb.load(); }

private:
    bool sanitizeAudio (juce::AudioBuffer<float>& buf) noexcept;
    float getStereoRms (const juce::AudioBuffer<float>& buf) const noexcept;

    MicStage     mic;
    CassetteTape tape;
    juce::dsp::Oversampling<float> oversampling
        { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true };
    juce::SmoothedValue<float> outputGainSmooth;
    juce::AudioBuffer<float> drySafetyBuffer;
    bool wasBypassed = false;

    std::atomic<float> inputMeterDb  { -100.0f };
    std::atomic<float> outputMeterDb { -100.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CandleAudioProcessor)
};
