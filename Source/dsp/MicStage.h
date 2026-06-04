#pragma once
#include <JuceHeader.h>

// Emulates the cheap RadioShack "Realistic" dynamic mic that Elliott Smith
// used for vocals on Roman Candle (non-detachable cord, on/off switch, ¼" plug).
//
// Modelled characteristics (based on comparable late-80s/early-90s Realistic
// budget dynamics):
//   • 2nd-order HPF at 120 Hz  — thin, papery low end
//   • Presence peak at 3.8 kHz, +6 dB, Q=0.85 — cheap capsule resonance
//   • 2nd-order LPF at 9 kHz  — steep HF roll-off
//   • High self-noise at ~–38 dBFS — spectrally white-ish (electret capsule)
//   • Input impedance mismatch colouring (gentle HF load from cable capacitance)
class MicStage
{
public:
    struct Parameters { bool enabled = false; };

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, const Parameters& p);

private:
    double sampleRate = 44100.0;

    juce::dsp::IIR::Filter<float> hpf[2];        // 120 Hz 2nd-order HPF
    juce::dsp::IIR::Filter<float> presence[2];   // 3.8 kHz presence peak
    juce::dsp::IIR::Filter<float> lpf[2];        // 9 kHz 2nd-order LPF
    juce::dsp::IIR::Filter<float> cableLP[2];    // cable-capacitance HF roll (14 kHz)

    // Noise shaping filters (one-pole, inline state)
    float noiseHP_L = 0.0f, noiseHP_R = 0.0f;
    float noiseLP_L = 0.0f, noiseLP_R = 0.0f;

    juce::Random random;
};
