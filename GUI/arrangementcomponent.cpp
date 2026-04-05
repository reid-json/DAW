#include "arrangementcomponent.h"
#include <cmath>

ArrangementComponent::ArrangementComponent(DAWState& stateIn)
    : state(stateIn)
{
}

void ArrangementComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.fillAll(juce::Colour::fromRGB(15, 20, 30));

    g.setColour(juce::Colour::fromRGBA(255, 255, 255, 18));
    constexpr int gridSpacing = 40;
    const float gridOffset = std::fmod(clipLeftInset - static_cast<float>(state.horizontalScrollOffset), static_cast<float>(gridSpacing));
    for (float x = gridOffset; x < getWidth(); x += gridSpacing)
        if (x >= 0.0f)
            g.drawVerticalLine(juce::roundToInt(x), 0.0f, static_cast<float>(getHeight()));

    for (int i = 0; i < state.trackCount; ++i)
    {
        const float y = static_cast<float>(i * (rowHeight + rowGap) - scrollOffset);
        if (y + rowHeight < 0.0f || y > bounds.getHeight())
            continue;

        const bool isSelectedTrack = !state.isMasterMixerFocused() && state.isTrackSelected(i);
        auto rowBounds = juce::Rectangle<float>(8.0f, y + 4.0f, bounds.getWidth() - scrollbarWidth - 18.0f, static_cast<float>(rowHeight));

        g.setColour(isSelectedTrack ? juce::Colour(0xff24478d).withAlpha(0.52f)
                                    : juce::Colour::fromRGBA(255, 255, 255, 10));
        g.fillRoundedRectangle(rowBounds, 8.0f);

        if (isSelectedTrack)
        {
            g.setColour(juce::Colour(0xffd7e3ff).withAlpha(0.42f));
            g.drawRoundedRectangle(rowBounds.reduced(1.0f), 8.0f, 1.4f);
        }
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

    if (getMaxScroll() > 0)
    {
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.fillRoundedRectangle(getScrollbarTrackBounds(), 4.0f);
        g.setColour(juce::Colour(0xff4c88ff).withAlpha(0.9f));
        g.fillRoundedRectangle(getScrollbarThumbBounds(), 4.0f);
    }

    if (getMaxHorizontalScroll() > 0.0f)
    {
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.fillRoundedRectangle(getHorizontalScrollbarTrackBounds(), 4.0f);
        g.setColour(juce::Colour(0xff4c88ff).withAlpha(0.9f));
        g.fillRoundedRectangle(getHorizontalScrollbarThumbBounds(), 4.0f);
    }
}

void ArrangementComponent::mouseDown(const juce::MouseEvent& e)
{
    if (getHorizontalScrollbarThumbBounds().contains(e.position))
    {
        draggingHorizontalScrollbar = true;
        state.isDraggingHorizontalScrollbar = true;
        horizontalScrollbarDragOffset = e.position.x - getHorizontalScrollbarThumbBounds().getX();
        return;
    }

    if (getScrollbarThumbBounds().contains(e.position))
    {
        draggingScrollbar = true;
        scrollbarDragOffset = e.position.y - getScrollbarThumbBounds().getY();
        return;
    }

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

void ArrangementComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (draggingHorizontalScrollbar)
    {
        const auto trackBounds = getHorizontalScrollbarTrackBounds();
        const auto thumbBounds = getHorizontalScrollbarThumbBounds();
        const float maxThumbX = trackBounds.getRight() - thumbBounds.getWidth();
        const float thumbX = juce::jlimit(trackBounds.getX(), maxThumbX, e.position.x - horizontalScrollbarDragOffset);
        const float proportion = (thumbX - trackBounds.getX()) / juce::jmax(1.0f, maxThumbX - trackBounds.getX());
        setHorizontalScrollOffset(static_cast<int>(std::round(proportion * getMaxHorizontalScroll())));
        return;
    }

    if (!draggingScrollbar)
        return;

    const auto trackBounds = getScrollbarTrackBounds();
    const auto thumbBounds = getScrollbarThumbBounds();
    const float maxThumbY = trackBounds.getBottom() - thumbBounds.getHeight();
    const float thumbY = juce::jlimit(trackBounds.getY(), maxThumbY, e.position.y - scrollbarDragOffset);
    const float proportion = (thumbY - trackBounds.getY()) / juce::jmax(1.0f, maxThumbY - trackBounds.getY());
    setScrollOffset(static_cast<int>(std::round(proportion * static_cast<float>(getMaxScroll()))));
}

void ArrangementComponent::mouseUp(const juce::MouseEvent& e)
{
    if (draggingHorizontalScrollbar)
    {
        draggingHorizontalScrollbar = false;
        state.isDraggingHorizontalScrollbar = false;
        return;
    }

    if (draggingScrollbar)
    {
        draggingScrollbar = false;
        return;
    }

    if (draggingPlacementId < 0)
        return;

    if (onTimelineClipMoved)
        onTimelineClipMoved(draggingPlacementId, getTrackIndexAt(e.position), getStartSecondsAt(e.position - dragOffset));

    draggingPlacementId = -1;
}

