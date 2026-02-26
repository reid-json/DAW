#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <jive_layouts/jive_layouts.h>

class GUIComponent : public juce::Component
{
public:
    GUIComponent();
    void resized() override;

private:
    jive::Interpreter viewInterpreter;
    std::unique_ptr<jive::GuiItem> root;
};
