#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

//==============================================================================
// VocalMixerAudioProcessorEditor
// Stage 1: blank window with a title. The signal-flow GUI arrives in stage 8.
//==============================================================================
class VocalMixerAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VocalMixerAudioProcessorEditor (VocalMixerAudioProcessor&);
    ~VocalMixerAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VocalMixerAudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalMixerAudioProcessorEditor)
};
