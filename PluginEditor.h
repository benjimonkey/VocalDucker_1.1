#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

#include <memory>

//==============================================================================
// VocalMixerAudioProcessorEditor
// Stage 2: three gain knobs (Instrument, Vocal, Master) so the I/O stage can be
// tested. The signal-flow GUI replaces this in stage 8.
//==============================================================================
class VocalMixerAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VocalMixerAudioProcessorEditor (VocalMixerAudioProcessor&);
    ~VocalMixerAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // Configure a rotary knob and its caption label.
    void setupKnob (juce::Slider& slider, juce::Label& label, const juce::String& caption);

    VocalMixerAudioProcessor& processorRef;

    juce::Slider instSlider, vocalSlider, masterSlider;
    juce::Label  instLabel,  vocalLabel,  masterLabel;

    // Attachments keep the knobs and the parameters in sync.
    // (Declared after the sliders so they are destroyed first.)
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> instAttachment, vocalAttachment, masterAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalMixerAudioProcessorEditor)
};
