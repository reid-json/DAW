#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Root/AudioEngine.h"

class Test_Audio_Engine : public juce::Component,
                          private juce::Timer
{
public:
    Test_Audio_Engine();
    ~Test_Audio_Engine() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    std::unique_ptr<AudioEngine> engine;

    juce::TextButton recordButton   { "Record" };
    juce::TextButton stopButton     { "Stop" };
    juce::TextButton playButton     { "Play" };
    juce::TextButton monitorButton  { "Monitor: OFF" };

    juce::Slider inputGainKnob;   
    juce::Slider outputGainKnob; 

    float smoothedPeak = 0.0f;
    const float attack  = 0.90f;
    const float release = 0.20f;

    float lastPeak    = 0.0f;
    bool  monitoringOn = false;
};