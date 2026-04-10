#include "timelinecomponent.h"
#include <cmath>

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
    const float gridOffset = std::fmod (leftInset - static_cast<float> (state.horizontalScrollOffset), static_cast<float> (gridSpacing));

    for (float x = gridOffset; x < getWidth(); x += gridSpacing)
        if (x >= 0.0f)
            g.drawVerticalLine (juce::roundToInt (x), 0.0f, (float) getHeight());

    g.setColour (juce::Colour::fromRGBA (255, 255, 255, 55));
    for (float x = gridOffset; x < getWidth(); x += gridSpacing * 4)
        if (x >= 0.0f)
            g.drawVerticalLine (juce::roundToInt (x), 0.0f, (float) getHeight());

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

void TimelineComponent::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    const float horizontalDelta = std::abs(wheel.deltaX) > 0.0f ? wheel.deltaX : wheel.deltaY;
    if (horizontalDelta == 0.0f)
        return;

    setHorizontalScrollOffset(state.horizontalScrollOffset - static_cast<int>(std::round(horizontalDelta * 96.0f)));

    if (auto* top = getTopLevelComponent())
        top->repaint();
}

float TimelineComponent::getPlayheadX() const
{
    return leftInset + (float) state.playhead * pixelsPerSecond - static_cast<float> (state.horizontalScrollOffset);
}

float TimelineComponent::getMaxHorizontalScroll() const
{
    double visibleDurationSeconds = 10.0;
    for (const auto& clip : state.timelineClips)
        visibleDurationSeconds = juce::jmax (visibleDurationSeconds, clip.startSeconds + clip.lengthSeconds + 2.0);

    visibleDurationSeconds = juce::jmax (visibleDurationSeconds, state.playhead + 4.0);
    const float contentWidth = leftInset + static_cast<float> (visibleDurationSeconds * pixelsPerSecond) + 80.0f;
    return juce::jmax (0.0f, contentWidth - static_cast<float> (getWidth()));
}

void TimelineComponent::setHorizontalScrollOffset(int newOffset)
{
    state.horizontalScrollOffset = juce::jlimit (0, static_cast<int> (std::ceil (getMaxHorizontalScroll())), newOffset);
    if (auto* top = getTopLevelComponent())
        top->repaint();
    else
        repaint();
}
