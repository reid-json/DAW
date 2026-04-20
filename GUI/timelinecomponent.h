#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "dawstate.h"
#include "theme.h"

class TimelineComponent : public juce::Component
{
public:
    TimelineComponent (DAWState& stateIn, ThemeData& themeIn);

    void paint (juce::Graphics& g) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

private:
    static constexpr float pixelsPerSecond = 100.0f;
    static constexpr float leftInset = 40.0f;

    DAWState& state;
    ThemeData& theme;

    float getPlayheadX() const;
    float getMaxHorizontalScroll() const;
    void setHorizontalScrollOffset(int newOffset);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimelineComponent)
};
