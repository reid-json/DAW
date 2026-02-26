#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_basics/juce_audio_basics.h>

#include "../IO/InputRingBuffer.h"
#include "../Tracks/AudioTrack.h"
#include "../graph/DSPChain.h"  

class AudioEngine : public juce::AudioIODeviceCallback
{
public:
    AudioEngine();
    ~AudioEngine() override;

    void start();
    void stop();

    void startRecording();
    void stopRecording();

    void startPlayback();
    void stopPlayback();

    void setInputGain(float newGain);   
    void setOutputGain(float newGain);  
    void setMonitoringEnabled(bool shouldMonitor);

    const juce::AudioBuffer<float>& getLatestInput() const { return latestInput; }

   
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& context) override;

    void audioDeviceAboutToStart(juce::AudioIODevice*) override {}
    void audioDeviceStopped() override {}

private:
    void process(float* left, float* right, int numSamples);

    juce::AudioDeviceManager deviceManager;

    bool isRunning         = false;
    bool isRecording       = false;
    bool isPlaying         = false;
    bool monitoringEnabled = false;

    float inputGain  = 1.0f; 
    float outputGain = 1.0f;  

   
    juce::AudioBuffer<float> latestInput;

    
    juce::AudioBuffer<float> processedInput;

    InputRingBuffer inputBuffer { 48000 * 10 };
    AudioTrack      track;

    DSPChain        dspChain; 
};