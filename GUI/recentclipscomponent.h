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

private:
    int getClipIndexAt(juce::Point<float> point) const;

    DAWState& state;
    int dragClipIndex = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RecentClipsComponent)
};
