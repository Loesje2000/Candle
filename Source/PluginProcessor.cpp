#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace ParamID
{
constexpr const char* trim        = "trim";
constexpr const char* eqHigh      = "eq_high";
constexpr const char* eqLow       = "eq_low";
constexpr const char* dbx         = "dbx";
constexpr const char* tapeSpeed   = "tape_speed";
constexpr const char* saturation  = "saturation";
constexpr const char* noise       = "noise";
constexpr const char* wowFlutter  = "wow_flutter";
constexpr const char* age         = "age";
constexpr const char* generations = "generations";
constexpr const char* micOn       = "mic_on";
constexpr const char* outputGain  = "output_gain";
constexpr const char* globalBypass = "global_bypass";
}

static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    auto pct = [] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; };
    auto db  = [] (float v, int) { return juce::String (v, 1) + " dB"; };
    auto onoff = [] (float v, int) { return v >= 0.5f ? "On" : "Off"; };

    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::trim, 1 }, "Trim",
        juce::NormalisableRange<float> (-20.0f, 20.0f), 0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (db)));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::eqHigh, 1 }, "EQ High",
        juce::NormalisableRange<float> (-10.0f, 10.0f), 0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (db)));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::eqLow, 1 }, "EQ Low",
        juce::NormalisableRange<float> (-10.0f, 10.0f), 0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (db)));

    p.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamID::dbx, 1 }, "DBX",
        true, juce::AudioParameterBoolAttributes().withStringFromValueFunction (onoff)));

    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParamID::tapeSpeed, 1 }, "Tape Speed",
        juce::StringArray { "HIGH", "NORM", "SLOW" }, 1));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::saturation, 1 }, "Saturation",
        juce::NormalisableRange<float> (0.0f, 1.0f), 0.4f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (pct)));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::noise, 1 }, "Noise",
        juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (pct)));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::wowFlutter, 1 }, "Wow/Flutter",
        juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (pct)));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::age, 1 }, "Age",
        juce::NormalisableRange<float> (0.0f, 1.0f), 0.2f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (pct)));

    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParamID::generations, 1 }, "Generations",
        juce::StringArray { "0", "1", "2", "3", "4" }, 0));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::outputGain, 1 }, "Output Gain",
        juce::NormalisableRange<float> (-12.0f, 12.0f), 0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (db)));

    p.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamID::micOn, 1 }, "Realistic Mic",
        false, juce::AudioParameterBoolAttributes().withStringFromValueFunction (onoff)));

    p.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamID::globalBypass, 1 }, "Bypass",
        false, juce::AudioParameterBoolAttributes().withStringFromValueFunction (onoff)));

    return { p.begin(), p.end() };
}

CandleAudioProcessor::CandleAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", makeLayout())
{
}

CandleAudioProcessor::APVTS::ParameterLayout CandleAudioProcessor::createParameterLayout()
{
    return makeLayout();
}

void CandleAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 };
    mic.prepare (spec);

    oversampling.initProcessing (static_cast<size_t> (samplesPerBlock));
    oversampling.reset();

    const auto factor = oversampling.getOversamplingFactor();
    juce::dsp::ProcessSpec oversampledSpec { sampleRate * static_cast<double> (factor),
                                              static_cast<juce::uint32> (samplesPerBlock * static_cast<int> (factor)),
                                              2 };
    tape.prepare (oversampledSpec);

    setLatencySamples (juce::roundToInt (oversampling.getLatencyInSamples()));

    outputGainSmooth.reset (sampleRate, 0.02);
    outputGainSmooth.setCurrentAndTargetValue (1.0f);
    drySafetyBuffer.setSize (2, samplesPerBlock, false, false, true);
    wasBypassed = false;
}

void CandleAudioProcessor::releaseResources()
{
    mic.reset();
    tape.reset();
    oversampling.reset();
    wasBypassed = false;
}

