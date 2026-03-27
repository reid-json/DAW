#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "../Arrangement/ArrangementState.h"
#include "../IO/DeviceManager.h"
#include "../Recording/PlaybackManager.h"
#include "../Recording/RecordingManager.h"
#include "../Tracks/TrackInputRouter.h"
#include <memory>
#include <vector>

class AudioEngine : public juce::MidiInputCallback
{
public:
    AudioEngine();
    ~AudioEngine();

    void initialise(int inputChannels, int outputChannels);
    void shutdown();

    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;

    juce::MidiMessageCollector& getMidiCollector() { return ioDeviceManager.getMidiCollector(); }
    juce::AudioDeviceManager& getDeviceManager() { return deviceManager; }

    void sendDiscoveryHandshake(juce::MidiOutput* output);

    bool startRecording();
    bool stopRecording();
    bool isRecording() const { return recordingManager.isRecording(); }
    double getCurrentRecordingLengthSeconds() const;

    bool startPlayback();
    bool togglePlaybackPause();
    void stopPlayback();
    bool isPlaying() const { return playbackManager.isPlaying(); }
    bool isPlaybackPaused() const { return playbackManager.isPaused(); }
    double getCurrentPlaybackPositionSeconds() const { return playbackManager.getCurrentPositionSeconds(); }
    double getCurrentTransportPositionSeconds() const;
    double getTimelineSampleRate() const;

    void setInputMonitoringEnabled(bool shouldMonitor);
    bool isInputMonitoringEnabled() const { return inputMonitoringEnabled; }

    CentralTrackSlot* addCentralTrackSlot();
    bool assignRecentAssetToCentralTrack(int assetId, int slotId);
    bool placeCentralTrackOnTimeline(int slotId, int timelineTrackId);
    bool placeRecentAssetDirectToTimeline(int assetId, int timelineTrackId);
    bool placeRecentAssetDirectToTimeline(int assetId, int timelineTrackId, juce::int64 startSample);
    bool moveTimelinePlacement(int placementId, int timelineTrackId, juce::int64 startSample);
    bool removeTimelinePlacement(int placementId);
    bool setCentralTrackMixerTrack(int slotId, int mixerTrackId);
    bool setCentralTrackTimelineTrack(int slotId, int timelineTrackId);
    bool setCentralTrackOutputToMaster(int slotId);
    bool setCentralTrackOutputToTrack(int slotId, int destinationSlotId);
    bool setCentralTrackLiveInputArmed(int slotId, bool shouldArm);

    int importAudioFileAsRecentAsset(const juce::File& file);
    int createMidiPatternAsset();
    int createLiveInputAsset(const juce::String& name);

    const ArrangementState& getArrangementState() const { return arrangementState; }

private:
    juce::AudioDeviceManager deviceManager;
    DeviceManager ioDeviceManager;
    ArrangementState arrangementState;
    PlaybackManager playbackManager;
    RecordingManager recordingManager;
    bool inputMonitoringEnabled = false;

    std::vector<std::unique_ptr<juce::MidiInput>> activeMidiInputs;
    std::vector<std::unique_ptr<juce::MidiOutput>> activeMidiOutputs;
};
