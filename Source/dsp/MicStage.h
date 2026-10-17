#pragma once
#include <JuceHeader.h>

// Emulates the two microphones used on Roman Candle-era recordings:
//
//   model 0 - REALISTIC: the cheap RadioShack "Realistic" dynamic mic Elliott
//     Smith used for vocals (non-detachable cord, on/off switch, 1/4" plug).
//     Generic budget-dynamic estimates, NOT measured from a real unit:
//       - 2nd-order HPF at 120 Hz: thin, papery low end
//       - Presence peak at 3.2 kHz, +4 dB, Q=1.4: narrow cheap-capsule honk
//       - 2nd-order LPF at 6.5 kHz: cheap dynamics fall off a cliff above ~5-6 kHz
//       - 1st-order LPF at 10 kHz: extra top-end dulling (cable/capsule load)
//       - Higher self-noise (~-57 dBFS): cheap capsule/preamp grain
//
//   model 1 - 57: the classic industry-standard dynamic also used on Roman
//     Candle sessions. Named simply "57" in the UI for legal reasons.
//     Modelled from the widely published response curve of that mic (an
//     approximation, not a measurement of a specific unit):
//       - Gentle low shelf, -3 dB below ~160 Hz
//       - Broad presence bump, +5.5 dB at 8.2 kHz, Q=0.9
//       - 2nd-order LPF at 12 kHz + 1st-order at 16 kHz: steep top-end fall
//       - Far lower self-noise than the Realistic (professional capsule)
class MicStage
{
public:
    // model: 0 = Realistic, 1 = 57
    struct Parameters { bool enabled = false; int model = 0; };

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, const Parameters& p);

private:
    double sampleRate = 44100.0;

    int lastModel = -1;   // coefficients are swapped when the model changes

    juce::dsp::IIR::Filter<float> hpf[2];        // 120 Hz HPF (Realistic) / low shelf (57)
    juce::dsp::IIR::Filter<float> presence[2];   // 3.2 kHz peak (Realistic) / 8.2 kHz peak (57)
    juce::dsp::IIR::Filter<float> lpf[2];        // 6.5 kHz LPF (Realistic) / 12 kHz LPF (57)
    juce::dsp::IIR::Filter<float> cableLP[2];    // 10 kHz LPF (Realistic) / 16 kHz LPF (57)

    // Noise shaping filters (one-pole, inline state)
    float noiseHP_L = 0.0f, noiseHP_R = 0.0f;
    float noiseLP_L = 0.0f, noiseLP_R = 0.0f;

    juce::Random random;
    bool prepared = false;
};
