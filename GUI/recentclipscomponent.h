#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "dawstate.h"

class RecentClipsComponent : public juce::Component
{
public:
    enum class ItemKind
    {
        clip,
        pattern
    };

    struct ItemRef
    {
        ItemKind kind = ItemKind::clip;
        int index = -1;
    };

    explicit RecentClipsComponent(DAWState& stateIn);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    std::function<void(int assetId, const juce::String& newName)> onAssetRenameRequested;

private:
    static constexpr int scrollbarWidth = 10;

    juce::Rectangle<int> getItemBounds(ItemKind kind, int index) const;
    std::optional<ItemRef> getItemAt(juce::Point<float> point) const;
    int getContentHeight() const;
    int getMaxScroll() const;
    juce::Rectangle<float> getScrollbarTrackBounds() const;
    juce::Rectangle<float> getScrollbarThumbBounds() const;
    void setScrollOffset(int newOffset);
    void scrollBy(float deltaY);
    void promptRenameItem(const ItemRef& item);

    DAWState& state;
    std::optional<ItemRef> dragItem;
    int scrollOffset = 0;
    bool draggingScrollbar = false;
    float scrollbarDragOffset = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RecentClipsComponent)
};
