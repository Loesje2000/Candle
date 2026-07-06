#include "MicStage.h"

void MicStage::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    // 2nd-order HPF at 120 Hz (Butterworth Q = 0.707)
    auto hpfCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, 120.0, 0.707);
    // Presence peak: +6 dB at 3.8 kHz, Q = 0.85
    auto presCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
        sampleRate, 3800.0, 0.85f, juce::Decibels::decibelsToGain (6.0f));
    // 2nd-order LPF at 9 kHz (Butterworth Q = 0.707)
    auto lpfCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, 9000.0, 0.707);
    // Cable capacitance: gentle 1st-order LPF at 14 kHz
    auto cableCoeffs = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass (sampleRate, 14000.0);

    for (int ch = 0; ch < 2; ++ch)
    {
        hpf[ch].prepare (spec);     *hpf[ch].coefficients     = *hpfCoeffs;
        presence[ch].prepare (spec); *presence[ch].coefficients = *presCoeffs;
        lpf[ch].prepare (spec);     *lpf[ch].coefficients     = *lpfCoeffs;
        cableLP[ch].prepare (spec); *cableLP[ch].coefficients = *cableCoeffs;
    }
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
    constexpr float kNoiseLevel = 0.0014f; // ~–57 dBFS

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
