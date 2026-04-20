#include "PluginProcessor.h"
#include "PluginEditor.h"

SevenBandEQAudioProcessorEditor::SevenBandEQAudioProcessorEditor(SevenBandEQAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    for (int i = 0; i < 7; ++i)
    {
        bandAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts,
            "band" + std::to_string(i) + "Gain",
            bandSliders[i]
        );

        addAndMakeVisible(bandSliders[i]);
        bandSliders[i].setSliderStyle(juce::Slider::LinearVertical);
        bandSliders[i].setTextBoxStyle(juce::Slider::TextBoxBelow, true, 50, 20);
    }

    setSize(400, 300);
}

SevenBandEQAudioProcessorEditor::~SevenBandEQAudioProcessorEditor() = default;

void SevenBandEQAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    g.setColour(juce::Colours::white);
    g.setFont(15.0f);
}

void SevenBandEQAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    int sliderWidth = area.getWidth() / 7;

    for (int i = 0; i < 7; ++i)
    {
        bandSliders[i].setBounds(i * sliderWidth, 0, sliderWidth, area.getHeight());
    }
}