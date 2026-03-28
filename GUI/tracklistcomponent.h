#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "dawstate.h"

class TrackListComponent : public juce::Component
{
public:
    explicit TrackListComponent(DAWState& stateIn);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    std::function<void(int trackIndex)> onRemoveTrackRequested;
    std::function<juce::StringArray()> getAvailableTrackPlugins;
    std::function<bool(int trackIndex, int slotIndex, const juce::String& pluginName)> onTrackPluginLoadRequested;
    std::function<void(int trackIndex, int slotIndex, bool shouldBeBypassed)> onTrackPluginBypassRequested;
    std::function<void(int trackIndex, int slotIndex)> onTrackPluginRemoveRequested;
    std::function<void(int trackIndex, int slotIndex)> onTrackPluginEditorRequested;
    std::function<void(int trackIndex, int slotIndex)> onTrackPluginSlotRemoveRequested;

private:
    static constexpr int rowHeight = 292;
    static constexpr int rowGap = 18;
    static constexpr int scrollbarWidth = 10;

    int getContentHeight() const;
    int getMaxScroll() const;
    int toContentY(float y) const;
    int getTrackIndexAt(juce::Point<float> point) const;
    juce::Rectangle<float> getCardInnerBounds(int trackIndex) const;
    juce::Rectangle<float> getHeaderBounds(int trackIndex) const;
    juce::Rectangle<float> getRemoveButtonBounds(int trackIndex) const;
    juce::Rectangle<float> getControlButtonsBounds(int trackIndex) const;
    juce::Rectangle<float> getButtonBounds(int trackIndex, int buttonIndex) const;
    juce::Rectangle<float> getPanKnobBounds(int trackIndex) const;
    juce::Rectangle<float> getFaderBounds(int trackIndex) const;
    juce::Rectangle<float> getPluginLaneBounds(int trackIndex) const;
    juce::Rectangle<float> getPluginSlotBounds(int trackIndex, int slotIndex) const;
    juce::Rectangle<float> getAddFxSlotBounds(int trackIndex) const;
    juce::Rectangle<float> getScrollbarTrackBounds() const;
    juce::Rectangle<float> getScrollbarThumbBounds() const;
    juce::Rectangle<float> getRowBounds(int rowIndex) const;
    void drawMixerStrip(juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& title, int trackIndex);
    void drawTransportButtons(juce::Graphics& g, juce::Rectangle<float> bounds, int trackIndex) const;
    void drawPanKnob(juce::Graphics& g, juce::Rectangle<float> bounds, int trackIndex) const;
    void drawMeter(juce::Graphics& g, juce::Rectangle<float> bounds, int trackIndex) const;
    void drawFader(juce::Graphics& g, juce::Rectangle<float> bounds, int trackIndex) const;
    void drawPluginLane(juce::Graphics& g, juce::Rectangle<float> bounds, int trackIndex) const;
    void scrollBy(float deltaY);
    void setScrollOffset(int newOffset);
    void promptRenameTrack(int trackIndex);
    void showTrackContextMenu(int trackIndex, juce::Rectangle<int> targetBounds);
    void showFxSlotMenu(int trackIndex, int slotIndex);
    void updatePanFromPosition(int trackIndex, juce::Point<float> position);
    void updateLevelFromPosition(int trackIndex, juce::Point<float> position);

    enum class DragTarget
    {
        none,
        pan,
        level
    };

    DAWState& state;
    int scrollOffset = 0;
    bool draggingScrollbar = false;
    float scrollbarDragOffset = 0.0f;
    DragTarget activeDragTarget = DragTarget::none;
    int dragTrackIndex = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackListComponent)
};
