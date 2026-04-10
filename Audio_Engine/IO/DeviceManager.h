#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include "InputNode.h"
#include "OutputNode.h"
#include "../Arrangement/ArrangementState.h"
#include "../Recording/PlaybackManager.h"
#include "../Recording/RecordingManager.h"
#include <vector>

class DeviceManager : public juce::AudioIODeviceCallback
{
public:
    DeviceManager() = default;
    ~DeviceManager() = default;

    void initialise(juce::AudioDeviceManager& deviceManager, int inputChannels, int outputChannels);
    
    void shutdown();
    
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& context) override;
    
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    
    void setArrangementState(const ArrangementState* state) { arrangementState = state; }
    void setRecordingManager(RecordingManager* manager) { recordingManager = manager; }
    void setPlaybackManager(PlaybackManager* manager) { playbackManager = manager; }
    void setInputMonitoringEnabled(bool shouldMonitor) { inputMonitoringEnabled = shouldMonitor; }

    using FxProcessCallback = std::function<void(juce::AudioBuffer<float>& buffer)>;
    void setFxProcessCallback(FxProcessCallback cb) { fxProcessCallback = std::move(cb); }

private:
    void prepareForDevice(juce::AudioIODevice& device);

    InputNode inputNode;
    OutputNode outputNode;
    std::vector<float> processedLeft;
    std::vector<float> processedRight;
    juce::AudioDeviceManager* deviceManagerPtr = nullptr;
    const ArrangementState* arrangementState = nullptr;
    PlaybackManager* playbackManager = nullptr;
    RecordingManager* recordingManager = nullptr;
    bool inputMonitoringEnabled = false;
    FxProcessCallback fxProcessCallback;
};
