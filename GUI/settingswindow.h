#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>

class SettingsContent : public juce::Component, private juce::ChangeListener
{
public:
    explicit SettingsContent (juce::AudioDeviceManager& dm) : deviceManager (dm)
    {
        addAndMakeVisible (outputDeviceBox);
        addAndMakeVisible (inputDeviceBox);
        addAndMakeVisible (sampleRateBox);
        addAndMakeVisible (bufferSizeBox);

        outputDeviceBox.onChange = [this]
        {
            juce::AudioDeviceManager::AudioDeviceSetup setup;
            deviceManager.getAudioDeviceSetup (setup);
            setup.outputDeviceName = outputDeviceBox.getText();
            deviceManager.setAudioDeviceSetup (setup, true);
        };

        inputDeviceBox.onChange = [this]
        {
            juce::AudioDeviceManager::AudioDeviceSetup setup;
            deviceManager.getAudioDeviceSetup (setup);
            setup.inputDeviceName = inputDeviceBox.getText();
            deviceManager.setAudioDeviceSetup (setup, true);
        };

        sampleRateBox.onChange = [this]
        {
            auto rate = sampleRateBox.getText().getDoubleValue();
            if (rate > 0)
            {
                juce::AudioDeviceManager::AudioDeviceSetup setup;
                deviceManager.getAudioDeviceSetup (setup);
                setup.sampleRate = rate;
                deviceManager.setAudioDeviceSetup (setup, true);
            }
        };

        bufferSizeBox.onChange = [this]
        {
            auto size = bufferSizeBox.getText().getIntValue();
            if (size > 0)
            {
                juce::AudioDeviceManager::AudioDeviceSetup setup;
                deviceManager.getAudioDeviceSetup (setup);
                setup.bufferSize = size;
                deviceManager.setAudioDeviceSetup (setup, true);
            }
        };

        deviceManager.addChangeListener (this);
        refresh();
        setSize (500, 300);
    }

    ~SettingsContent() override
    {
        deviceManager.removeChangeListener (this);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff1a1a2e));

        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.setFont (14.0f);

        auto area = getLocalBounds().reduced (20, 15);
        g.drawText ("Output Device", area.removeFromTop (22), juce::Justification::centredLeft);
        area.removeFromTop (30 + 10);
        g.drawText ("Input Device", area.removeFromTop (22), juce::Justification::centredLeft);
        area.removeFromTop (30 + 10);
        g.drawText ("Sample Rate", area.removeFromTop (22), juce::Justification::centredLeft);
        area.removeFromTop (30 + 10);
        g.drawText ("Buffer Size", area.removeFromTop (22), juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (20, 15);
        area.removeFromTop (22);
        outputDeviceBox.setBounds (area.removeFromTop (28));
        area.removeFromTop (10);
        area.removeFromTop (22);
        inputDeviceBox.setBounds (area.removeFromTop (28));
        area.removeFromTop (10);
        area.removeFromTop (22);
        sampleRateBox.setBounds (area.removeFromTop (28));
        area.removeFromTop (10);
        area.removeFromTop (22);
        bufferSizeBox.setBounds (area.removeFromTop (28));
    }

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override { refresh(); }

    void refresh()
    {
        auto* type = deviceManager.getCurrentDeviceTypeObject();
        if (type == nullptr) return;

        juce::AudioDeviceManager::AudioDeviceSetup setup;
        deviceManager.getAudioDeviceSetup (setup);

        outputDeviceBox.clear (juce::dontSendNotification);
        auto outputNames = type->getDeviceNames (false);
        for (int i = 0; i < outputNames.size(); ++i)
            outputDeviceBox.addItem (outputNames[i], i + 1);
        outputDeviceBox.setText (setup.outputDeviceName, juce::dontSendNotification);

        inputDeviceBox.clear (juce::dontSendNotification);
        auto inputNames = type->getDeviceNames (true);
        for (int i = 0; i < inputNames.size(); ++i)
            inputDeviceBox.addItem (inputNames[i], i + 1);
        inputDeviceBox.setText (setup.inputDeviceName, juce::dontSendNotification);

        auto* device = deviceManager.getCurrentAudioDevice();
        if (device != nullptr)
        {
            sampleRateBox.clear (juce::dontSendNotification);
            for (auto rate : device->getAvailableSampleRates())
                sampleRateBox.addItem (juce::String (rate, 0) + " Hz", static_cast<int> (rate));
            sampleRateBox.setText (juce::String (setup.sampleRate, 0) + " Hz", juce::dontSendNotification);

            bufferSizeBox.clear (juce::dontSendNotification);
            for (auto size : device->getAvailableBufferSizes())
                bufferSizeBox.addItem (juce::String (size), size);
            bufferSizeBox.setText (juce::String (setup.bufferSize), juce::dontSendNotification);
        }
    }

    juce::AudioDeviceManager& deviceManager;
    juce::ComboBox outputDeviceBox, inputDeviceBox, sampleRateBox, bufferSizeBox;
};

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
        setResizeLimits (400, 250, 800, 500);

        auto* content = new SettingsContent (deviceManager);
        setContentOwned (content, true);
        centreWithSize (500, 300);
        setVisible (true);
    }

    void closeButtonPressed() override
    {
        setVisible (false);
    }
};
