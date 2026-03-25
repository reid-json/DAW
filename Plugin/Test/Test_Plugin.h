#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class FilterTestComponent : public juce::Component
{
public:
    FilterTestComponent();
    ~FilterTestComponent() override;

    void resized() override;

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};
