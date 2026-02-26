#include "MainComponent.h"

MainComponent::MainComponent()
{
    gui = std::make_unique<GUIComponent>();
    addAndMakeVisible(gui.get());
    setSize(1000, 700);
}

void MainComponent::resized()
{
    if (gui != nullptr)
    {
        gui->setBounds(getLocalBounds());
    }
}
