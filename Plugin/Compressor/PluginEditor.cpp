#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../BuiltInPluginTheme.h"

CompressorAudioProcessorEditor::CompressorAudioProcessorEditor(CompressorAudioProcessor& p, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    BuiltInPluginTheme::applyEditorTheme (*this);

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
    BuiltInPluginTheme::styleSlider (thresholdSlider);
    thresholdAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(vts, "threshold", thresholdSlider));
    thresholdLabel.setText("Threshold", juce::dontSendNotification);
    BuiltInPluginTheme::styleLabel (thresholdLabel);

    ratioSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    ratioSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    BuiltInPluginTheme::styleSlider (ratioSlider);
    ratioAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(vts, "ratio", ratioSlider));
    ratioLabel.setText("Ratio", juce::dontSendNotification);
    BuiltInPluginTheme::styleLabel (ratioLabel);

    gainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    BuiltInPluginTheme::styleSlider (gainSlider);
    gainAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(vts, "gain", gainSlider));
    gainLabel.setText("Gain", juce::dontSendNotification);
    BuiltInPluginTheme::styleLabel (gainLabel);

    attackSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    attackSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    BuiltInPluginTheme::styleSlider (attackSlider);
    attackAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(vts, "attack", attackSlider));
    attackLabel.setText("Attack", juce::dontSendNotification);
    BuiltInPluginTheme::styleLabel (attackLabel);

    releaseSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    releaseSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    BuiltInPluginTheme::styleSlider (releaseSlider);
    releaseAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(vts, "release", releaseSlider));
    releaseLabel.setText("Release", juce::dontSendNotification);
    BuiltInPluginTheme::styleLabel (releaseLabel);

    setSize(500, 325);
}

CompressorAudioProcessorEditor::~CompressorAudioProcessorEditor()
{
    BuiltInPluginTheme::clearEditorTheme (*this);
}

void CompressorAudioProcessorEditor::paint(juce::Graphics& g)
{
    BuiltInPluginTheme::paintEditorBackground (g, *this);
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
