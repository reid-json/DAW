#pragma once
#include "guicomponent.h"

class MainComponent : public juce::Component
{
public:
    MainComponent();
    void resized() override;

private:
    std::unique_ptr<GUIComponent> gui;
};