#include "GuiComponent.h"
/*basic step kinda thrown together from what i could find with the resources*/
/*really dont know if this is good, so more research into this extention and verifying setup will be needed*/
/*just put together so i could see structure working*/
GuiComponent::GuiComponent()
{
    magicState   = std::make_unique<foleys::MagicGUIState>();
    magicBuilder = std::make_unique<foleys::MagicGUIBuilder>(*magicState);

    magicBuilder->createGUI(*this);

    addAndMakeVisible(testLabel);
    testLabel.setText("GUI setupish", juce::dontSendNotification);
    testLabel.setJustificationType(juce::Justification::centred);

    setSize(600, 400);
}

void GuiComponent::resized()
{
    testLabel.setBounds(getLocalBounds());
}