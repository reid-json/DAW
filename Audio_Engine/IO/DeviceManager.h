#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include "InputNode.h"
#include "OutputNode.h"
#include "../Arrangement/ArrangementState.h"
#include "../MIDI/MidiInputNode.h"
#include "../Recording/PlaybackManager.h"
#include "../Recording/RecordingManager.h"
#include "../Tracks/TrackInputRouter.h"
#include <vector>

class DeviceManager : public juce::AudioIODeviceCallback
{
public:
    DeviceManager();
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
    
    void setTracks(std::vector<std::unique_ptr<TrackInputRouter>>* t) { tracks = t; }
    void setArrangementState(const ArrangementState* state) { arrangementState = state; }
    void setRecordingManager(RecordingManager* manager) { recordingManager = manager; }
    void setPlaybackManager(PlaybackManager* manager) { playbackManager = manager; }
    void setInputMonitoringEnabled(bool shouldMonitor) { inputMonitoringEnabled = shouldMonitor; }
    InputNode& getInputNode() { return inputNode; }
    
    juce::MidiMessageCollector& getMidiCollector() { return midiCollector; }

private:
    void prepareForDevice(juce::AudioIODevice& device);

    InputNode inputNode;
    OutputNode outputNode;
    MidiInputNode midiInputNode;
    std::vector<std::unique_ptr<TrackInputRouter>>* tracks = nullptr;
    std::vector<float> processedLeft;
    std::vector<float> processedRight;
    std::vector<float> mixLeft;
    std::vector<float> mixRight;
    std::vector<float> tempLeft;
    std::vector<float> tempRight;
    juce::MidiMessageCollector midiCollector;
    double sampleRate = 44100.0;
    juce::AudioDeviceManager* deviceManagerPtr = nullptr;
    const ArrangementState* arrangementState = nullptr;
    PlaybackManager* playbackManager = nullptr;
    RecordingManager* recordingManager = nullptr;
    bool inputMonitoringEnabled = true;
};