bool CandleAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto input  = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();

    const bool inputOk  = input == juce::AudioChannelSet::mono()
                       || input == juce::AudioChannelSet::stereo();
    const bool outputOk = output == juce::AudioChannelSet::mono()
                       || output == juce::AudioChannelSet::stereo();

    return inputOk && outputOk && output.size() >= input.size();
}

void CandleAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int inCh  = getTotalNumInputChannels();
    const int outCh = getTotalNumOutputChannels();
    for (int ch = inCh; ch < outCh; ++ch) buffer.clear (ch, 0, buffer.getNumSamples());
    if (buffer.getNumChannels() == 0) return;

    if (inCh == 1 && outCh >= 2 && buffer.getNumChannels() >= 2)
        buffer.copyFrom (1, 0, buffer, 0, 0, buffer.getNumSamples());

    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    sanitizeAudio (buffer);
    const float inputRms = getStereoRms (buffer);
    inputMeterDb.store (juce::Decibels::gainToDecibels (
        juce::jmax (inputRms, 1.0e-6f), -100.0f));

    drySafetyBuffer.setSize (juce::jmin (2, buffer.getNumChannels()), buffer.getNumSamples(),
                             false, false, true);
    for (int ch = 0; ch < drySafetyBuffer.getNumChannels(); ++ch)
        drySafetyBuffer.copyFrom (ch, 0, buffer, ch, 0, buffer.getNumSamples());

    if (buffer.getNumChannels() < 2)
    {
        outputMeterDb.store (inputMeterDb.load());
        return;
    }

    const bool bypassed = get (ParamID::globalBypass) > 0.5f;
    if (bypassed)
    {
        outputMeterDb.store (inputMeterDb.load());
        wasBypassed = true;
        return;
    }

    if (wasBypassed)
    {
        mic.reset();
        tape.reset();
        oversampling.reset();
        outputGainSmooth.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (get (ParamID::outputGain)));
        wasBypassed = false;
    }

    mic.process (buffer, MicStage::Parameters { get (ParamID::micOn) > 0.5f });

    CassetteTape::Parameters p;
    p.trim        = get (ParamID::trim);
    p.eqHigh      = get (ParamID::eqHigh);
    p.eqLow       = get (ParamID::eqLow);
    p.dbxOn       = get (ParamID::dbx) > 0.5f;
    p.speed       = static_cast<int> (get (ParamID::tapeSpeed) + 0.5f);
    p.saturation  = get (ParamID::saturation);
    p.noise       = get (ParamID::noise);
    p.wowFlutter  = get (ParamID::wowFlutter);
    p.age         = get (ParamID::age);
    p.generations = static_cast<int> (get (ParamID::generations) + 0.5f);

    {
        juce::dsp::AudioBlock<float> nativeBlock (buffer);
        auto ovBlock = oversampling.processSamplesUp (nativeBlock);

        float* ovChannels[2] = { ovBlock.getChannelPointer (0), ovBlock.getChannelPointer (1) };
        juce::AudioBuffer<float> ovBuffer (ovChannels, 2, static_cast<int> (ovBlock.getNumSamples()));

        tape.process (ovBuffer, p);
        if (sanitizeAudio (ovBuffer))
        {
            tape.reset();
            oversampling.reset();
        }

        oversampling.processSamplesDown (nativeBlock);
    }
    if (sanitizeAudio (buffer))
    {
        tape.reset();
        oversampling.reset();
    }

    // Output gain
    outputGainSmooth.setTargetValue (juce::Decibels::decibelsToGain (get (ParamID::outputGain)));
    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getWritePointer (1);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const float g = outputGainSmooth.getNextValue();
        L[i] *= g;
        R[i] *= g;
    }
    if (sanitizeAudio (buffer))
        tape.reset();

    float outputRms = getStereoRms (buffer);
    if (inputRms > 3.0e-4f && outputRms < 1.0e-5f)
    {
        for (int ch = 0; ch < drySafetyBuffer.getNumChannels(); ++ch)
            buffer.copyFrom (ch, 0, drySafetyBuffer, ch, 0, buffer.getNumSamples());
        outputRms = getStereoRms (buffer);
    }

    outputMeterDb.store (juce::Decibels::gainToDecibels (
        juce::jmax (outputRms, 1.0e-6f), -100.0f));
}

