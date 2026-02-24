#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
/*This is where you add the header files to connect the modules to the main app so it will pull the code here*/
/*Right now the only one i did was the gui to see if it actually worked, will add more as the project goes on*/
#include "../gui/GuiComponent.h"
/*JIVE components*/
#include <jive_layouts/jive_layouts.h>
#include <jive_style_sheets/jive_style_sheets.h>


class MainComponent  : public juce::Component
{
public:

    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    //declare JIVE interpreter
    jive::Interpreter interpreter;
    std::unique_ptr<jive::View> view;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
