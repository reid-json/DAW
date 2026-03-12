#include "arrangementcomponent.h"

ArrangementComponent::ArrangementComponent (DAWState& stateIn)
    : state (stateIn)
{
}

void ArrangementComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.fillAll (juce::Colour::fromRGB (15, 20, 30));

    constexpr int rowHeight = 64;
    constexpr int rowGap = 8;
    constexpr float clipHeight = 40.0f;
    constexpr float clipYInset = 12.0f;
    constexpr float pixelsPerSecond = 100.0f;

    g.setColour (juce::Colour::fromRGBA (255, 255, 255, 18));
    for (int x = 0; x < getWidth(); x += 40)
        g.drawVerticalLine (x, 0.0f, (float) getHeight());

    for (int i = 0; i < state.trackCount; ++i)
    {
        const float y = (float) (i * (rowHeight + rowGap));

        g.setColour (juce::Colour::fromRGBA (255, 255, 255, 10));
        g.fillRoundedRectangle (8.0f, y + 4.0f, bounds.getWidth() - 16.0f, (float) rowHeight, 8.0f);

        const float clipX = 40.0f + (float) i * 70.0f;
        const float clipWidth = 160.0f + (float) i * 35.0f;

        g.setColour (juce::Colour::fromRGB (58, 122, 254));
        g.fillRoundedRectangle (clipX, y + clipYInset, clipWidth, clipHeight, 8.0f);

        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.setFont (14.0f);
        g.drawText ("Clip " + juce::String (i + 1),
                    juce::Rectangle<int> ((int) clipX + 10, (int) y + 18, (int) clipWidth - 20, 20),
                    juce::Justification::centredLeft,
                    false);
    }

    const auto playheadX = juce::jlimit (0.0f, bounds.getWidth(), getPlayheadX());
    g.setColour (juce::Colour::fromRGB (58, 122, 254));
    g.drawLine (playheadX, 0.0f, playheadX, bounds.getHeight(), 2.0f);
}

float ArrangementComponent::getPlayheadX() const
{
    constexpr float pixelsPerSecond = 100.0f;
    return (float) state.playhead * pixelsPerSecond;
}