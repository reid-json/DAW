#include "PluginProcessor.h"
#include "PluginEditor.h"

CompressorAudioProcessorEditor::CompressorAudioProcessorEditor(CompressorAudioProcessor& p, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    addAndMakeVisible(thresholdSlider);
    addAndMakeVisible(ratioSlider);
    addAndMakeVisible(gainSlider);
    addAndMakeVisible(attackSlider);
    addAndMakeVisible(releaseSlider);
    addAndMakeVisible(thresholdLabel);
    addAndMakeVisible(ratioLabel);
    addAndMakeVisible(gainLabel);
    addAndMakeVisible(attackLabel);
    addAndMakeVisible(releaseLabel);

    thresholdSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    thresholdSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    thresholdAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(vts, "threshold", thresholdSlider));
    thresholdLabel.setText("Threshold", juce::dontSendNotification);

    ratioSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    ratioSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    ratioAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(vts, "ratio", ratioSlider));
    ratioLabel.setText("Ratio", juce::dontSendNotification);

    gainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    gainAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(vts, "gain", gainSlider));
    gainLabel.setText("Gain", juce::dontSendNotification);

    attackSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    attackSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    attackAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(vts, "attack", attackSlider));
    attackLabel.setText("Attack", juce::dontSendNotification);

    releaseSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    releaseSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    releaseAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(vts, "release", releaseSlider));
    releaseLabel.setText("Release", juce::dontSendNotification);

    thresholdLabel.setJustificationType(juce::Justification::centred);
    ratioLabel.setJustificationType(juce::Justification::centred);
    gainLabel.setJustificationType(juce::Justification::centred);
    attackLabel.setJustificationType(juce::Justification::centred);
    releaseLabel.setJustificationType(juce::Justification::centred);

    setSize(500, 325);
}

CompressorAudioProcessorEditor::~CompressorAudioProcessorEditor() {}

void CompressorAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(15.0f));
}

void CompressorAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    int sliderWidth = 100;
    int sliderHeight = 100;
    int labelHeight = 20;

    int spacing = (area.getWidth() - (3 * sliderWidth)) / 4;

    int sliderY = 25;
    int labelY = sliderY + sliderHeight;

    thresholdSlider.setBounds(spacing, sliderY, sliderWidth, sliderHeight);
    thresholdLabel.setBounds(spacing, labelY, sliderWidth, labelHeight);

    int ratioX = spacing * 2 + sliderWidth;
    ratioSlider.setBounds(ratioX, sliderY, sliderWidth, sliderHeight);
    ratioLabel.setBounds(ratioX, labelY, sliderWidth, labelHeight);

    int gainX = spacing * 3 + sliderWidth * 2;
    gainSlider.setBounds(gainX, sliderY, sliderWidth, sliderHeight);
    gainLabel.setBounds(gainX, labelY, sliderWidth, labelHeight);

    attackSlider.setBounds(spacing, sliderY + 150, sliderWidth, sliderHeight);
    attackLabel.setBounds(spacing, labelY + 150, sliderWidth, labelHeight);

    releaseSlider.setBounds(ratioX, sliderY + 150, sliderWidth, sliderHeight);
    releaseLabel.setBounds(ratioX, labelY + 150, sliderWidth, labelHeight);
}
