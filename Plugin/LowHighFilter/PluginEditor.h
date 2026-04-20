#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class LowpassHighpassFilterAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    LowpassHighpassFilterAudioProcessorEditor(LowpassHighpassFilterAudioProcessor&, juce::AudioProcessorValueTreeState& vts);
    ~LowpassHighpassFilterAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::Slider cutoffFrequencySlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cutoffFrequencyAttachment;
    juce::Label cutoffFrequencyLabel;
    juce::ToggleButton highpassButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> highpassAttachment;
    juce::Label highpassButtonLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LowpassHighpassFilterAudioProcessorEditor)
};
