#include "arrangementcomponent.h"

ArrangementComponent::ArrangementComponent(DAWState& stateIn)
    : state(stateIn)
{
}

void ArrangementComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.fillAll(juce::Colour::fromRGB(15, 20, 30));

    g.setColour(juce::Colour::fromRGBA(255, 255, 255, 18));
    for (int x = 0; x < getWidth(); x += 40)
        g.drawVerticalLine(x, 0.0f, static_cast<float>(getHeight()));

    for (int i = 0; i < state.trackCount; ++i)
    {
        const float y = static_cast<float>(i * (rowHeight + rowGap));
        g.setColour(juce::Colour::fromRGBA(255, 255, 255, 10));
        g.fillRoundedRectangle(8.0f, y + 4.0f, bounds.getWidth() - 16.0f, static_cast<float>(rowHeight), 8.0f);
    }

    for (const auto& clip : state.timelineClips)
    {
        const auto clipBounds = getClipBounds(clip);
        auto colour = juce::Colour::fromRGB(58, 122, 254);
        g.setColour(colour);
        g.fillRoundedRectangle(clipBounds, 8.0f);

        const auto deleteBounds = getDeleteButtonBounds(clip);
        g.setColour(juce::Colour::fromRGBA(12, 17, 25, 190));
        g.fillEllipse(deleteBounds);

        g.setColour(juce::Colours::white);
        g.drawText("x",
                   deleteBounds.toNearestInt(),
                   juce::Justification::centred,
                   false);

        g.drawText(clip.name,
                   clipBounds.toNearestInt().reduced(10, 6).withTrimmedLeft(static_cast<int>(deleteButtonSize) + 10),
                   juce::Justification::centredLeft,
                   true);
    }

    const auto playheadX = juce::jlimit(0.0f, bounds.getWidth(), getPlayheadX());
    g.setColour(juce::Colour::fromRGB(58, 122, 254));
    g.drawLine(playheadX, 0.0f, playheadX, bounds.getHeight(), 2.0f);
}

void ArrangementComponent::mouseDown(const juce::MouseEvent& e)
{
    if (auto* clip = findClipAt(e.position))
    {
        if (getDeleteButtonBounds(*clip).contains(e.position))
        {
            if (onTimelineClipDeleteRequested)
                onTimelineClipDeleteRequested(clip->placementId);
            return;
        }

        draggingPlacementId = clip->placementId;
        const auto clipBounds = getClipBounds(*clip);
        dragOffset = e.position - clipBounds.getPosition();
    }
}

void ArrangementComponent::mouseDrag(const juce::MouseEvent&)
{
}

void ArrangementComponent::mouseUp(const juce::MouseEvent& e)
{
    if (draggingPlacementId < 0)
        return;

    if (onTimelineClipMoved)
        onTimelineClipMoved(draggingPlacementId, getTrackIndexAt(e.position), getStartSecondsAt(e.position - dragOffset));

    draggingPlacementId = -1;
}

bool ArrangementComponent::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
    return dragSourceDetails.description.toString().startsWith("recent:");
}

void ArrangementComponent::itemDropped(const SourceDetails& dragSourceDetails)
{
    const auto description = dragSourceDetails.description.toString();
    if (!description.startsWith("recent:"))
        return;

    const int assetId = description.fromFirstOccurrenceOf("recent:", false, false).getIntValue();
    const auto dropPoint = dragSourceDetails.localPosition.toFloat();
    if (onRecentClipDropped)
        onRecentClipDropped(assetId, getTrackIndexAt(dropPoint), getStartSecondsAt(dropPoint));
}

int ArrangementComponent::getTrackIndexAt(juce::Point<float> point) const
{
    return juce::jlimit(0, juce::jmax(0, state.trackCount - 1), static_cast<int>(point.y) / (rowHeight + rowGap));
}

double ArrangementComponent::getStartSecondsAt(juce::Point<float> point) const
{
    return juce::jmax(0.0, static_cast<double>(point.x - clipLeftInset) / pixelsPerSecond);
}

TimelineClipItem* ArrangementComponent::findClipAt(juce::Point<float> point)
{
    for (auto& clip : state.timelineClips)
    {
        if (getClipBounds(clip).contains(point))
            return &clip;
    }

    return nullptr;
}

juce::Rectangle<float> ArrangementComponent::getClipBounds(const TimelineClipItem& clip) const
{
    const float y = static_cast<float>(clip.trackIndex * (rowHeight + rowGap)) + clipTopInset;
    const float clipX = clipLeftInset + static_cast<float>(clip.startSeconds * pixelsPerSecond);
    const float clipWidth = static_cast<float>(clip.lengthSeconds * pixelsPerSecond);
    return { clipX, y, clipWidth, clipHeight };
}

juce::Rectangle<float> ArrangementComponent::getDeleteButtonBounds(const TimelineClipItem& clip) const
{
    const auto clipBounds = getClipBounds(clip);
    return { clipBounds.getX() + 8.0f,
             clipBounds.getY() + 6.0f,
             deleteButtonSize,
             deleteButtonSize };
}

float ArrangementComponent::getPlayheadX() const
{
    return clipLeftInset + static_cast<float>(state.playhead * pixelsPerSecond);
}
