#include "tempocontrolcomponent.h"

#include <cmath>

TempoControlComponent::TempoControlComponent(DAWState& stateIn)
    : state(stateIn)
{
    captionLabel.setText("Tempo", juce::dontSendNotification);
    captionLabel.setJustificationType(juce::Justification::centredLeft);
    captionLabel.setColour(juce::Label::textColourId, juce::Colour(0xffaab4c8));
    captionLabel.setFont(juce::Font(11.0f, juce::Font::plain));
    addAndMakeVisible(captionLabel);

    tempoSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    tempoSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    tempoSlider.setRange(40.0, 240.0, 1.0);
    tempoSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff202838));
    tempoSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xff3a7afe));
    tempoSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xffdce8ff));
    tempoSlider.onValueChange = [this]
    {
        if (isSyncing)
            return;

        state.setTempoBpm(tempoSlider.getValue());
        syncFromState();
    };
    addAndMakeVisible(tempoSlider);

    tempoValueLabel.setEditable(true, true, false);
    tempoValueLabel.setJustificationType(juce::Justification::centred);
    tempoValueLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff202838));
    tempoValueLabel.setColour(juce::Label::outlineColourId, juce::Colours::white.withAlpha(0.12f));
    tempoValueLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe0e6f0));
    tempoValueLabel.onTextChange = [this] { commitLabelText(); };
    tempoValueLabel.onEditorHide = [this] { commitLabelText(); };
    addAndMakeVisible(tempoValueLabel);

    syncFromState();
}

void TempoControlComponent::resized()
{
    auto bounds = getLocalBounds();
    auto rightLabel = bounds.removeFromRight(54);
    captionLabel.setBounds(bounds.removeFromLeft(42).withTrimmedTop(6));
    bounds.removeFromLeft(8);
    tempoSlider.setBounds(bounds.withTrimmedTop(8).withHeight(24));
    tempoValueLabel.setBounds(rightLabel.withTrimmedTop(7).withHeight(22));
}

void TempoControlComponent::syncFromState()
{
    const auto tempoText = juce::String(static_cast<int>(std::round(state.tempoBpm)));

    juce::ScopedValueSetter<bool> syncSetter(isSyncing, true);
    tempoSlider.setValue(state.tempoBpm, juce::dontSendNotification);
    tempoValueLabel.setText(tempoText, juce::dontSendNotification);
}

void TempoControlComponent::commitLabelText()
{
    if (isSyncing)
        return;

    const auto trimmed = tempoValueLabel.getText().trim();
    const double parsedTempo = trimmed.getDoubleValue();
    if (parsedTempo > 0.0)
        state.setTempoBpm(parsedTempo);

    syncFromState();
}
