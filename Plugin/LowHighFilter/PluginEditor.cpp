#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../BuiltInPluginTheme.h"

LowpassHighpassFilterAudioProcessorEditor::LowpassHighpassFilterAudioProcessorEditor (LowpassHighpassFilterAudioProcessor& p, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor (&p)
{
    BuiltInPluginTheme::applyEditorTheme (*this);

    addAndMakeVisible(cutoffFrequencySlider);
    cutoffFrequencySlider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
    BuiltInPluginTheme::styleSlider (cutoffFrequencySlider);
    cutoffFrequencyAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(vts, "cutoff_frequency", cutoffFrequencySlider));
    addAndMakeVisible(cutoffFrequencyLabel);
    cutoffFrequencyLabel.setText("Cutoff Frequency", juce::dontSendNotification);
    BuiltInPluginTheme::styleLabel (cutoffFrequencyLabel);
    addAndMakeVisible(highpassButton);
    BuiltInPluginTheme::styleToggleButton (highpassButton);
    highpassAttachment.reset(new juce::AudioProcessorValueTreeState::ButtonAttachment(vts, "highpass", highpassButton));
    addAndMakeVisible(highpassButtonLabel);
    highpassButtonLabel.setText("Highpass", juce::dontSendNotification);
    BuiltInPluginTheme::styleLabel (highpassButtonLabel);
    setSize (200, 450);
}

LowpassHighpassFilterAudioProcessorEditor::~LowpassHighpassFilterAudioProcessorEditor()
{
    BuiltInPluginTheme::clearEditorTheme (*this);
}

void LowpassHighpassFilterAudioProcessorEditor::paint (juce::Graphics& g)
{
    BuiltInPluginTheme::paintEditorBackground (g, *this);
}

void LowpassHighpassFilterAudioProcessorEditor::resized()
{
    cutoffFrequencySlider.setBounds({ 15, 35, 100, 300 });
    cutoffFrequencyLabel.setBounds({ cutoffFrequencySlider.getX() + 30, cutoffFrequencySlider.getY() - 30, 200, 50 });
    highpassButton.setBounds({ cutoffFrequencySlider.getX(), cutoffFrequencySlider.getY() + cutoffFrequencySlider.getHeight() + 15, 30, 50 });
    highpassButtonLabel.setBounds({ cutoffFrequencySlider.getX() + highpassButton.getWidth() + 15, highpassButton.getY(), cutoffFrequencySlider.getWidth() - highpassButton.getWidth(), highpassButton.getHeight() });
}
