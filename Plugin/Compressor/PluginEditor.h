#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class CompressorAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    CompressorAudioProcessorEditor(CompressorAudioProcessor&, juce::AudioProcessorValueTreeState& vts);
    ~CompressorAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    CompressorAudioProcessor& audioProcessor;
    juce::Slider thresholdSlider;
    juce::Slider ratioSlider;
    juce::Slider gainSlider;
    juce::Slider attackSlider;
    juce::Slider releaseSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> thresholdAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ratioAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;
    juce::Label thresholdLabel;
    juce::Label ratioLabel;
    juce::Label gainLabel;
    juce::Label attackLabel;
    juce::Label releaseLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorAudioProcessorEditor)
};
