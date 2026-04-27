#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>

namespace
{
    const juce::uint32 settingsBackgroundColour = 0xff18181b;
    const juce::uint32 settingsTextColour = 0xffffffff;

    class SettingsComboBoxLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        void setAccentColour (juce::Colour newAccentColour)
        {
            accentColour = newAccentColour;
        }

        void drawComboBox (juce::Graphics& g,
                           int width,
                           int height,
                           bool,
                           int,
                           int,
                           int,
                           int,
                           juce::ComboBox& box) override
        {
            auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);
            constexpr float corner = 8.0f;

            g.setColour (juce::Colours::white.withAlpha (0.98f));
            g.fillRoundedRectangle (bounds, corner);

            auto inner = bounds.reduced (2.0f);
            g.setColour (accentColour);
            g.fillRoundedRectangle (inner, corner - 2.0f);

            juce::Path arrow;
            const auto arrowCentreX = (float) width - 18.0f;
            const auto arrowCentreY = (float) height * 0.5f;
            arrow.startNewSubPath (arrowCentreX - 5.0f, arrowCentreY - 2.0f);
            arrow.lineTo (arrowCentreX, arrowCentreY + 3.0f);
            arrow.lineTo (arrowCentreX + 5.0f, arrowCentreY - 2.0f);

            g.setColour (juce::Colours::white);
            g.strokePath (arrow, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        juce::Font getComboBoxFont (juce::ComboBox&) override
        {
            return juce::Font (13.0f, juce::Font::bold);
        }

        juce::Label* createComboBoxTextBox (juce::ComboBox& box) override
        {
            auto* label = juce::LookAndFeel_V4::createComboBoxTextBox (box);
            label->setFont (getComboBoxFont (box));
            label->setColour (juce::Label::textColourId, juce::Colours::white);
            label->setJustificationType (juce::Justification::centredLeft);
            return label;
        }

        void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
        {
            label.setBounds (6, 1, box.getWidth() - 30, box.getHeight() - 2);
            label.setFont (getComboBoxFont (box));
        }

        void drawPopupMenuBackground (juce::Graphics& g, int width, int height) override
        {
            g.fillAll (juce::Colour (settingsBackgroundColour));
            g.setColour (juce::Colours::white.withAlpha (0.18f));
            g.drawRect (0, 0, width, height, 1);
        }

        void drawPopupMenuItem (juce::Graphics& g,
                                const juce::Rectangle<int>& area,
                                bool isSeparator,
                                bool isActive,
                                bool isHighlighted,
                                bool isTicked,
                                bool,
                                const juce::String& text,
                                const juce::String&,
                                const juce::Drawable*,
                                const juce::Colour*) override
        {
            if (isSeparator)
            {
                g.setColour (juce::Colours::white.withAlpha (0.14f));
                g.fillRect (area.reduced (8, area.getHeight() / 2).withHeight (1));
                return;
            }

            auto itemArea = area.reduced (4, 2).toFloat();
            if (isHighlighted && isActive)
            {
                g.setColour (juce::Colours::white.withAlpha (0.98f));
                g.fillRoundedRectangle (itemArea, 7.0f);
                g.setColour (accentColour);
                g.fillRoundedRectangle (itemArea.reduced (2.0f), 5.0f);
            }

            g.setColour (isActive ? juce::Colours::white : juce::Colours::white.withAlpha (0.45f));
            g.setFont (juce::Font (13.0f, juce::Font::bold));
            g.drawText (text, area.reduced (14, 0), juce::Justification::centredLeft, true);

            if (isTicked)
            {
                g.setFont (juce::Font (13.0f, juce::Font::bold));
                auto tickArea = area.withTrimmedLeft (juce::jmax (0, area.getWidth() - 18));
                g.drawText ("*", tickArea, juce::Justification::centred, false);
            }
        }

    private:
        juce::Colour accentColour { juce::Colour (0xffe68000) };
    };
}

