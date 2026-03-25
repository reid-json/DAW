#include "recentclipscomponent.h"

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
        auto area = juce::Rectangle<int>(0, i * rowHeight, getWidth(), rowHeight - 6).reduced(4);
        g.setColour(juce::Colour(0xff223044));
        g.fillRoundedRectangle(area.toFloat(), 8.0f);
        g.setColour(juce::Colours::white);
        g.drawText(state.recentClips[static_cast<size_t>(i)].name, area.reduced(10, 8), juce::Justification::topLeft, true);
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.drawText(juce::String(state.recentClips[static_cast<size_t>(i)].lengthSeconds, 2) + " s",
                   area.reduced(10, 8).withTrimmedTop(24), juce::Justification::topLeft, false);
    }
}

void RecentClipsComponent::mouseDown(const juce::MouseEvent& e)
{
    dragClipIndex = getClipIndexAt(e.position);
}

void RecentClipsComponent::mouseDrag(const juce::MouseEvent& e)
{
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

int RecentClipsComponent::getClipIndexAt(juce::Point<float> point) const
{
    const int index = static_cast<int>(point.y) / rowHeight;
    return index >= 0 && index < static_cast<int>(state.recentClips.size()) ? index : -1;
}
