#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "dawstate.h"

class RecentClipsComponent : public juce::Component
{
public:
    explicit RecentClipsComponent(DAWState& stateIn);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

private:
    static constexpr int scrollbarWidth = 10;

    int getClipIndexAt(juce::Point<float> point) const;
    int getContentHeight() const;
    int getMaxScroll() const;
    int toContentY(float y) const;
    juce::Rectangle<float> getScrollbarTrackBounds() const;
    juce::Rectangle<float> getScrollbarThumbBounds() const;
    void setScrollOffset(int newOffset);
    void scrollBy(float deltaY);

    DAWState& state;
    int dragClipIndex = -1;
    int scrollOffset = 0;
    bool draggingScrollbar = false;
    float scrollbarDragOffset = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RecentClipsComponent)
};
