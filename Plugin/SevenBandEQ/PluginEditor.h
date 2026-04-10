#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class SevenBandEQAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    SevenBandEQAudioProcessorEditor(SevenBandEQAudioProcessor&);
    ~SevenBandEQAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    SevenBandEQAudioProcessor& audioProcessor;

    std::array<juce::Slider, 7> bandSliders;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 7> bandAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SevenBandEQAudioProcessorEditor)
};