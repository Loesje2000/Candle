#include "CassetteTape.h"

void CassetteTape::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    for (int ch = 0; ch < 2; ++ch)
    {
        eqHighFilter[ch].prepare (spec);
        eqLowFilter[ch].prepare  (spec);
        speedLPF[ch].prepare     (spec);
        speedHPF[ch].prepare     (spec);
        genLPF[ch].prepare       (spec);
        headBump[ch].prepare     (spec);
        iecRecordEQ[ch].prepare  (spec);
        iecPlaybackEQ[ch].prepare(spec);
    }

    // IEC Type I ferric: +6 dB record shelf / -6 dB playback shelf at 1326 Hz (120 µs TC)
    const auto recCoeffs  = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        sampleRate, 1326.0, 0.71, juce::Decibels::decibelsToGain ( 6.0f));
    const auto playCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        sampleRate, 1326.0, 0.71, juce::Decibels::decibelsToGain (-6.0f));
    for (int ch = 0; ch < 2; ++ch)
    {
        *iecRecordEQ[ch].coefficients   = *recCoeffs;
        *iecPlaybackEQ[ch].coefficients = *playCoeffs;
    }

    // Print-through: 0.7 s post-echo buffer
    ptDelSamples = static_cast<int> (0.70 * sampleRate);
    ptBufL.assign (ptDelSamples + 512, 0.0f);
    ptBufR.assign (ptDelSamples + 512, 0.0f);
    ptWritePos = 0;

    // Room early-reflection taps (small bedroom: ~8, 14, 21 ms)
    roomTap1 = static_cast<int> (0.008 * sampleRate);
    roomTap2 = static_cast<int> (0.014 * sampleRate);
    roomTap3 = static_cast<int> (0.021 * sampleRate);
    roomBufL.assign (kRoomSize, 0.0f);
    roomBufR.assign (kRoomSize, 0.0f);
    roomWritePos = 0;

    satSmoothed.reset   (sampleRate, 0.03);
    noiseSmoothed.reset (sampleRate, 0.05);
    wowSmoothed.reset   (sampleRate, 0.03);
    satSmoothed.setCurrentAndTargetValue   (0.4f);
    noiseSmoothed.setCurrentAndTargetValue (0.5f);
    wowSmoothed.setCurrentAndTargetValue   (0.3f);
    resetState();
}

void CassetteTape::reset() { resetState(); }

void CassetteTape::resetState()
{
    delayBufL.assign (kDelaySize, 0.0f);
    delayBufR.assign (kDelaySize, 0.0f);
    delayWritePos = 0;
    wowPhase1 = wowPhase2 = wowPhase3 = 0.0f;
    flutterPhase1 = flutterPhase2 = 0.0f;
    wowRateDrift = 0.0f;
    noiseLPL = noiseLPR = noiseHPL = noiseHPR = 0.0f;
    crosstalkLPL = crosstalkLPR = 0.0f;
    dcBlockL = dcBlockR = 0.0f;
    // Start both trackers at reference power so encode/decode open at unity gain
    dbxEncRmsL = dbxEncRmsR = 0.10f;
    dbxDecRmsL = dbxDecRmsR = 0.10f;
    dropoutTimer = 0.0f;
    dropoutGain  = 1.0f;
    dropoutGainSmooth = 1.0f;
    dropoutActive = false;
    vuSmooth = 0.0f;
    vuLevel.store (0.0f);
    lastSpeed = -1;
    lastGenerations = -1;
    humPhase = 0.0f;
    std::fill (ptBufL.begin(),   ptBufL.end(),   0.0f);
    std::fill (ptBufR.begin(),   ptBufR.end(),   0.0f);
    ptWritePos = 0;
    std::fill (roomBufL.begin(), roomBufL.end(), 0.0f);
    std::fill (roomBufR.begin(), roomBufR.end(), 0.0f);
    roomWritePos = 0;
    for (int ch = 0; ch < 2; ++ch)
    {
        eqHighFilter[ch].reset();
        eqLowFilter[ch].reset();
        speedLPF[ch].reset();
        speedHPF[ch].reset();
        genLPF[ch].reset();
        headBump[ch].reset();
        iecRecordEQ[ch].reset();
        iecPlaybackEQ[ch].reset();
    }
}

