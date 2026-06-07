#pragma once
#include <JuceHeader.h>
#include <vector>

// Full signal chain for the Tascam Portastudio 424 cassette emulation.
// Signal path: trim → mixer EQ → DBX encode → saturation → speed EQ
//              → wow/flutter → noise → crosstalk → head bump → dropout
//              → DBX decode → generation loss
class CassetteTape
{
public:
    struct Parameters
    {
        float saturation = 0.4f;   // 0–1
        float noise      = 0.5f;   // 0–1
        float wowFlutter = 0.3f;   // 0–1
        float age        = 0.2f;   // 0–1, head wear
        int   speed      = 1;      // 0=HIGH (9.5cm/s), 1=NORM (4.8cm/s), 2=SLOW (2.4cm/s)
        bool  dbxOn      = true;
        int   generations = 0;     // 0–4 bounce generations
        float eqHigh     = 0.0f;   // dB, ±10, shelf at 10 kHz
        float eqLow      = 0.0f;   // dB, ±10, shelf at 100 Hz
        float trim       = 0.0f;   // dB, ±20
    };

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, const Parameters& p);

    float getVULevel() const noexcept { return vuLevel.load(); }

private:
    void updateFilters (const Parameters& p);
    void resetState();
    bool sanitizeSample (float& left, float& right) noexcept;

    static float saturate (float x, float drive) noexcept;
    float readDelay (const std::vector<float>& buf, int writePos, float delaySamples) const noexcept;

    double sampleRate = 44100.0;

    // Mixer EQ (pre-tape)
    juce::dsp::IIR::Filter<float> eqHighFilter[2], eqLowFilter[2];

    // Speed EQ: LPF + HPF for tape bandwidth
    juce::dsp::IIR::Filter<float> speedLPF[2], speedHPF[2];

    // Generation loss LPF (cascaded per generation)
    juce::dsp::IIR::Filter<float> genLPF[2];

    // Head bump (playback inductance boost ~80 Hz)
    juce::dsp::IIR::Filter<float> headBump[2];

    // DBX RMS envelope detectors: encode (tracks input) and decode (tracks tape output)
    float dbxEncRmsL = 0.0f, dbxEncRmsR = 0.0f;
    float dbxDecRmsL = 0.0f, dbxDecRmsR = 0.0f;

    // Wow & flutter delay line
    static constexpr int kDelaySize = 4096;
    std::vector<float> delayBufL, delayBufR;
    int delayWritePos = 0;

    // Wow/flutter oscillator phases
    float wowPhase1 = 0.0f, wowPhase2 = 0.0f, wowPhase3 = 0.0f;
    float flutterPhase1 = 0.0f, flutterPhase2 = 0.0f;
    float wowRateDrift = 0.0f;  // slow random walk on wow oscillator rate

    // Noise shaping filter states (single-pole)
    float noiseLPL = 0.0f, noiseLPR = 0.0f;
    float noiseHPL = 0.0f, noiseHPR = 0.0f;

    // Crosstalk LP filter state
    float crosstalkLPL = 0.0f, crosstalkLPR = 0.0f;

    // DC blocking
    float dcBlockL = 0.0f, dcBlockR = 0.0f;

    // Head wear HF loss filter state
    float ageLPL = 0.0f, ageLPR = 0.0f;

    // Dropout state
    float dropoutTimer = 0.0f;
    float dropoutGain  = 1.0f;
    float dropoutGainSmooth = 1.0f;
    bool  dropoutActive = false;

    // VU meter (post-processing RMS, ~300ms ballistic)
    float vuSmooth = 0.0f;
    std::atomic<float> vuLevel { 0.0f };

    // Smoothed parameter values
    juce::SmoothedValue<float> satSmoothed, noiseSmoothed, wowSmoothed;

    int   lastSpeed = -1;
    int   lastGenerations = -1;
    bool  lastDbx = true;
    float lastEqHigh = 0.0f, lastEqLow = 0.0f;

    juce::Random random;
    bool prepared = false;

    // IEC Type I ferric record/playback equalization
    juce::dsp::IIR::Filter<float> iecRecordEQ[2];    // pre-saturation high-shelf boost
    juce::dsp::IIR::Filter<float> iecPlaybackEQ[2];  // post-tape complementary cut

    // AC hum oscillator (60 Hz + 120 Hz harmonic)
    float humPhase = 0.0f;

    // Print-through: ~0.7 s post-echo of tape signal at very low level
    std::vector<float> ptBufL, ptBufR;
    int ptWritePos   = 0;
    int ptDelSamples = 0;

    // Room early reflections (small bedroom, 3 sparse taps)
    static constexpr int kRoomSize = 8192;
    std::vector<float> roomBufL, roomBufR;
    int roomWritePos = 0;
    int roomTap1 = 0, roomTap2 = 0, roomTap3 = 0;
};
