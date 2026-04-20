#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class GainPanAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    GainPanAudioProcessorEditor(GainPanAudioProcessor&, juce::AudioProcessorValueTreeState& vts);
    ~GainPanAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    GainPanAudioProcessor& audioProcessor;
    juce::Slider gainSlider;
    juce::Slider panSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    juce::Label gainLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAttachment;
    juce::Label panLabel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GainPanAudioProcessorEditor)
};
