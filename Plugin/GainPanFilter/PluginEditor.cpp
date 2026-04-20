#include "PluginProcessor.h"
#include "PluginEditor.h"

GainPanAudioProcessorEditor::GainPanAudioProcessorEditor (GainPanAudioProcessor& p, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    //add all bisible elements
    addAndMakeVisible(gainSlider);
    gainSlider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
    gainAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(vts, "gain", gainSlider));
    addAndMakeVisible(gainLabel);
    gainLabel.setText("Gain", juce::dontSendNotification);
    addAndMakeVisible(panSlider);
    panSlider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
    panAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(vts, "pan", panSlider));
    addAndMakeVisible(panLabel);
    panLabel.setText("Pan", juce::dontSendNotification);
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize(400, 350);
}

GainPanAudioProcessorEditor::~GainPanAudioProcessorEditor()
{
}

void GainPanAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));
}

void GainPanAudioProcessorEditor::resized()
{
    int sliderWidth = 100;
    int sliderHeight = 250; // slightly shorter than editor height
    int sliderTop = 70;  // top margin from editor
    int labelHeight = 20;
    int gap = 40;

    int gainSliderLeft = 75;
    int panSliderLeft = gainSliderLeft + sliderWidth + gap;

    // Set sliders
    gainSlider.setBounds(gainSliderLeft, sliderTop, sliderWidth, sliderHeight);
    panSlider.setBounds(panSliderLeft, sliderTop, sliderWidth, sliderHeight);

    // Labels directly above sliders
    gainLabel.setBounds(gainSliderLeft, sliderTop - labelHeight - 5, sliderWidth, labelHeight);
    panLabel.setBounds(panSliderLeft, sliderTop - labelHeight - 5, sliderWidth, labelHeight);
}