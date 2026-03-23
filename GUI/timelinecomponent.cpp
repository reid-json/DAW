#include "timelinecomponent.h"

TimelineComponent::TimelineComponent (DAWState& stateIn)
    : state (stateIn)
{
}

void TimelineComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.fillAll (juce::Colour::fromRGB (22, 27, 38));

    g.setColour (juce::Colour::fromRGBA (255, 255, 255, 25));
    constexpr int gridSpacing = 40;

    for (int x = 0; x < getWidth(); x += gridSpacing)
        g.drawVerticalLine (x, 0.0f, (float) getHeight());

    g.setColour (juce::Colour::fromRGBA (255, 255, 255, 55));
    for (int x = 0; x < getWidth(); x += gridSpacing * 4)
        g.drawVerticalLine (x, 0.0f, (float) getHeight());

    const auto playheadX = juce::jlimit (0.0f, bounds.getWidth(), getPlayheadX());

    g.setColour (juce::Colours::white);
    g.setFont (14.0f);
    g.drawText ("Time: " + juce::String (state.playhead, 2) + " s",
                12, 6, 140, 20,
                juce::Justification::left,
                false);

    g.setColour (juce::Colour::fromRGB (58, 122, 254));
    g.drawLine (playheadX, 0.0f, playheadX, bounds.getHeight(), 2.0f);

    g.fillEllipse (playheadX - 4.0f, 2.0f, 8.0f, 8.0f);
}

void TimelineComponent::resized()
{
}

float TimelineComponent::getPlayheadX() const
{
    constexpr float pixelsPerSecond = 60.0f;
    return (float) state.playhead * pixelsPerSecond;
}