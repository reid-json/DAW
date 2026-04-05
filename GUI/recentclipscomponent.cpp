#include "recentclipscomponent.h"

#include <cmath>

namespace
{
constexpr int sectionHeaderHeight = 30;
constexpr int rowHeight = 56;
constexpr int sectionGap = 12;
constexpr int emptyRowHeight = 42;

juce::Image createDragPreviewImage(const juce::String& name, double lengthSeconds, bool isPattern)
{
    juce::Image image(juce::Image::ARGB, 220, rowHeight - 6, true);
    juce::Graphics g(image);

    auto area = image.getBounds().reduced(4);
    g.setColour(isPattern ? juce::Colour(0xff30415d) : juce::Colour(0xff223044));
    g.fillRoundedRectangle(area.toFloat(), 8.0f);

    g.setColour(juce::Colours::white);
    g.drawText(name, area.reduced(10, 8), juce::Justification::topLeft, true);

    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.drawText((isPattern ? "Pattern" : "Clip") + juce::String("  ") + juce::String(lengthSeconds, 2) + " s",
               area.reduced(10, 8).withTrimmedTop(24),
               juce::Justification::topLeft,
               false);

    return image;
}

juce::String getSectionTitle(RecentClipsComponent::ItemKind kind)
{
    return kind == RecentClipsComponent::ItemKind::clip ? "Saved Clips" : "Saved Patterns";
}
}

RecentClipsComponent::RecentClipsComponent(DAWState& stateIn)
    : state(stateIn)
{
}

juce::Rectangle<int> RecentClipsComponent::getItemBounds(ItemKind kind, int index) const
{
    const bool hasClips = ! state.recentClips.empty();
    const bool hasPatterns = ! state.savedPatterns.empty();

    int y = -scrollOffset;
    y += sectionHeaderHeight;

    if (kind == ItemKind::clip)
        return juce::Rectangle<int>(0, y + index * rowHeight, getWidth() - scrollbarWidth, rowHeight - 6).reduced(4);

    y += hasClips ? static_cast<int>(state.recentClips.size()) * rowHeight : (hasPatterns ? emptyRowHeight : 0);
    y += sectionGap + sectionHeaderHeight;
    return juce::Rectangle<int>(0, y + index * rowHeight, getWidth() - scrollbarWidth, rowHeight - 6).reduced(4);
}

void RecentClipsComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::transparentBlack);

    const bool hasClips = ! state.recentClips.empty();
    const bool hasPatterns = ! state.savedPatterns.empty();

    if (! hasClips && ! hasPatterns)
    {
        auto emptyBounds = getLocalBounds().reduced(10, 20);
        g.setColour(juce::Colours::white.withAlpha(0.92f));
        g.setFont(16.0f);
        g.drawText("No saved clips or patterns yet",
                   emptyBounds.removeFromTop(28),
                   juce::Justification::centredTop,
                   false);

        g.setColour(juce::Colours::white.withAlpha(0.55f));
        g.setFont(13.0f);
        g.drawFittedText("Record audio or draw notes in the piano roll to create reusable items.",
                         emptyBounds.removeFromTop(44),
                         juce::Justification::centredTop,
                         3);
        return;
    }

    int y = -scrollOffset;
    auto drawSection = [&] (ItemKind kind, int count, bool showInlineEmpty)
    {
        auto header = juce::Rectangle<int>(0, y, getWidth() - scrollbarWidth, sectionHeaderHeight).reduced(4, 2);
        if (header.getBottom() >= 0 && header.getY() <= getHeight())
        {
            g.setColour(juce::Colours::white.withAlpha(0.88f));
            g.setFont(14.0f);
            g.drawText(getSectionTitle(kind), header, juce::Justification::centredLeft, false);
        }

        y += sectionHeaderHeight;

        if (count == 0 && showInlineEmpty)
        {
            auto emptyArea = juce::Rectangle<int>(0, y, getWidth() - scrollbarWidth, emptyRowHeight).reduced(4);
            if (emptyArea.getBottom() >= 0 && emptyArea.getY() <= getHeight())
            {
                g.setColour(juce::Colour(0xff1b2433));
                g.fillRoundedRectangle(emptyArea.toFloat(), 8.0f);

                g.setColour(juce::Colours::white.withAlpha(0.48f));
                g.setFont(13.0f);
                g.drawText(kind == ItemKind::clip ? "No saved clips yet" : "No saved patterns yet",
                           emptyArea.reduced(10, 8),
                           juce::Justification::centredLeft,
                           false);
            }

            y += emptyRowHeight + sectionGap;
            return;
        }

        for (int i = 0; i < count; ++i)
        {
            auto area = getItemBounds(kind, i);
            if (area.getBottom() < 0 || area.getY() > getHeight())
                continue;

            const auto isPattern = kind == ItemKind::pattern;
            const auto colour = isPattern ? juce::Colour(0xff30415d) : juce::Colour(0xff223044);
            const auto& name = isPattern ? state.savedPatterns[(size_t) i].name
                                         : state.recentClips[(size_t) i].name;
            const auto length = isPattern ? state.savedPatterns[(size_t) i].lengthSeconds
                                          : state.recentClips[(size_t) i].lengthSeconds;

            g.setColour(colour);
            g.fillRoundedRectangle(area.toFloat(), 8.0f);
            g.setColour(juce::Colours::white);
            g.drawText(name, area.reduced(10, 8), juce::Justification::topLeft, true);
            g.setColour(juce::Colours::white.withAlpha(0.6f));
            g.drawText((isPattern ? "Pattern" : "Clip") + juce::String("  ") + juce::String(length, 2) + " s",
                       area.reduced(10, 8).withTrimmedTop(24), juce::Justification::topLeft, false);
        }

        y += count * rowHeight + sectionGap;
    };

    drawSection(ItemKind::clip, (int) state.recentClips.size(), hasPatterns);
    drawSection(ItemKind::pattern, (int) state.savedPatterns.size(), hasClips);

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

    dragItem = getItemAt(e.position);
}

void RecentClipsComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (const auto item = getItemAt(e.position))
        promptRenameItem(*item);
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

    if (! dragItem.has_value())
        return;

    int assetId = -1;
    juce::String name;
    double lengthSeconds = 0.0;
    const bool isPattern = dragItem->kind == ItemKind::pattern;

    if (isPattern)
    {
        if (dragItem->index < 0 || dragItem->index >= (int) state.savedPatterns.size())
            return;

        const auto& pattern = state.savedPatterns[(size_t) dragItem->index];
        assetId = pattern.assetId;
        name = pattern.name;
        lengthSeconds = pattern.lengthSeconds;
    }
    else
    {
        if (dragItem->index < 0 || dragItem->index >= (int) state.recentClips.size())
            return;

        const auto& clip = state.recentClips[(size_t) dragItem->index];
        assetId = clip.assetId;
        name = clip.name;
        lengthSeconds = clip.lengthSeconds;
    }

    if (assetId <= 0)
        return;

    if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this))
    {
        container->startDragging("recent:" + juce::String(assetId),
                                 this,
                                 createDragPreviewImage(name, lengthSeconds, isPattern),
                                 false,
                                 nullptr,
                                 &e.source);
        dragItem.reset();
    }
}

void RecentClipsComponent::mouseUp(const juce::MouseEvent&)
{
    draggingScrollbar = false;
    dragItem.reset();
}

void RecentClipsComponent::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    if (wheel.deltaY != 0.0f)
        scrollBy(-wheel.deltaY * 72.0f);
}

std::optional<RecentClipsComponent::ItemRef> RecentClipsComponent::getItemAt(juce::Point<float> point) const
{
    for (int i = 0; i < (int) state.recentClips.size(); ++i)
    {
        if (getItemBounds(ItemKind::clip, i).toFloat().contains(point))
            return ItemRef { ItemKind::clip, i };
    }

    for (int i = 0; i < (int) state.savedPatterns.size(); ++i)
    {
        if (getItemBounds(ItemKind::pattern, i).toFloat().contains(point))
            return ItemRef { ItemKind::pattern, i };
    }

    return std::nullopt;
}

int RecentClipsComponent::getContentHeight() const
{
    const bool hasClips = ! state.recentClips.empty();
    const bool hasPatterns = ! state.savedPatterns.empty();

    if (! hasClips && ! hasPatterns)
        return getHeight();

    const int clipSectionBodyHeight = hasClips ? (int) state.recentClips.size() * rowHeight
                                               : (hasPatterns ? emptyRowHeight : 0);
    const int patternSectionBodyHeight = hasPatterns ? (int) state.savedPatterns.size() * rowHeight
                                                     : (hasClips ? emptyRowHeight : 0);

    return juce::jmax(rowHeight,
                      sectionHeaderHeight + clipSectionBodyHeight
                          + sectionGap
                          + sectionHeaderHeight + patternSectionBodyHeight);
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

void RecentClipsComponent::promptRenameItem(const ItemRef& item)
{
    int assetId = -1;
    juce::String currentName;

    if (item.kind == ItemKind::clip)
    {
        if (! juce::isPositiveAndBelow(item.index, (int) state.recentClips.size()))
            return;

        assetId = state.recentClips[(size_t) item.index].assetId;
        currentName = state.recentClips[(size_t) item.index].name;
    }
    else
    {
        if (! juce::isPositiveAndBelow(item.index, (int) state.savedPatterns.size()))
            return;

        assetId = state.savedPatterns[(size_t) item.index].assetId;
        currentName = state.savedPatterns[(size_t) item.index].name;
    }

    if (assetId <= 0)
        return;

    auto* renameWindow = new juce::AlertWindow("Rename Item", "Enter a new name", juce::AlertWindow::NoIcon);
    renameWindow->addTextEditor("itemName", currentName, "Name");
    renameWindow->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    renameWindow->addButton("Rename", 1, juce::KeyPress(juce::KeyPress::returnKey));

    juce::Component::SafePointer<RecentClipsComponent> safeThis(this);
    juce::Component::SafePointer<juce::AlertWindow> safeWindow(renameWindow);
    renameWindow->enterModalState(true,
                                  juce::ModalCallbackFunction::create([safeThis, safeWindow, assetId](int result)
                                  {
                                      if (result != 1 || safeThis == nullptr || safeWindow == nullptr)
                                          return;

                                      if (auto* editor = safeWindow->getTextEditor("itemName"))
                                      {
                                          const auto renamed = editor->getText().trim();
                                          if (renamed.isNotEmpty() && safeThis->onAssetRenameRequested)
                                              safeThis->onAssetRenameRequested(assetId, renamed);
                                      }
                                  }),
                                  true);
}
