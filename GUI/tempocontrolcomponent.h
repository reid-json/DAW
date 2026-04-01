#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "dawstate.h"

class TempoControlComponent : public juce::Component
{
public:
    explicit TempoControlComponent(DAWState& stateIn);

    void resized() override;

private:
    void syncFromState();
    void commitLabelText();

    DAWState& state;
    juce::Label captionLabel;
    juce::Slider tempoSlider;
    juce::Label tempoValueLabel;
    bool isSyncing = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TempoControlComponent)
};