class SettingsContent : public juce::Component, private juce::ChangeListener
{
public:
    explicit SettingsContent (juce::AudioDeviceManager& dm) : deviceManager (dm)
    {
        addAndMakeVisible (outputDeviceBox);
        addAndMakeVisible (inputDeviceBox);
        addAndMakeVisible (sampleRateBox);
        addAndMakeVisible (bufferSizeBox);

        juce::ComboBox* boxes[] = { &outputDeviceBox, &inputDeviceBox, &sampleRateBox, &bufferSizeBox };
        for (auto* box : boxes)
        {
            box->setLookAndFeel (&comboBoxLookAndFeel);
            box->setColour (juce::ComboBox::textColourId, juce::Colours::white);
            box->setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
            box->setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xffe68000));
            box->setColour (juce::ComboBox::arrowColourId, juce::Colours::white);
        }

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
            double rate = sampleRateBox.getText().getDoubleValue();
            if (rate <= 0) return;
            juce::AudioDeviceManager::AudioDeviceSetup setup;
            deviceManager.getAudioDeviceSetup (setup);
            setup.sampleRate = rate;
            deviceManager.setAudioDeviceSetup (setup, true);
        };

        bufferSizeBox.onChange = [this]
        {
            int size = bufferSizeBox.getText().getIntValue();
            if (size <= 0) return;
            juce::AudioDeviceManager::AudioDeviceSetup setup;
            deviceManager.getAudioDeviceSetup (setup);
            setup.bufferSize = size;
            deviceManager.setAudioDeviceSetup (setup, true);
        };

        deviceManager.addChangeListener (this);
        refresh();
        setSize (500, 300);
    }

    void setAccentColour (juce::Colour newAccentColour)
    {
        comboBoxLookAndFeel.setAccentColour (newAccentColour);
        outputDeviceBox.setColour (juce::ComboBox::backgroundColourId, newAccentColour);
        inputDeviceBox.setColour (juce::ComboBox::backgroundColourId, newAccentColour);
        sampleRateBox.setColour (juce::ComboBox::backgroundColourId, newAccentColour);
        bufferSizeBox.setColour (juce::ComboBox::backgroundColourId, newAccentColour);
        repaint();
    }

    ~SettingsContent() override
    {
        outputDeviceBox.setLookAndFeel (nullptr);
        inputDeviceBox.setLookAndFeel (nullptr);
        sampleRateBox.setLookAndFeel (nullptr);
        bufferSizeBox.setLookAndFeel (nullptr);
        deviceManager.removeChangeListener (this);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (settingsBackgroundColour));
        g.setColour (juce::Colour (settingsTextColour));
        g.setFont (juce::Font (16.0f, juce::Font::bold));

        auto area = getLocalBounds().reduced (20, 15);

        g.drawText ("Output Device", area.removeFromTop (22), juce::Justification::centredLeft);
        area.removeFromTop (38);

        g.drawText ("Input Device", area.removeFromTop (22), juce::Justification::centredLeft);
        area.removeFromTop (38);

        g.drawText ("Sample Rate", area.removeFromTop (22), juce::Justification::centredLeft);
        area.removeFromTop (38);

        g.drawText ("Buffer Size", area.removeFromTop (22), juce::Justification::centredLeft);
        area.removeFromTop (38);
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
        area.removeFromTop (10);
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
    SettingsComboBoxLookAndFeel comboBoxLookAndFeel;
    juce::ComboBox outputDeviceBox, inputDeviceBox, sampleRateBox, bufferSizeBox;
};

class SettingsWindow : public juce::DocumentWindow
{
public:
    SettingsWindow (juce::AudioDeviceManager& deviceManager)
        : juce::DocumentWindow ("Settings",
                                juce::Colour (settingsBackgroundColour),
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

    void setAccentColour (juce::Colour newAccentColour)
    {
        if (auto* settingsContent = dynamic_cast<SettingsContent*> (getContentComponent()))
            settingsContent->setAccentColour (newAccentColour);
    }

    void closeButtonPressed() override
    {
        setVisible (false);
    }
};
