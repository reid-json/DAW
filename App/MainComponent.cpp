#pragma once

#include "MainComponent.h"
#include <jive_layouts/jive_layouts.h>
#include <jive_style_sheets/jive_style_sheets.h>
#include <jive_gui_basics/jive_gui_basics.h>
//==============================================================================

MainComponent::MainComponent()
{
    setSize (600, 400);
    
    view = std::make_unique<jive::View>(interpreter);

    view->setStyleSheet(BinaryData::style_css);
    view->setContent(BinaryData::layout_xml);

    addAndMakeVisible(view->getComponent());
}

MainComponent::~MainComponent()
{
}

void MainComponent::resized()
{
    // This is called when the MainComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    if (view != nullptr)
        view->getComponent().setBounds(getLocalBounds());
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
