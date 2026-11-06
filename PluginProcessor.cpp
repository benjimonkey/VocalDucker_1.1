#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    // Gain knobs go from -60 dB (treated as silence) to +12 dB.
    constexpr float kMinGainDb = -60.0f;
    constexpr float kMaxGainDb = 12.0f;

    // dB -> linear gain; anything at or below -60 dB becomes exactly 0.
    inline float dbToGain (float db) noexcept
    {
        return juce::Decibels::decibelsToGain (db, kMinGainDb);
    }
}

//==============================================================================
VocalMixerAudioProcessor::VocalMixerAudioProcessor()
    : AudioProcessor (BusesProperties()
                          // Stereo Input 1 = instruments (main bus)
                          .withInput  ("Instruments", juce::AudioChannelSet::stereo(), true)
                          // Stereo Input 2 = vocal (aux bus)
                          .withInput  ("Vocal",       juce::AudioChannelSet::stereo(), true)
                          // Stereo Output 1
                          .withOutput ("Output",      juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    // Cache the raw parameter pointers for fast audio-thread access.
    instGainParam   = apvts.getRawParameterValue (ParamIDs::instGain);
    vocalGainParam  = apvts.getRawParameterValue (ParamIDs::vocalGain);
    masterGainParam = apvts.getRawParameterValue (ParamIDs::masterGain);

    // Tell the host the latency right away (also refreshed in prepareToPlay).
    setLatencySamples (kLatencySamples);
}

//==============================================================================
// Parameter layout: three gain knobs, each -60 to +12 dB, default 0 dB.
juce::AudioProcessorValueTreeState::ParameterLayout
VocalMixerAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    const juce::NormalisableRange<float> gainRange (kMinGainDb, kMaxGainDb, 0.1f);
    const auto gainAttributes = juce::AudioParameterFloatAttributes().withLabel ("dB");

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::instGain, 1 },   "Instrument Gain",
        gainRange, 0.0f, gainAttributes));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::vocalGain, 1 },  "Vocal Gain",
        gainRange, 0.0f, gainAttributes));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::masterGain, 1 }, "Master Gain",
        gainRange, 0.0f, gainAttributes));

    return layout;
}

//==============================================================================
void VocalMixerAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    // Report constant latency to the host.
    setLatencySamples (kLatencySamples);

    // Allocate and clear the alignment delays (never allocate in processBlock).
    instDelay.prepare  (kNumChannels, kLatencySamples);
    vocalDelay.prepare (kNumChannels, kLatencySamples);

    // Set up 20 ms gain smoothing and jump straight to the current values.
    constexpr double smoothSeconds = 0.02;

    instGainSmooth.reset   (sampleRate, smoothSeconds);
    vocalGainSmooth.reset  (sampleRate, smoothSeconds);
    masterGainSmooth.reset (sampleRate, smoothSeconds);

    instGainSmooth.setCurrentAndTargetValue   (dbToGain (instGainParam->load()));
    vocalGainSmooth.setCurrentAndTargetValue  (dbToGain (vocalGainParam->load()));
    masterGainSmooth.setCurrentAndTargetValue (dbToGain (masterGainParam->load()));
}

void VocalMixerAudioProcessor::releaseResources()
{
    instDelay.reset();
    vocalDelay.reset();
}

bool VocalMixerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto stereo = juce::AudioChannelSet::stereo();

    // Instruments (main in) and the output must be stereo.
    if (layouts.getMainInputChannelSet()  != stereo) return false;
    if (layouts.getMainOutputChannelSet() != stereo) return false;

    // The vocal aux input may be stereo, or disabled (treated as silence).
    const auto& vocalSet = layouts.getChannelSet (true, 1);
    return vocalSet == stereo || vocalSet.isDisabled();
}

//==============================================================================
void VocalMixerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& /*midi*/)
{
    // Avoid denormals slowing down the audio thread.
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();

    // Safety: we need at least the stereo main bus in the buffer.
    if (buffer.getNumChannels() < kNumChannels)
        return;

    // Is the vocal aux input actually connected?
    auto* vocalBus = getBus (true, 1);
    const bool vocalActive = (vocalBus != nullptr) && vocalBus->isEnabled();

    // Look up which buffer channels belong to which bus.
    // NOTE: the main output shares its channels with the main (instrument)
    // input, so writing the output overwrites the instrument input. We always
    // read a sample from every input BEFORE writing the output sample.
    float*       instPtr[kNumChannels] = {};
    float*       outPtr[kNumChannels]  = {};
    const float* vocalPtr[kNumChannels] = {};

    float* const* channelData = buffer.getArrayOfWritePointers();

    for (int ch = 0; ch < kNumChannels; ++ch)
    {
        instPtr[ch] = channelData[getChannelIndexInProcessBlockBuffer (true, 0, ch)];
        outPtr[ch]  = channelData[getChannelIndexInProcessBlockBuffer (false, 0, ch)];

        if (vocalActive)
            vocalPtr[ch] = channelData[getChannelIndexInProcessBlockBuffer (true, 1, ch)];
    }

    // Pick up the latest knob positions (smoothed per sample below).
    instGainSmooth.setTargetValue   (dbToGain (instGainParam->load()));
    vocalGainSmooth.setTargetValue  (dbToGain (vocalGainParam->load()));
    masterGainSmooth.setTargetValue (dbToGain (masterGainParam->load()));

    for (int i = 0; i < numSamples; ++i)
    {
        // One gain value per sample, shared by both channels.
        const float instGain   = instGainSmooth.getNextValue();
        const float vocalGain  = vocalGainSmooth.getNextValue();
        const float masterGain = masterGainSmooth.getNextValue();

        for (int ch = 0; ch < kNumChannels; ++ch)
        {
            // 1) Read every input sample first (output aliases instrument input).
            const float instIn  = instPtr[ch][i];
            const float vocalIn = vocalActive ? vocalPtr[ch][i] : 0.0f;

            // 2) Delay both paths by the same constant latency.
            const float instDelayed  = instDelay.process  (ch, instIn);
            const float vocalDelayed = vocalDelay.process (ch, vocalIn);

            // 3) Per-channel gain before the sum, then master gain on the sum.
            const float mixed = (instGain * instDelayed) + (vocalGain * vocalDelayed);

            // 4) Only now write the output sample.
            outPtr[ch][i] = masterGain * mixed;
        }
    }
}

//==============================================================================
juce::AudioProcessorEditor* VocalMixerAudioProcessor::createEditor()
{
    return new VocalMixerAudioProcessorEditor (*this);
}

//==============================================================================
void VocalMixerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Save all parameters as XML.
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void VocalMixerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Restore parameters from XML, checking the tag matches.
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
// Entry point used by the plugin wrapper to create the processor.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalMixerAudioProcessor();
}