juce::AudioProcessorEditor* CandleAudioProcessor::createEditor()
{
    return new CandleAudioProcessorEditor (*this);
}

void CandleAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void CandleAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

float CandleAudioProcessor::getStereoRms (const juce::AudioBuffer<float>& buf) const noexcept
{
    const int n = buf.getNumSamples();
    const int channels = juce::jmin (2, buf.getNumChannels());
    if (n <= 0 || channels <= 0) return 0.0f;
    double sum = 0.0;
    for (int ch = 0; ch < channels; ++ch)
    {
        const auto* data = buf.getReadPointer (ch);
        for (int i = 0; i < n; ++i)
            sum += static_cast<double> (data[i]) * data[i];
    }
    return std::sqrt (static_cast<float> (sum / static_cast<double> (n * channels)));
}

bool CandleAudioProcessor::sanitizeAudio (juce::AudioBuffer<float>& buf) noexcept
{
    bool changed = false;
    constexpr float kMax = 4.0f;
    for (int ch = 0; ch < juce::jmin (2, buf.getNumChannels()); ++ch)
    {
        auto* data = buf.getWritePointer (ch);
        for (int i = 0; i < buf.getNumSamples(); ++i)
        {
            if (! std::isfinite (data[i])) { data[i] = 0.0f; changed = true; }
            else
            {
                const float c = juce::jlimit (-kMax, kMax, data[i]);
                changed = changed || c != data[i];
                data[i] = c;
            }
        }
    }
    return changed;
}

juce::StringArray CandleAudioProcessor::getPresetNames() const
{
    return { "Candle", "Porta", "Bounce x3", "Late Night", "New Tape" };
}

void CandleAudioProcessor::applyFactoryPreset (int index)
{
    auto set = [this] (const char* id, float value)
    {
        if (auto* param = apvts.getParameter (id))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost (param->convertTo0to1 (value));
            param->endChangeGesture();
        }
    };

    // Reset to neutral first
    set (ParamID::trim, 0.0f);  set (ParamID::eqHigh, 0.0f);   set (ParamID::eqLow, 0.0f);
    set (ParamID::dbx, 1.0f);   set (ParamID::tapeSpeed, 1.0f); set (ParamID::saturation, 0.4f);
    set (ParamID::noise, 0.5f); set (ParamID::wowFlutter, 0.3f); set (ParamID::age, 0.2f);
    set (ParamID::generations, 0.0f); set (ParamID::outputGain, 0.0f);

    switch (index)
    {
        case 0: // Candle: DBX off, NORM speed — noisy but not warbling
            set (ParamID::dbx, 0.0f);        set (ParamID::tapeSpeed, 1.0f);
            set (ParamID::saturation, 0.45f); set (ParamID::noise, 0.55f);
            set (ParamID::wowFlutter, 0.30f); set (ParamID::age, 0.30f);
            set (ParamID::generations, 1.0f); set (ParamID::eqLow, -1.5f);
            set (ParamID::micOn, 1.0f);
            break;
        case 1: // Porta: DBX on, HIGH speed, clean cassette
            set (ParamID::tapeSpeed, 0.0f);   set (ParamID::saturation, 0.3f);
            set (ParamID::noise, 0.35f);       set (ParamID::wowFlutter, 0.20f);
            break;
        case 2: // Bounce x3: DBX off, NORM — accumulated noise, still musical
            set (ParamID::dbx, 0.0f);          set (ParamID::saturation, 0.5f);
            set (ParamID::noise, 0.62f);        set (ParamID::wowFlutter, 0.35f);
            set (ParamID::age, 0.30f);          set (ParamID::generations, 3.0f);
            break;
        case 3: // Late Night: DBX on, aged transport, warmer
            set (ParamID::tapeSpeed, 0.0f);    set (ParamID::saturation, 0.45f);
            set (ParamID::noise, 0.45f);        set (ParamID::wowFlutter, 0.35f);
            set (ParamID::age, 0.60f);          set (ParamID::eqLow, 1.5f);
            break;
        case 4: // New Tape: DBX on, HIGH, minimal artifacts
            set (ParamID::tapeSpeed, 0.0f);    set (ParamID::saturation, 0.15f);
            set (ParamID::noise, 0.20f);        set (ParamID::wowFlutter, 0.10f);
            set (ParamID::age, 0.0f);
            break;
        default: break;
    }
}

