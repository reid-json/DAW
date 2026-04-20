#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "dawstate.h"
#include "sharedpopupmenulookandfeel.h"
#include "theme.h"

class ArrangementComponent : public juce::Component,
                             public juce::DragAndDropTarget
{
public:
    ArrangementComponent(DAWState& stateIn, ThemeData& themeIn);
    void setBodySpiceImage (juce::Image newImage);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;

    std::function<void(int assetId, int trackIndex, double startSeconds)> onRecentClipDropped;
    std::function<void(int placementId, int trackIndex, double startSeconds)> onTimelineClipMoved;
    std::function<void(int placementId)> onTimelineClipDeleteRequested;
    std::function<void(int assetId, const juce::String& newName)> onAssetRenameRequested;
    std::function<void(int assetId)> onPatternEditRequested;

private:
    static constexpr float pixelsPerSecond = 100.0f;
    static constexpr float clipLeftInset = 40.0f;
    static constexpr float clipTopInset = 12.0f;
    static constexpr float clipHeight = 40.0f;
    static constexpr float deleteButtonSize = 18.0f;
    static constexpr int rowHeight = 92;
    static constexpr int rowGap = 8;
    static constexpr int scrollbarWidth = 10;
    static constexpr int horizontalScrollbarHeight = 10;

    int getTrackIndexAt(juce::Point<float> point) const;
    double getStartSecondsAt(juce::Point<float> point) const;
    TimelineClipItem* findClipAt(juce::Point<float> point);
    juce::Rectangle<float> getClipBounds(const TimelineClipItem& clip) const;
    juce::Rectangle<float> getDeleteButtonBounds(const TimelineClipItem& clip) const;
    float getPlayheadX() const;
    int getContentHeight() const;
    int getMaxScroll() const;
    float toContentY(float y) const;
    juce::Rectangle<float> getScrollbarTrackBounds() const;
    juce::Rectangle<float> getScrollbarThumbBounds() const;
    juce::Rectangle<float> getHorizontalScrollbarTrackBounds() const;
    juce::Rectangle<float> getHorizontalScrollbarThumbBounds() const;
    void setScrollOffset(int newOffset);
    void scrollBy(float deltaY);
    float getMaxHorizontalScroll() const;
    void setHorizontalScrollOffset(int newOffset);
    void showClipMenu(const TimelineClipItem& clip);
    void promptRenameClip(int assetId, const juce::String& currentName);

    DAWState& state;
    ThemeData& theme;
    int draggingPlacementId = -1;
    bool draggingScrollbar = false;
    bool draggingHorizontalScrollbar = false;
    float scrollbarDragOffset = 0.0f;
    float horizontalScrollbarDragOffset = 0.0f;
    juce::Point<float> dragOffset;
    juce::Image bodySpiceImage;
    std::unique_ptr<SharedPopupMenuLookAndFeel> popupMenuLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangementComponent)
};
