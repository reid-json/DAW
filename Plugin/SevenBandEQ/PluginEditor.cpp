#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../BuiltInPluginTheme.h"

SevenBandEQAudioProcessorEditor::SevenBandEQAudioProcessorEditor(SevenBandEQAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    BuiltInPluginTheme::applyEditorTheme (*this);

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
        BuiltInPluginTheme::styleSlider (bandSliders[i]);
    }

    setSize(400, 300);
}

SevenBandEQAudioProcessorEditor::~SevenBandEQAudioProcessorEditor()
{
    BuiltInPluginTheme::clearEditorTheme (*this);
}

void SevenBandEQAudioProcessorEditor::paint(juce::Graphics& g)
{
    BuiltInPluginTheme::paintEditorBackground (g, *this);
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
