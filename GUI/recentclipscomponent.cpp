#include "recentclipscomponent.h"
#include <cmath>

namespace
{
constexpr int rowHeight = 56;

juce::Image createDragPreviewImage(const RecentClipItem& clip)
{
    juce::Image image(juce::Image::ARGB, 220, rowHeight - 6, true);
    juce::Graphics g(image);

    auto area = image.getBounds().reduced(4);
    g.setColour(juce::Colour(0xff223044));
    g.fillRoundedRectangle(area.toFloat(), 8.0f);

    g.setColour(juce::Colours::white);
    g.drawText(clip.name, area.reduced(10, 8), juce::Justification::topLeft, true);

    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.drawText(juce::String(clip.lengthSeconds, 2) + " s",
               area.reduced(10, 8).withTrimmedTop(24),
               juce::Justification::topLeft,
               false);

    return image;
}
}

RecentClipsComponent::RecentClipsComponent(DAWState& stateIn)
    : state(stateIn)
{
}

void RecentClipsComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::transparentBlack);

    if (state.recentClips.empty())
    {
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.drawText("No clips or audio files yet", getLocalBounds().reduced(8), juce::Justification::topLeft, true);
        return;
    }

    for (int i = 0; i < static_cast<int>(state.recentClips.size()); ++i)
    {
        auto area = juce::Rectangle<int>(0, i * rowHeight - scrollOffset, getWidth() - scrollbarWidth, rowHeight - 6).reduced(4);
        if (area.getBottom() < 0 || area.getY() > getHeight())
            continue;

        g.setColour(juce::Colour(0xff223044));
        g.fillRoundedRectangle(area.toFloat(), 8.0f);
        g.setColour(juce::Colours::white);
        g.drawText(state.recentClips[static_cast<size_t>(i)].name, area.reduced(10, 8), juce::Justification::topLeft, true);
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.drawText(juce::String(state.recentClips[static_cast<size_t>(i)].lengthSeconds, 2) + " s",
                   area.reduced(10, 8).withTrimmedTop(24), juce::Justification::topLeft, false);
    }

    if (getMaxScroll() > 0)
    {
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.fillRoundedRectangle(getScrollbarTrackBounds(), 4.0f);
        g.setColour(juce::Colour(0xff4c88ff).withAlpha(0.9f));
        g.fillRoundedRectangle(getScrollbarThumbBounds(), 4.0f);
    }
}

void RecentClipsComponent::mouseDown(const juce::MouseEvent& e)
{
    if (getScrollbarThumbBounds().contains(e.position))
    {
        draggingScrollbar = true;
        scrollbarDragOffset = e.position.y - getScrollbarThumbBounds().getY();
        return;
    }

    dragClipIndex = getClipIndexAt(e.position);
}

void RecentClipsComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (draggingScrollbar)
    {
        const auto trackBounds = getScrollbarTrackBounds();
        const auto thumbBounds = getScrollbarThumbBounds();
        const float maxThumbY = trackBounds.getBottom() - thumbBounds.getHeight();
        const float thumbY = juce::jlimit(trackBounds.getY(), maxThumbY, e.position.y - scrollbarDragOffset);
        const float proportion = (thumbY - trackBounds.getY()) / juce::jmax(1.0f, maxThumbY - trackBounds.getY());
        setScrollOffset(static_cast<int>(std::round(proportion * static_cast<float>(getMaxScroll()))));
        return;
    }

    if (dragClipIndex < 0 || dragClipIndex >= static_cast<int>(state.recentClips.size()))
        return;

    if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this))
    {
        const auto& clip = state.recentClips[static_cast<size_t>(dragClipIndex)];
        container->startDragging("recent:" + juce::String(clip.assetId),
                                 this,
                                 createDragPreviewImage(clip),
                                 false,
                                 nullptr,
                                 &e.source);
        dragClipIndex = -1;
    }
}

void RecentClipsComponent::mouseUp(const juce::MouseEvent&)
{
    draggingScrollbar = false;
}

void RecentClipsComponent::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    if (wheel.deltaY != 0.0f)
        scrollBy(-wheel.deltaY * 72.0f);
}

int RecentClipsComponent::getClipIndexAt(juce::Point<float> point) const
{
    const int index = toContentY(point.y) / rowHeight;
    return index >= 0 && index < static_cast<int>(state.recentClips.size()) ? index : -1;
}

int RecentClipsComponent::getContentHeight() const
{
    return juce::jmax(rowHeight, static_cast<int>(state.recentClips.size()) * rowHeight);
}

int RecentClipsComponent::getMaxScroll() const
{
    return juce::jmax(0, getContentHeight() - getHeight());
}

int RecentClipsComponent::toContentY(float y) const
{
    return static_cast<int>(std::floor(y + static_cast<float>(scrollOffset)));
}

juce::Rectangle<float> RecentClipsComponent::getScrollbarTrackBounds() const
{
    return { static_cast<float>(getWidth() - scrollbarWidth), 2.0f, static_cast<float>(scrollbarWidth - 2), static_cast<float>(getHeight() - 4) };
}

juce::Rectangle<float> RecentClipsComponent::getScrollbarThumbBounds() const
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

void RecentClipsComponent::setScrollOffset(int newOffset)
{
    scrollOffset = juce::jlimit(0, getMaxScroll(), newOffset);
    repaint();
}

void RecentClipsComponent::scrollBy(float deltaY)
{
    setScrollOffset(scrollOffset + static_cast<int>(std::round(deltaY)));
}