void CassetteTape::updateFilters (const Parameters& p)
{
    const bool needUpdate = (p.speed != lastSpeed)
                         || (p.generations != lastGenerations)
                         || (p.eqHigh != lastEqHigh)
                         || (p.eqLow  != lastEqLow);
    if (! needUpdate) return;

    lastSpeed       = p.speed;
    lastGenerations = p.generations;
    lastEqHigh      = p.eqHigh;
    lastEqLow       = p.eqLow;

    // Mixer EQ (pre-tape shelving)
    const auto hshelf = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        sampleRate, 10000.0, 0.71, juce::Decibels::decibelsToGain (p.eqHigh));
    const auto lshelf = juce::dsp::IIR::Coefficients<float>::makeLowShelf (
        sampleRate, 100.0, 0.71, juce::Decibels::decibelsToGain (p.eqLow));

    // Speed-dependent bandwidth
    double lpfHz, hpfHz;
    switch (p.speed)
    {
        case 0:  lpfHz = 16000.0; hpfHz = 38.0;  break; // HIGH
        case 2:  lpfHz =  9500.0; hpfHz = 52.0;  break; // SLOW
        default: lpfHz = 14000.0; hpfHz = 40.0;  break; // NORM
    }
    const auto lpf = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass  (sampleRate, lpfHz);
    const auto hpf = juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass (sampleRate, hpfHz);

    // Generation loss LPF: each bounce rolls off HF
    const double genCutoff = 16000.0 / std::pow (1.35, static_cast<double> (p.generations));
    const auto   genlp = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass (sampleRate, genCutoff);

    // Head bump: +1.5 dB peak at 80 Hz, Q=1.8
    const auto bump = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
        sampleRate, 80.0, 1.8f, juce::Decibels::decibelsToGain (1.5f));

    for (int ch = 0; ch < 2; ++ch)
    {
        *eqHighFilter[ch].coefficients = *hshelf;
        *eqLowFilter[ch].coefficients  = *lshelf;
        *speedLPF[ch].coefficients     = *lpf;
        *speedHPF[ch].coefficients     = *hpf;
        *genLPF[ch].coefficients       = *genlp;
        *headBump[ch].coefficients     = *bump;
    }
}

float CassetteTape::saturate (float x, float drive) noexcept
{
    const float y = x * drive;
    if (y > 1.0f)  return  (2.0f - 1.0f / y) / drive;
    if (y < -1.0f) return -(2.0f - 1.0f / (-y)) / drive;
    // 2nd harmonic bias (even), like ferric oxide asymmetry
    return x + x * x * (drive * 0.028f) - (x * x * x) * (drive * drive * 0.05f);
}

float CassetteTape::readDelay (const std::vector<float>& buf, int writePos, float delaySamples) const noexcept
{
    delaySamples = juce::jlimit (4.0f, static_cast<float> (kDelaySize - 2), delaySamples);
    float readPos = static_cast<float> (writePos) - delaySamples;
    while (readPos < 0.0f) readPos += static_cast<float> (kDelaySize);
    const int p0   = static_cast<int> (readPos) % kDelaySize;
    const int p1   = (p0 + 1) % kDelaySize;
    const float fr = readPos - std::floor (readPos);
    return buf[static_cast<size_t> (p0)] * (1.0f - fr) + buf[static_cast<size_t> (p1)] * fr;
}

