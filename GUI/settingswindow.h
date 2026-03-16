#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>

class SettingsWindow : public juce::DocumentWindow
{
public:
    SettingsWindow (juce::AudioDeviceManager& deviceManager)
        : juce::DocumentWindow ("Settings",
                                juce::Colours::black,
                                juce::DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar (true);
        setResizable (true, true);
        setResizeLimits (600, 400, 1200, 900);

        auto* selector = new juce::AudioDeviceSelectorComponent (
            deviceManager,
            0, 256,
            0, 256,
            true, true, true, false);

        selector->setSize (600, 400);   // important

        setContentOwned (selector, true);
        centreWithSize (600, 400);
        setVisible (true);
    }

    void closeButtonPressed() override
    {
        setVisible (false);
        setWantsKeyboardFocus (true);
    }
};