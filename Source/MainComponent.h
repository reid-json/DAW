#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

class MainComponent : public juce::AudioAppComponent,
                      public juce::Button::Listener
{
public:
    MainComponent();
    ~MainComponent() override;

    // GUI
    void paint(juce::Graphics&) override;
    void resized() override;

    // Audio callbacks
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    // Button listener
    void buttonClicked(juce::Button* button) override;

private:
    // UI components
    juce::TextButton loadButton { "Load File" };
    juce::Label fileNameLabel;

    // Audio members
    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;

    // Audio info
    int currentBlockSize = 0;
    double currentSampleRate = 0.0;

    // Functions
    void loadFile();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};