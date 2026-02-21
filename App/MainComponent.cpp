#pragma once

#include "MainComponent.h"
#include <jive_layouts/jive_layouts.h>
#include <jive_style_sheets/jive_style_sheets.h>
//==============================================================================

MainComponent::MainComponent()
{
    setSize (600, 400);
    // jive test
    using namespace jive;
    juce::Label testLabel;
    addAndMakeVisible(testLabel);
    testLabel.setText("JIVE works!", juce::dontSendNotification);
    testLabel.setColour(juce::Label::backgroundColourId, juce::Colours::orange);
    testLabel.setBounds(50, 50, 200, 50);
}

MainComponent::~MainComponent()
{
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setFont (juce::FontOptions (16.0f));
    g.setColour (juce::Colours::white);
    g.drawText ("Hello World!", getLocalBounds(), juce::Justification::centred, true);
}

void MainComponent::resized()
{
    // This is called when the MainComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
}
