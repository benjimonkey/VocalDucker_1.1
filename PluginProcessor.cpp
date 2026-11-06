#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
VocalMixerAudioProcessor::VocalMixerAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

//==============================================================================
// Parameter layout. Empty for now; stages 2-5 add gains, gate, filter, ducker.
juce::AudioProcessorValueTreeState::ParameterLayout
VocalMixerAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    return layout;
}

//==============================================================================
void VocalMixerAudioProcessor::prepareToPlay (double /*sampleRate*/, int /*samplesPerBlock*/)
{
    // Nothing to prepare yet.
}

void VocalMixerAudioProcessor::releaseResources()
{
}

bool VocalMixerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Stage 1: plain stereo in / stereo out only.
    return layouts.getMainInputChannelSet()  == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void VocalMixerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& /*midi*/)
{
    // Avoid denormals slowing down the audio thread.
    juce::ScopedNoDenormals noDenormals;

    // Clear any output channels that have no matching input.
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    // Pass-through: the buffer already holds the input audio.
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
