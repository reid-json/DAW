#pragma once

#include <algorithm>
#include <vector>
#include <juce_core/juce_core.h>

struct TrackState
{
    juce::String name { "Audio Track" };
    juce::StringArray plugins;
    float volumeDb { -6.0f };
    float pan { 0.0f }; // -1.0 left, 0 center, 1.0 right
    bool armed { false };
    bool muted { false };
};

struct DAWState
{
    enum class TransportState
    {
        stopped,
        playing,
        paused
    };

    TransportState transportState = TransportState::stopped;

    int sidebarWidth = 300;
    int clipSidebarWidth = 220;

    double playhead = 0.0;
    float masterVolumeDb { -6.0f };
    float masterLevel = 0.0f;
    int selectedTrackIndex = 0;

    bool isRecording = false;
    bool clipSidebarCollapsed = false;
    bool audioMonitoringEnabled = false;

    std::vector<TrackState> tracks;
    std::vector<juce::String> recordedClips;
    int nextRecordedClipNumber = 1;

    DAWState()
    {
        addTrack();
        addTrack();
    }

    int getNumTracks() const
    {
        return static_cast<int> (tracks.size());
    }

    const TrackState& getTrack (int index) const
    {
        jassert (index >= 0 && index < getNumTracks());
        return tracks[(size_t) index];
    }

    TrackState& getTrack (int index)
    {
        jassert (index >= 0 && index < getNumTracks());
        return tracks[(size_t) index];
    }

    void play()
    {
        transportState = TransportState::playing;
    }

    void stop()
    {
        const bool wasRecording = isRecording;

        transportState = TransportState::stopped;
        playhead = 0.0;
        isRecording = false;

        if (wasRecording)
            addRecordedClip();
    }

    void pause()
    {
        transportState = TransportState::paused;
    }

    void restart()
    {
        transportState = TransportState::stopped;
        playhead = 0.0;
    }

    void toggleRecord()
    {
        if (isRecording)
        {
            isRecording = false;
            addRecordedClip();
            transportState = TransportState::stopped;
            playhead = 0.0;
            return;
        }

        isRecording = true;

        if (transportState == TransportState::stopped)
            transportState = TransportState::playing;
    }

    void toggleAudioMonitoring()
    {
        audioMonitoringEnabled = ! audioMonitoringEnabled;
    }

    void addTrack()
    {
        TrackState track;
        track.name = "Track " + juce::String (getNumTracks() + 1);

        // temp/testing plugins
        if (getNumTracks() == 0)
            track.plugins.add ("Compressor");
        else if (getNumTracks() == 1)
        {
            track.plugins.add ("EQ");
            track.plugins.add ("Reverb");
        }

        tracks.push_back (track);
    }

    void removeTrack()
    {
        if (getNumTracks() > 1)
            tracks.pop_back();
    }

    void removeTrackAt (int index)
    {
        if (getNumTracks() <= 1)
            return;

        if (index < 0 || index >= getNumTracks())
            return;

        tracks.erase (tracks.begin() + index);

        if (selectedTrackIndex >= getNumTracks())
            selectedTrackIndex = getNumTracks() - 1;

        if (selectedTrackIndex < 0)
            selectedTrackIndex = 0;
    }

    void toggleClipSidebar()
    {
        clipSidebarCollapsed = ! clipSidebarCollapsed;
    }

    void addRecordedClip()
    {
        recordedClips.push_back ("Recorded Clip " + juce::String (nextRecordedClipNumber++) + ".wav");
    }

    void tick (double dt)
    {
        if (transportState == TransportState::playing)
        {
            playhead += dt;

            constexpr double visibleLengthSeconds = 10.0;
            if (playhead > visibleLengthSeconds)
                playhead = 0.0;
        }
    }
};