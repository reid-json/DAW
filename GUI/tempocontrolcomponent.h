#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "dawstate.h"
#include "theme.h"

class TempoControlComponent : public juce::Component
{
public:
    TempoControlComponent(DAWState& stateIn, ThemeData& themeIn);
    ~TempoControlComponent() override;

    void resized() override;
    void refreshFromState();

private:
    void applyTheme();
    void syncFromState();
    void commitLabelText();

    DAWState& state;
    ThemeData& theme;
    juce::Label captionLabel;
    juce::Slider tempoSlider;
    juce::Label tempoValueLabel;
    std::unique_ptr<juce::LookAndFeel_V4> sliderLookAndFeel;
    bool isSyncing = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TempoControlComponent)
};
