#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "dawstate.h"

class TimelineComponent : public juce::Component
{
public:
    explicit TimelineComponent (DAWState& stateIn);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

private:
    static constexpr float pixelsPerSecond = 100.0f;
    static constexpr float leftInset = 40.0f;

    DAWState& state;

    float getPlayheadX() const;
    float getMaxHorizontalScroll() const;
    void setHorizontalScrollOffset(int newOffset);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimelineComponent)
};
