#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "dawstate.h"

class TimelineComponent : public juce::Component
{
public:
    explicit TimelineComponent (DAWState& stateIn);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    DAWState& state;

    float getPlayheadX() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimelineComponent)
};