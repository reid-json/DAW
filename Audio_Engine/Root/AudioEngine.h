#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "../Arrangement/ArrangementState.h"
#include "../IO/DeviceManager.h"
#include "../Recording/PlaybackManager.h"
#include "../Recording/RecordingManager.h"
#include "../Tracks/RecordedClip.h"
#include <vector>

class AudioEngine
{
public:
    AudioEngine();
    ~AudioEngine();

    void initialise(int inputChannels, int outputChannels);
    void shutdown();

    juce::AudioDeviceManager& getDeviceManager() { return deviceManager; }
    DeviceManager& getIODeviceManager() { return ioDeviceManager; }

    bool startRecording();
    bool stopRecording();
    bool isRecording() const { return recordingManager.isRecording(); }
    double getCurrentRecordingLengthSeconds() const;

    bool startPlayback();
    bool togglePlaybackPause();
    void stopPlayback();
    bool isPlaying() const { return playbackManager.isPlaying(); }
    bool isPlaybackPaused() const { return playbackManager.isPaused(); }
    double getCurrentTransportPositionSeconds() const;
    double getTimelineSampleRate() const;

    void setInputMonitoringEnabled(bool shouldMonitor);
    bool isInputMonitoringEnabled() const { return inputMonitoringEnabled; }

    bool placeRecentAssetDirectToTimeline(int assetId, int timelineTrackId, juce::int64 startSample = 0);
    bool moveTimelinePlacement(int placementId, int timelineTrackId, juce::int64 startSample);
    bool removeTimelinePlacement(int placementId);

    int importAudioFileAsRecentAsset(const juce::File& file);
    int createPatternAsset(const juce::String& name, double lengthSeconds, std::vector<PatternNote> patternNotes = {}, const juce::String& instrumentName = {});
    bool updatePatternAsset(int assetId, const juce::String& name, double lengthSeconds, std::vector<PatternNote> patternNotes = {}, const juce::String& instrumentName = {});
    bool renameAsset(int assetId, const juce::String& newName);
    void setPatternTrackRenderer(ArrangementState::PatternTrackRenderer renderer);
    void setTrackFxProcessor(ArrangementState::TrackFxProcessor processor);

    const ArrangementState& getArrangementState() const { return arrangementState; }
    ArrangementState& getArrangementState() { return arrangementState; }

private:
    RecordedClip makePatternClip(const juce::String& name, double lengthSeconds) const;

    juce::AudioDeviceManager deviceManager;
    DeviceManager ioDeviceManager;
    ArrangementState arrangementState;
    PlaybackManager playbackManager;
    RecordingManager recordingManager;
    bool inputMonitoringEnabled = false;
};