juce::File CandleAudioProcessor::getUserPresetDirectory() const
{
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("Rubblesonic").getChildFile ("Candle").getChildFile ("Presets");
    dir.createDirectory();
    return dir;
}

juce::StringArray CandleAudioProcessor::getUserPresetNames() const
{
    juce::StringArray names;
    for (const auto& file : getUserPresetDirectory().findChildFiles (juce::File::findFiles, false, "*.xml"))
        if (auto xml = juce::XmlDocument::parse (file))
            names.add (xml->getStringAttribute ("presetName", file.getFileNameWithoutExtension()));
    names.sort (true);
    return names;
}

bool CandleAudioProcessor::saveUserPreset (const juce::String& name)
{
    const auto trimmed = name.trim();
    if (trimmed.isEmpty())
        return false;

    auto xml = apvts.copyState().createXml();
    if (xml == nullptr)
        return false;

    xml->setAttribute ("presetName", trimmed);
    const auto file = getUserPresetDirectory().getChildFile (juce::File::createLegalFileName (trimmed) + ".xml");
    return xml->writeTo (file);
}

bool CandleAudioProcessor::loadUserPresetByName (const juce::String& name)
{
    for (const auto& file : getUserPresetDirectory().findChildFiles (juce::File::findFiles, false, "*.xml"))
    {
        if (auto xml = juce::XmlDocument::parse (file))
        {
            const auto presetName = xml->getStringAttribute ("presetName", file.getFileNameWithoutExtension());
            if (presetName == name && xml->hasTagName (apvts.state.getType()))
            {
                apvts.replaceState (juce::ValueTree::fromXml (*xml));
                return true;
            }
        }
    }
    return false;
}

bool CandleAudioProcessor::deleteUserPreset (const juce::String& name)
{
    for (const auto& file : getUserPresetDirectory().findChildFiles (juce::File::findFiles, false, "*.xml"))
    {
        if (auto xml = juce::XmlDocument::parse (file))
        {
            const auto presetName = xml->getStringAttribute ("presetName", file.getFileNameWithoutExtension());
            if (presetName == name)
                return file.deleteFile();
        }
    }
    return false;
}

bool CandleAudioProcessor::renameUserPreset (const juce::String& oldName, const juce::String& newName)
{
    const auto trimmedNew = newName.trim();
    if (trimmedNew.isEmpty())
        return false;

    juce::File oldFile;
    for (const auto& file : getUserPresetDirectory().findChildFiles (juce::File::findFiles, false, "*.xml"))
    {
        if (auto xml = juce::XmlDocument::parse (file))
        {
            const auto presetName = xml->getStringAttribute ("presetName", file.getFileNameWithoutExtension());
            if (presetName == oldName) { oldFile = file; break; }
        }
    }
    if (! oldFile.existsAsFile())
        return false;

    auto xml = juce::XmlDocument::parse (oldFile);
    if (xml == nullptr)
        return false;

    xml->setAttribute ("presetName", trimmedNew);
    const auto newFile = getUserPresetDirectory().getChildFile (juce::File::createLegalFileName (trimmedNew) + ".xml");

    if (! xml->writeTo (newFile))
        return false;

    if (newFile != oldFile)
        oldFile.deleteFile();

    return true;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new CandleAudioProcessor(); }
