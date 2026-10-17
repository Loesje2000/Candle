#include "MicStage.h"

namespace
{
    // Installs the filter coefficient set for one mic model into the shared
    // filter banks (swapped on model change; IIR state carries over cleanly).
    void setModelCoefficients (double sr,
                               juce::dsp::IIR::Filter<float>* hpf,
                               juce::dsp::IIR::Filter<float>* presence,
                               juce::dsp::IIR::Filter<float>* lpf,
                               juce::dsp::IIR::Filter<float>* cableLP,
                               int model)
    {
        if (model == 1)
        {
            // 57: gentle bass shelf, broad 8.2 kHz edge, steep top end
            const auto lowShelf = juce::dsp::IIR::Coefficients<float>::makeLowShelf (
                sr, 160.0, 0.707, juce::Decibels::decibelsToGain (-3.0f));
            const auto peak = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
                sr, 8200.0, 0.9f, juce::Decibels::decibelsToGain (5.5f));
            const auto lpfC = juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, 12000.0, 0.707);
            const auto cabC = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass (sr, 16000.0);
            for (int ch = 0; ch < 2; ++ch)
            {
                *hpf[ch].coefficients      = *lowShelf;
                *presence[ch].coefficients = *peak;
                *lpf[ch].coefficients      = *lpfC;
                *cableLP[ch].coefficients  = *cabC;
            }
        }
        else
        {
            // Realistic: 120 Hz HPF, narrow 3.2 kHz honk, 6.5 kHz cliff
            const auto hpfC = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, 120.0, 0.707);
            const auto peak = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
                sr, 3200.0, 1.4f, juce::Decibels::decibelsToGain (4.0f));
            const auto lpfC = juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, 6500.0, 0.707);
            const auto cabC = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass (sr, 10000.0);
            for (int ch = 0; ch < 2; ++ch)
            {
                *hpf[ch].coefficients      = *hpfC;
                *presence[ch].coefficients = *peak;
                *lpf[ch].coefficients      = *lpfC;
                *cableLP[ch].coefficients  = *cabC;
            }
        }
    }
}

void MicStage::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    for (int ch = 0; ch < 2; ++ch)
    {
        hpf[ch].prepare (spec);
        presence[ch].prepare (spec);
        lpf[ch].prepare (spec);
        cableLP[ch].prepare (spec);
    }
    lastModel = -1;  // force coefficient install on the first processed block
    reset();
    prepared = true;
}

void MicStage::reset()
{
    for (int ch = 0; ch < 2; ++ch)
    {
        hpf[ch].reset();
        presence[ch].reset();
        lpf[ch].reset();
        cableLP[ch].reset();
    }
    noiseHP_L = noiseHP_R = noiseLP_L = noiseLP_R = 0.0f;
}

void MicStage::process (juce::AudioBuffer<float>& buffer, const Parameters& p)
{
    if (! p.enabled || ! prepared) return;

    // Swap filter banks when the mic model changes (0 = Realistic, 1 = 57)
    const int model = juce::jlimit (0, 1, p.model);
    if (model != lastModel)
    {
        setModelCoefficients (sampleRate, hpf, presence, lpf, cableLP, model);
        lastModel = model;
    }

    const int   numSamples = buffer.getNumSamples();
    const float sr         = static_cast<float> (sampleRate);
    const float twoPi      = juce::MathConstants<float>::twoPi;

    // One-pole noise shaping filter coefficients
    const float noiseHPCoeff = (twoPi * 180.0f / sr) / (1.0f + twoPi * 180.0f / sr);
    const float noiseLPCoeff = (twoPi * 11000.0f / sr) / (1.0f + twoPi * 11000.0f / sr);

    // Mic self-noise: the frequency shaping is the primary character of this stage.
    // The noise is kept subtle (~-57 dBFS fixed floor) so it only surfaces on very
    // quiet passages — exactly how a cheap mic behaves in a real recording context.
    // The cassette stage already supplies the main hiss floor; this just adds a
    // whisper of extra grain on top.
    // Realistic: cheap capsule/preamp grain (~-57 dBFS).
    // 57: professional capsule, far quieter (~-74 dBFS).
    const float kNoiseLevel = (model == 1) ? 0.0002f : 0.0014f;

    // Block RMS for envelope tracking (used to keep noise at a stable absolute level)
    float blockRmsSum = 0.0f;
    const auto* rp = buffer.getReadPointer (0);
    for (int i = 0; i < numSamples; ++i) blockRmsSum += rp[i] * rp[i];
    const float blockRms = std::sqrt (blockRmsSum / juce::jmax (1, numSamples) + 1.0e-12f);
    (void) blockRms; // available for future signal-dependent noise if needed

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getWritePointer (1);

    for (int i = 0; i < numSamples; ++i)
    {
        // ── Frequency response shaping ────────────────────────────────────
        float sL = hpf[0].processSample (L[i]);
        float sR = hpf[1].processSample (R[i]);

        sL = presence[0].processSample (sL);
        sR = presence[1].processSample (sR);

        sL = lpf[0].processSample (sL);
        sR = lpf[1].processSample (sR);

        sL = cableLP[0].processSample (sL);
        sR = cableLP[1].processSample (sR);

        // ── Self-noise (bandpass-shaped, independent per channel) ─────────
        const float rawL = random.nextFloat() * 2.0f - 1.0f;
        const float rawR = random.nextFloat() * 2.0f - 1.0f;

        noiseLP_L += noiseLPCoeff * (rawL - noiseLP_L);
        noiseLP_R += noiseLPCoeff * (rawR - noiseLP_R);

        const float nL = noiseLP_L - noiseHP_L;
        const float nR = noiseLP_R - noiseHP_R;
        noiseHP_L += noiseHPCoeff * (noiseLP_L - noiseHP_L);
        noiseHP_R += noiseHPCoeff * (noiseLP_R - noiseHP_R);

        L[i] = sL + nL * kNoiseLevel;
        R[i] = sR + nR * kNoiseLevel;
    }
}
