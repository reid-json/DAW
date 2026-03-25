#include "PluginProcessor.h"
#include "PluginEditor.h"

LowpassHighpassFilterAudioProcessorEditor::LowpassHighpassFilterAudioProcessorEditor (LowpassHighpassFilterAudioProcessor& p, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor (&p)
{
    addAndMakeVisible(cutoffFrequencySlider);
    cutoffFrequencySlider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
    cutoffFrequencyAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(vts, "cutoff_frequency", cutoffFrequencySlider));
    addAndMakeVisible(cutoffFrequencyLabel);
    cutoffFrequencyLabel.setText("Cutoff Frequency", juce::dontSendNotification);
    addAndMakeVisible(highpassButton);
    highpassAttachment.reset(new juce::AudioProcessorValueTreeState::ButtonAttachment(vts, "highpass", highpassButton));
    addAndMakeVisible(highpassButtonLabel);
    highpassButtonLabel.setText("Highpass", juce::dontSendNotification);
    setSize (200, 450);
}

LowpassHighpassFilterAudioProcessorEditor::~LowpassHighpassFilterAudioProcessorEditor()
{
}

void LowpassHighpassFilterAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xffFF8C42));
    g.setFont (juce::FontOptions (15.0f));
}

void LowpassHighpassFilterAudioProcessorEditor::resized()
{
    cutoffFrequencySlider.setBounds({ 15, 35, 100, 300 });
    cutoffFrequencyLabel.setBounds({ cutoffFrequencySlider.getX() + 30, cutoffFrequencySlider.getY() - 30, 200, 50 });
    highpassButton.setBounds({ cutoffFrequencySlider.getX(), cutoffFrequencySlider.getY() + cutoffFrequencySlider.getHeight() + 15, 30, 50 });
    highpassButtonLabel.setBounds({ cutoffFrequencySlider.getX() + highpassButton.getWidth() + 15, highpassButton.getY(), cutoffFrequencySlider.getWidth() - highpassButton.getWidth(), highpassButton.getHeight() });
}
