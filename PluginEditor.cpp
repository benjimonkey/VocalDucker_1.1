#include "PluginEditor.h"

//==============================================================================
VocalMixerAudioProcessorEditor::VocalMixerAudioProcessorEditor (VocalMixerAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    // Final window size is 1000 x 660 (see stage 8).
    setSize (1000, 660);
}

void VocalMixerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1b1d22));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (24.0f));
    g.drawFittedText ("VocalMixer", getLocalBounds(), juce::Justification::centred, 1);
}

void VocalMixerAudioProcessorEditor::resized()
{
    // No child components yet.
}
