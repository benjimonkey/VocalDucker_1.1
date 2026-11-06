#include "PluginEditor.h"

//==============================================================================
VocalMixerAudioProcessorEditor::VocalMixerAudioProcessorEditor (VocalMixerAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setupKnob (instSlider,   instLabel,   "Instruments");
    setupKnob (vocalSlider,  vocalLabel,  "Vocal");
    setupKnob (masterSlider, masterLabel, "Master Out");

    // Bind each knob to its parameter.
    instAttachment   = std::make_unique<SliderAttachment> (processorRef.apvts, ParamIDs::instGain,   instSlider);
    vocalAttachment  = std::make_unique<SliderAttachment> (processorRef.apvts, ParamIDs::vocalGain,  vocalSlider);
    masterAttachment = std::make_unique<SliderAttachment> (processorRef.apvts, ParamIDs::masterGain, masterSlider);

    // Final window size is 1000 x 660 (see stage 8).
    setSize (1000, 660);
}

void VocalMixerAudioProcessorEditor::setupKnob (juce::Slider& slider, juce::Label& label,
                                                const juce::String& caption)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    slider.setTextValueSuffix (" dB");
    slider.setDoubleClickReturnValue (true, 0.0);   // double-click resets to 0 dB
    addAndMakeVisible (slider);

    label.setText (caption, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (label);
}

void VocalMixerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1b1d22));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (24.0f));
    g.drawFittedText ("VocalMixer", getLocalBounds().removeFromTop (60),
                      juce::Justification::centred, 1);
}

void VocalMixerAudioProcessorEditor::resized()
{
    // Three equal columns, centred in the window.
    auto area = getLocalBounds().withSizeKeepingCentre (600, 260);
    const int columnWidth = area.getWidth() / 3;

    auto layoutColumn = [&area, columnWidth] (juce::Label& label, juce::Slider& slider)
    {
        auto column = area.removeFromLeft (columnWidth);
        label.setBounds (column.removeFromTop (24));
        slider.setBounds (column.reduced (10));
    };

    layoutColumn (instLabel,   instSlider);
    layoutColumn (vocalLabel,  vocalSlider);
    layoutColumn (masterLabel, masterSlider);
}
