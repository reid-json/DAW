#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "dawstate.h"

class ArrangementComponent : public juce::Component,
                             public juce::DragAndDropTarget
{
public:
    explicit ArrangementComponent(DAWState& stateIn);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;

    std::function<void(int assetId, int trackIndex, double startSeconds)> onRecentClipDropped;
    std::function<void(int placementId, int trackIndex, double startSeconds)> onTimelineClipMoved;
    std::function<void(int placementId)> onTimelineClipDeleteRequested;

private:
    static constexpr float pixelsPerSecond = 100.0f;
    static constexpr float clipLeftInset = 40.0f;
    static constexpr float clipTopInset = 12.0f;
    static constexpr float clipHeight = 40.0f;
    static constexpr float deleteButtonSize = 18.0f;
    static constexpr int rowHeight = 64;
    static constexpr int rowGap = 8;

    int getTrackIndexAt(juce::Point<float> point) const;
    double getStartSecondsAt(juce::Point<float> point) const;
    TimelineClipItem* findClipAt(juce::Point<float> point);
    juce::Rectangle<float> getClipBounds(const TimelineClipItem& clip) const;
    juce::Rectangle<float> getDeleteButtonBounds(const TimelineClipItem& clip) const;
    float getPlayheadX() const;

    DAWState& state;
    int draggingPlacementId = -1;
    juce::Point<float> dragOffset;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangementComponent)
};
