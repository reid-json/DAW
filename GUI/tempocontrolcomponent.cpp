#include "tempocontrolcomponent.h"

#include <cmath>

namespace
{
class MixerStyleSliderLookAndFeel : public juce::LookAndFeel_V4
{
public:
    explicit MixerStyleSliderLookAndFeel(ThemeData& themeIn) : theme(themeIn) {}

    void drawLinearSlider(juce::Graphics& g,
                          int x,
                          int y,
                          int width,
                          int height,
                          float sliderPos,
                          float,
                          float,
                          const juce::Slider::SliderStyle,
                          juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height);
        auto track = bounds.withHeight(6.0f).withCentre({ bounds.getCentreX(), bounds.getCentreY() });

        g.setColour(theme.colour("tracklist.fader.track-active", juce::Colours::white.withAlpha(0.12f)));
        g.fillRoundedRectangle(track, 3.0f);
        g.setColour(theme.colour("tracklist.fader.outline", juce::Colours::white.withAlpha(0.75f)));
        g.drawRoundedRectangle(track, 3.0f, 1.0f);

        const auto filledWidth = juce::jlimit(0.0f, track.getWidth(), sliderPos - track.getX());
        g.setColour(theme.colour("tracklist.fader.fill-active", juce::Colour(0xffe68000)));
        g.fillRoundedRectangle(track.withWidth(filledWidth), 3.0f);

        auto handle = juce::Rectangle<float>(0.0f, 0.0f, 10.0f, bounds.getHeight())
                          .withCentre({ sliderPos, bounds.getCentreY() });
        g.setColour(theme.colour("tracklist.fader.handle-active", juce::Colours::white.withAlpha(0.85f)));
        g.fillRoundedRectangle(handle, 4.0f);
        g.setColour(theme.colour("tracklist.fader.outline", juce::Colours::white.withAlpha(0.75f)));
        g.drawRoundedRectangle(handle, 4.0f, 1.0f);
    }

private:
    ThemeData& theme;
};
}

TempoControlComponent::TempoControlComponent(DAWState& stateIn, ThemeData& themeIn)
    : state(stateIn), theme(themeIn)
{
    captionLabel.setText("Tempo", juce::dontSendNotification);
    captionLabel.setJustificationType(juce::Justification::centredLeft);
    captionLabel.setFont(juce::Font(13.0f, juce::Font::bold));
    addAndMakeVisible(captionLabel);

    tempoSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    tempoSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    tempoSlider.setRange(40.0, 240.0, 1.0);
    sliderLookAndFeel = std::make_unique<MixerStyleSliderLookAndFeel>(theme);
    tempoSlider.setLookAndFeel(sliderLookAndFeel.get());
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
    tempoValueLabel.setFont(juce::Font(13.0f, juce::Font::bold));
    tempoValueLabel.onTextChange = [this] { commitLabelText(); };
    tempoValueLabel.onEditorHide = [this] { commitLabelText(); };
    addAndMakeVisible(tempoValueLabel);

    applyTheme();
    syncFromState();
}

TempoControlComponent::~TempoControlComponent()
{
    tempoSlider.setLookAndFeel(nullptr);
}

void TempoControlComponent::resized()
{
    auto bounds = getLocalBounds();
    auto rightLabel = bounds.removeFromRight(54);
    bounds.removeFromLeft(10);
    captionLabel.setBounds(bounds.removeFromLeft(50).withTrimmedTop(6));
    bounds.removeFromLeft(2);
    tempoSlider.setBounds(bounds.withTrimmedTop(8).withHeight(24));
    tempoValueLabel.setBounds(rightLabel.withTrimmedTop(7).withHeight(22));
}

void TempoControlComponent::refreshFromState()
{
    applyTheme();
    syncFromState();
}

void TempoControlComponent::applyTheme()
{
    captionLabel.setColour(juce::Label::textColourId, theme.colour("tracklist.text", theme.colour("tempo.caption.text", juce::Colours::white)));
    tempoSlider.setColour(juce::Slider::backgroundColourId, theme.colour("tempo.slider.background", juce::Colour(0xff202838)));
    tempoSlider.setColour(juce::Slider::trackColourId, theme.colour("tempo.slider.track", juce::Colour(0xff3a7afe)));
    tempoSlider.setColour(juce::Slider::thumbColourId, theme.colour("tempo.slider.thumb", juce::Colours::white));
    tempoValueLabel.setColour(juce::Label::backgroundColourId, theme.colour("tempo.value.background", juce::Colour(0xff202838)));
    tempoValueLabel.setColour(juce::Label::outlineColourId, theme.colour("tempo.value.outline", juce::Colours::white.withAlpha(0.12f)));
    tempoValueLabel.setColour(juce::Label::textColourId, theme.colour("tempo.value.text", juce::Colours::white));
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
