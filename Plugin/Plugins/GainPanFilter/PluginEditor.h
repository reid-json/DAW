/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class GainPanAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    GainPanAudioProcessorEditor (GainPanAudioProcessor&, juce::AudioProcessorValueTreeState& vts);
    ~GainPanAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    GainPanAudioProcessor& audioProcessor;
    juce::Slider gainSlider;
    juce::Slider panSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>gainAttachment;
    juce::Label gainLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>panAttachment;
    juce::Label panLabel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainPanAudioProcessorEditor)
};