void CassetteTape::process (juce::AudioBuffer<float>& buffer, const Parameters& p)
{
    updateFilters (p);

    satSmoothed.setTargetValue   (p.saturation);
    noiseSmoothed.setTargetValue (p.noise);
    wowSmoothed.setTargetValue   (p.wowFlutter);

    const int   numSamples = buffer.getNumSamples();
    const float sr         = static_cast<float> (sampleRate);
    const float dt         = 1.0f / sr;
    const float twoPi      = juce::MathConstants<float>::twoPi;

    // Speed-dependent wow/flutter depths
    float wowDepthSamples, flutterDepthSamples, wowRate;
    switch (p.speed)
    {
        case 0: // HIGH: 0.06% WRMS
            wowDepthSamples     = sr * 0.00085f;
            flutterDepthSamples = sr * 0.00028f;
            wowRate = 0.60f;
            break;
        case 2: // SLOW: 0.20% WRMS (estimated)
            wowDepthSamples     = sr * 0.00283f;
            flutterDepthSamples = sr * 0.00085f;
            wowRate = 0.45f;
            break;
        default: // NORM: 0.10% WRMS
            wowDepthSamples     = sr * 0.00141f;
            flutterDepthSamples = sr * 0.00042f;
            wowRate = 0.55f;
            break;
    }

    // DBX: fast attack (10 ms) / slow release (300 ms) for audible pumping/breathing
    const float dbxAtkTC = std::exp (-1.0f / (0.010f * sr));
    const float dbxRelTC = std::exp (-1.0f / (0.300f * sr));
    // Reference level ~–10 dBFS
    const float dbxRef = 0.316f;

    // Base noise amplitude
    float baseNoise;
    if (p.dbxOn)
        baseNoise = (p.speed == 0) ? 0.00126f : (p.speed == 2 ? 0.00200f : 0.00158f);
    else
        baseNoise = (p.speed == 0) ? 0.00447f : (p.speed == 2 ? 0.00700f : 0.00562f);

    // Per-generation noise multiplier
    const float genNoiseMult = std::pow (1.26f, static_cast<float> (p.generations));

    // Crosstalk: 55 dB separation at 1 kHz
    const float crosstalkAmt = 0.0018f;
    const float crosstalkC   = (twoPi * 3500.0f / sr) / (1.0f + twoPi * 3500.0f / sr);

    // Dropout probability scales with age
    const float dropProbPerSample = p.age * p.age * 0.000012f;

    // Head wear HF loss
    const float ageLPHz = 18000.0f - p.age * 8000.0f;
    const float ageLPC  = (twoPi * ageLPHz / sr) / (1.0f + twoPi * ageLPHz / sr);
    float ageHPL = 0.0f, ageHPR = 0.0f; // per-block state (alias ok)

    // Print-through: post-echo level scales with age
    const float ptAmt = 0.0030f * p.age; // ~–50 dBFS at age=1

    // Room early-reflection gains (-18, -22, -26 dB)
    constexpr float kRoom1 = 0.1260f, kRoom2 = 0.0794f, kRoom3 = 0.0501f;
    const int ptBufLen = static_cast<int> (ptBufL.size());

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getWritePointer (1);

    const float trimGain = juce::Decibels::decibelsToGain (p.trim);
    const float dropSmoothFactor = 1.0f - std::exp (-1.0f / (0.001f * sr));

    for (int i = 0; i < numSamples; ++i)
    {
        const float sat      = satSmoothed.getNextValue();
        const float noiseAmt = noiseSmoothed.getNextValue();
        const float wowAmt   = wowSmoothed.getNextValue();

        float inL = L[i] * trimGain;
        float inR = R[i] * trimGain;

        // ── 1. Mixer EQ ──────────────────────────────────────────────────────
        inL = eqHighFilter[0].processSample (eqLowFilter[0].processSample (inL));
        inR = eqHighFilter[1].processSample (eqLowFilter[1].processSample (inR));

        // ── 1b. IEC Type I record pre-emphasis (+6 dB shelf at 1326 Hz) ──────
        inL = iecRecordEQ[0].processSample (inL);
        inR = iecRecordEQ[1].processSample (inR);

        // ── 2. DBX encode — fast attack / slow release for pumping ───────────
        if (p.dbxOn)
        {
            const float pwrL = inL * inL, pwrR = inR * inR;
            dbxEncRmsL = (pwrL > dbxEncRmsL)
                ? dbxAtkTC * dbxEncRmsL + (1.0f - dbxAtkTC) * pwrL
                : dbxRelTC * dbxEncRmsL + (1.0f - dbxRelTC) * pwrL;
            dbxEncRmsR = (pwrR > dbxEncRmsR)
                ? dbxAtkTC * dbxEncRmsR + (1.0f - dbxAtkTC) * pwrR
                : dbxRelTC * dbxEncRmsR + (1.0f - dbxRelTC) * pwrR;
            const float rmsL = std::sqrt (dbxEncRmsL + 1.0e-10f);
            const float rmsR = std::sqrt (dbxEncRmsR + 1.0e-10f);
            const float gL = juce::jlimit (0.1f, 10.0f, std::sqrt (dbxRef / rmsL));
            const float gR = juce::jlimit (0.1f, 10.0f, std::sqrt (dbxRef / rmsR));
            inL *= gL;
            inR *= gR;
        }

        // ── 3. Tape saturation ───────────────────────────────────────────────
        const float driveBoost = p.dbxOn ? 0.55f : 1.0f;
        const float drive      = 1.0f + sat * 3.0f * driveBoost;
        inL = saturate (inL, drive);
        inR = saturate (inR, drive);

        // ── 3b. Write to print-through buffer (captures tape record signal) ──
        if (ptBufLen > 0)
        {
            ptBufL[static_cast<size_t> (ptWritePos)] = inL;
            ptBufR[static_cast<size_t> (ptWritePos)] = inR;
        }

        // ── 4. Speed EQ (tape bandwidth) ─────────────────────────────────────
        inL = speedLPF[0].processSample (speedHPF[0].processSample (inL));
        inR = speedLPF[1].processSample (speedHPF[1].processSample (inR));

        // ── 4b. Head wear HF loss ─────────────────────────────────────────────
        ageHPL += ageLPC * (inL - ageHPL);
        ageHPR += ageLPC * (inR - ageHPR);
        if (p.age > 0.01f) { inL = ageHPL; inR = ageHPR; }

        // ── 5. Write into wow/flutter delay line ──────────────────────────────
        delayBufL[static_cast<size_t> (delayWritePos)] = inL;
        delayBufR[static_cast<size_t> (delayWritePos)] = inR;

        // Advance oscillator phases (slowly drifting wow rate)
        wowRateDrift += (random.nextFloat() * 2.0f - 1.0f) * dt * 0.007f;
        wowRateDrift  = juce::jlimit (-0.10f, 0.10f, wowRateDrift);
        const float driftedWow  = wowRate * (1.0f + wowRateDrift);
        const float wowRate2    = driftedWow * 0.38f;
        const float wowRate3    = driftedWow * 0.61f;
        const float flutterRate = (p.speed == 0) ? 16.0f : (p.speed == 2 ? 10.0f : 13.0f);
        const float flutter2    = flutterRate * 1.47f;

        wowPhase1     += driftedWow  * dt; if (wowPhase1     > 1.0f) wowPhase1     -= 1.0f;
        wowPhase2     += wowRate2    * dt; if (wowPhase2     > 1.0f) wowPhase2     -= 1.0f;
        wowPhase3     += wowRate3    * dt; if (wowPhase3     > 1.0f) wowPhase3     -= 1.0f;
        flutterPhase1 += flutterRate * dt; if (flutterPhase1 > 1.0f) flutterPhase1 -= 1.0f;
        flutterPhase2 += flutter2    * dt; if (flutterPhase2 > 1.0f) flutterPhase2 -= 1.0f;

        const float wowMod = std::sin (twoPi * wowPhase1) * wowDepthSamples * 1.0f
                           + std::sin (twoPi * wowPhase2) * wowDepthSamples * 0.4f
                           + std::sin (twoPi * wowPhase3 + 0.9f) * wowDepthSamples * 0.25f;
        const float flutterNoise = random.nextFloat() * 2.0f - 1.0f;
        const float flutterMod  = (std::sin (twoPi * flutterPhase1)
                                  + 0.5f * std::sin (twoPi * flutterPhase2 + 1.3f)
                                  + flutterNoise * 0.15f) * flutterDepthSamples;

        const float delaySamples = 256.0f + wowMod * wowAmt + flutterMod * wowAmt;
        inL = readDelay (delayBufL, delayWritePos, delaySamples);
        inR = readDelay (delayBufR, delayWritePos, delaySamples);
        delayWritePos = (delayWritePos + 1) % kDelaySize;

        // ── 6. Tape noise + AC hum ───────────────────────────────────────────
        const float noiseHPCoeff = (twoPi * 200.0f  / sr) / (1.0f + twoPi * 200.0f  / sr);
        const float noiseLPCoeff = (twoPi * 9000.0f / sr) / (1.0f + twoPi * 9000.0f / sr);

        const float rawNL = random.nextFloat() * 2.0f - 1.0f;
        const float rawNR = random.nextFloat() * 2.0f - 1.0f;
        noiseLPL += noiseLPCoeff * (rawNL - noiseLPL);
        noiseLPR += noiseLPCoeff * (rawNR - noiseLPR);
        const float shapedNL = noiseLPL - noiseHPL;
        const float shapedNR = noiseLPR - noiseHPR;
        noiseHPL += noiseHPCoeff * (noiseLPL - noiseHPL);
        noiseHPR += noiseHPCoeff * (noiseLPR - noiseHPR);

        const float noiseLevel = noiseAmt * baseNoise * 2.0f * genNoiseMult;
        inL += shapedNL * noiseLevel;
        inR += shapedNR * noiseLevel;

        // AC hum: 60 Hz fundamental + 120 Hz harmonic, mono (ground loop)
        humPhase += 60.0f * dt;
        if (humPhase >= 1.0f) humPhase -= 1.0f;
        const float hum = (std::sin (twoPi * humPhase) * 0.70f
                         + std::sin (twoPi * humPhase * 2.0f) * 0.30f)
                        * noiseAmt * 0.0050f; // ~–46 dBFS at noise=1
        inL += hum;
        inR += hum * 0.97f; // slight channel asymmetry

        // ── 7. Crosstalk ─────────────────────────────────────────────────────
        crosstalkLPL += crosstalkC * (inL - crosstalkLPL);
        crosstalkLPR += crosstalkC * (inR - crosstalkLPR);
        inL += crosstalkLPR * crosstalkAmt;
        inR += crosstalkLPL * crosstalkAmt;

        // ── 8. DBX decode — fast attack / slow release ───────────────────────
        if (p.dbxOn)
        {
            const float pwrL = inL * inL, pwrR = inR * inR;
            dbxDecRmsL = (pwrL > dbxDecRmsL)
                ? dbxAtkTC * dbxDecRmsL + (1.0f - dbxAtkTC) * pwrL
                : dbxRelTC * dbxDecRmsL + (1.0f - dbxRelTC) * pwrL;
            dbxDecRmsR = (pwrR > dbxDecRmsR)
                ? dbxAtkTC * dbxDecRmsR + (1.0f - dbxAtkTC) * pwrR
                : dbxRelTC * dbxDecRmsR + (1.0f - dbxRelTC) * pwrR;
            const float rmsL = std::sqrt (dbxDecRmsL + 1.0e-10f);
            const float rmsR = std::sqrt (dbxDecRmsR + 1.0e-10f);
            const float gL = juce::jlimit (0.05f, 20.0f, std::sqrt (rmsL / dbxRef));
            const float gR = juce::jlimit (0.05f, 20.0f, std::sqrt (rmsR / dbxRef));
            inL *= gL;
            inR *= gR;
        }

        // ── 9. Dropout ───────────────────────────────────────────────────────
        if (! dropoutActive && random.nextFloat() < dropProbPerSample)
        {
            dropoutActive = true;
            dropoutTimer  = sr * (0.003f + random.nextFloat() * 0.012f);
            dropoutGain   = 0.40f + random.nextFloat() * 0.45f;
        }
        if (dropoutActive && --dropoutTimer <= 0.0f)
            dropoutActive = false;
        dropoutGainSmooth += dropSmoothFactor * ((dropoutActive ? dropoutGain : 1.0f) - dropoutGainSmooth);
        inL *= dropoutGainSmooth;
        inR *= dropoutGainSmooth;

        // ── 10. Generation loss ───────────────────────────────────────────────
        if (p.generations > 0)
        {
            inL = genLPF[0].processSample (inL);
            inR = genLPF[1].processSample (inR);
        }

        // ── 11. IEC Type I playback de-emphasis (-6 dB shelf at 1326 Hz) ─────
        inL = iecPlaybackEQ[0].processSample (inL);
        inR = iecPlaybackEQ[1].processSample (inR);

        // ── 12. Head bump ─────────────────────────────────────────────────────
        inL = headBump[0].processSample (inL);
        inR = headBump[1].processSample (inR);

        // ── 13. DC block ──────────────────────────────────────────────────────
        float dcbL = inL - dcBlockL; dcBlockL = inL - dcbL * 0.9998f;
        float dcbR = inR - dcBlockR; dcBlockR = inR - dcbR * 0.9998f;
        inL = dcbL; inR = dcbR;

        // ── 14. Print-through post-echo ───────────────────────────────────────
        if (ptBufLen > 0 && ptAmt > 0.0f)
        {
            const int ptRd = (ptWritePos - ptDelSamples + ptBufLen) % ptBufLen;
            inL += ptBufL[static_cast<size_t> (ptRd)] * ptAmt;
            inR += ptBufR[static_cast<size_t> (ptRd)] * ptAmt;
        }
        ptWritePos = (ptWritePos + 1) % juce::jmax (1, ptBufLen);

        // ── 15. Room early reflections (small bedroom) ────────────────────────
        roomBufL[static_cast<size_t> (roomWritePos)] = inL;
        roomBufR[static_cast<size_t> (roomWritePos)] = inR;
        {
            const int r1 = (roomWritePos - roomTap1 + kRoomSize) % kRoomSize;
            const int r2 = (roomWritePos - roomTap2 + kRoomSize) % kRoomSize;
            const int r3 = (roomWritePos - roomTap3 + kRoomSize) % kRoomSize;
            inL += roomBufL[static_cast<size_t>(r1)] * kRoom1
                 + roomBufL[static_cast<size_t>(r2)] * kRoom2
                 + roomBufL[static_cast<size_t>(r3)] * kRoom3;
            inR += roomBufR[static_cast<size_t>(r1)] * kRoom1
                 + roomBufR[static_cast<size_t>(r2)] * kRoom2
                 + roomBufR[static_cast<size_t>(r3)] * kRoom3;
        }
        roomWritePos = (roomWritePos + 1) % kRoomSize;

        L[i] = inL;
        R[i] = inR;
    }

    // VU meter: RMS with ~300 ms ballistics (0 VU = –18 dBFS)
    double sumSq = 0.0;
    for (int i = 0; i < numSamples; ++i)
        sumSq += static_cast<double> (L[i]) * L[i] + static_cast<double> (R[i]) * R[i];
    const float blockRms  = std::sqrt (static_cast<float> (sumSq / juce::jmax (1, numSamples * 2)));
    const float blockSecs = static_cast<float> (numSamples) / sr;
    const float coeff     = 1.0f - std::exp (-blockSecs / 0.300f);
    vuSmooth += coeff * (blockRms - vuSmooth);
    vuLevel.store (vuSmooth);
}