void ArrangementComponent::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    if (std::abs(wheel.deltaX) > 0.0f)
    {
        setHorizontalScrollOffset(state.horizontalScrollOffset - static_cast<int>(std::round(wheel.deltaX * 96.0f)));
    }
    else if (juce::ModifierKeys::currentModifiers.isShiftDown() && wheel.deltaY != 0.0f)
    {
        setHorizontalScrollOffset(state.horizontalScrollOffset - static_cast<int>(std::round(wheel.deltaY * 96.0f)));
    }
    else if (wheel.deltaY != 0.0f)
    {
        scrollBy(-wheel.deltaY * 72.0f);
    }

    if (auto* top = getTopLevelComponent())
        top->repaint();
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
    return juce::jlimit(0, juce::jmax(0, state.trackCount - 1), static_cast<int>(toContentY(point.y)) / (rowHeight + rowGap));
}

double ArrangementComponent::getStartSecondsAt(juce::Point<float> point) const
{
    return juce::jmax(0.0, static_cast<double>(point.x + static_cast<float>(state.horizontalScrollOffset) - clipLeftInset) / pixelsPerSecond);
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
    const float y = static_cast<float>(clip.trackIndex * (rowHeight + rowGap)) + clipTopInset - static_cast<float>(scrollOffset);
    const float clipX = clipLeftInset + static_cast<float>(clip.startSeconds * pixelsPerSecond) - static_cast<float>(state.horizontalScrollOffset);
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
    return clipLeftInset + static_cast<float>(state.playhead * pixelsPerSecond) - static_cast<float>(state.horizontalScrollOffset);
}

int ArrangementComponent::getContentHeight() const
{
    return juce::jmax(rowHeight + 24, state.trackCount * rowHeight + juce::jmax(0, state.trackCount - 1) * rowGap + 24);
}

int ArrangementComponent::getMaxScroll() const
{
    return juce::jmax(0, getContentHeight() - getHeight());
}

float ArrangementComponent::toContentY(float y) const
{
    return y + static_cast<float>(scrollOffset);
}

juce::Rectangle<float> ArrangementComponent::getScrollbarTrackBounds() const
{
    return { static_cast<float>(getWidth() - scrollbarWidth), 2.0f, static_cast<float>(scrollbarWidth - 2), static_cast<float>(getHeight() - horizontalScrollbarHeight - 6) };
}

juce::Rectangle<float> ArrangementComponent::getScrollbarThumbBounds() const
{
    const auto trackBounds = getScrollbarTrackBounds();
    if (getMaxScroll() <= 0)
        return {};

    const float visibleRatio = static_cast<float>(getHeight()) / static_cast<float>(getContentHeight());
    const float thumbHeight = juce::jlimit(28.0f, trackBounds.getHeight(), trackBounds.getHeight() * visibleRatio);
    const float scrollRatio = static_cast<float>(scrollOffset) / static_cast<float>(getMaxScroll());
    const float thumbY = trackBounds.getY() + (trackBounds.getHeight() - thumbHeight) * scrollRatio;
    return { trackBounds.getX(), thumbY, trackBounds.getWidth(), thumbHeight };
}

void ArrangementComponent::setScrollOffset(int newOffset)
{
    scrollOffset = juce::jlimit(0, getMaxScroll(), newOffset);
    repaint();
}

void ArrangementComponent::scrollBy(float deltaY)
{
    setScrollOffset(scrollOffset + static_cast<int>(std::round(deltaY)));
}

float ArrangementComponent::getMaxHorizontalScroll() const
{
    double visibleDurationSeconds = 10.0;
    for (const auto& clip : state.timelineClips)
        visibleDurationSeconds = juce::jmax (visibleDurationSeconds, clip.startSeconds + clip.lengthSeconds + 2.0);

    visibleDurationSeconds = juce::jmax (visibleDurationSeconds, state.playhead + 4.0);
    const float contentWidth = clipLeftInset + static_cast<float> (visibleDurationSeconds * pixelsPerSecond) + 80.0f;
    return juce::jmax (0.0f, contentWidth - static_cast<float> (getWidth() - scrollbarWidth));
}

void ArrangementComponent::setHorizontalScrollOffset(int newOffset)
{
    state.horizontalScrollOffset = juce::jlimit (0, static_cast<int> (std::ceil (getMaxHorizontalScroll())), newOffset);
    if (auto* top = getTopLevelComponent())
        top->repaint();
    else
        repaint();
}

juce::Rectangle<float> ArrangementComponent::getHorizontalScrollbarTrackBounds() const
{
    return { 2.0f,
             static_cast<float>(getHeight() - horizontalScrollbarHeight),
             static_cast<float>(getWidth() - scrollbarWidth - 6),
             static_cast<float>(horizontalScrollbarHeight - 2) };
}

juce::Rectangle<float> ArrangementComponent::getHorizontalScrollbarThumbBounds() const
{
    const auto trackBounds = getHorizontalScrollbarTrackBounds();
    if (getMaxHorizontalScroll() <= 0.0f)
        return {};

    const float contentWidth = static_cast<float>(getWidth() - scrollbarWidth) + getMaxHorizontalScroll();
    const float visibleRatio = static_cast<float>(getWidth() - scrollbarWidth) / juce::jmax(1.0f, contentWidth);
    const float thumbWidth = juce::jlimit(40.0f, trackBounds.getWidth(), trackBounds.getWidth() * visibleRatio);
    const float scrollRatio = static_cast<float>(state.horizontalScrollOffset) / juce::jmax(1.0f, getMaxHorizontalScroll());
    const float thumbX = trackBounds.getX() + (trackBounds.getWidth() - thumbWidth) * scrollRatio;
    return { thumbX, trackBounds.getY(), thumbWidth, trackBounds.getHeight() };
}
