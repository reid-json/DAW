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
    std::function<void(int trackIndex)> onTrackSelected;
    std::function<void()> onMixerFocusChanged;
    std::function<juce::StringArray()> getAvailableTrackPlugins;
    std::function<juce::StringArray()> getAvailableTrackInputs;
    std::function<juce::StringArray()> getAvailableTrackOutputs;
    std::function<juce::String()> getTrackInputDeviceName;
    std::function<juce::String()> getTrackOutputDeviceName;
    std::function<bool(int trackIndex, int slotIndex, const juce::String& pluginName)> onTrackPluginLoadRequested;
    std::function<void(int trackIndex, int slotIndex, bool shouldBeBypassed)> onTrackPluginBypassRequested;
    std::function<void(int trackIndex, int slotIndex)> onTrackPluginRemoveRequested;
    std::function<void(int trackIndex, int slotIndex)> onTrackPluginEditorRequested;
    std::function<void(int trackIndex, int slotIndex)> onTrackPluginSlotRemoveRequested;

private:
    static constexpr int rowHeight = 92;
    static constexpr int rowGap = 12;
    static constexpr int scrollbarWidth = 10;

    int getContentHeight() const;
    int getMaxScroll() const;
    int toContentY(float y) const;
    int getVisualRowCount() const;
    int getVisualRowAt(juce::Point<float> point) const;
    bool isMasterRow(int rowIndex) const;
    int getTrackIndexForRow(int rowIndex) const;
    juce::Rectangle<float> getCardInnerBounds(int rowIndex) const;
    juce::Rectangle<float> getHeaderBounds(int rowIndex) const;
    juce::Rectangle<float> getContentTypeBadgeBounds(int trackIndex) const;
    juce::Rectangle<float> getRemoveButtonBounds(int trackIndex) const;
    juce::Rectangle<float> getStatusIndicatorBounds(int rowIndex, int indicatorIndex) const;
    juce::Rectangle<float> getCompactMeterBounds(int rowIndex) const;
    juce::Rectangle<float> getCompactLevelBounds(int rowIndex) const;
    juce::Rectangle<float> getScrollbarTrackBounds() const;
    juce::Rectangle<float> getScrollbarThumbBounds() const;
    juce::Rectangle<float> getRowBounds(int rowIndex) const;
    void drawMixerStrip(juce::Graphics& g, juce::Rectangle<float> bounds, int rowIndex);
    void drawStatusIndicators(juce::Graphics& g, juce::Rectangle<float> bounds, int trackIndex) const;
    void drawCompactMeter(juce::Graphics& g, juce::Rectangle<float> bounds, float level, bool isActive) const;
    void scrollBy(float deltaY);
    void setScrollOffset(int newOffset);
    void promptRenameTrack(int trackIndex);
    void showTrackContextMenu(int trackIndex, juce::Rectangle<int> targetBounds);

    DAWState& state;
    int scrollOffset = 0;
    bool draggingScrollbar = false;
    float scrollbarDragOffset = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackListComponent)
};
