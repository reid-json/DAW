#pragma once

#include <algorithm>
#include <vector>

struct DAWState
{
    enum class TransportState
    {
        stopped,
        playing,
        paused
    };

    TransportState transportState = TransportState::stopped;

    int trackCount = 1;
    int sidebarWidth = 240;
    int clipSidebarWidth = 220;

    double playhead = 0.0;
    double volumeDb = -6.0;

    bool isRecording = false;
    bool clipSidebarCollapsed = false;

    std::vector<juce::String> recordedClips;
    int nextRecordedClipNumber = 1;

    void play()    { transportState = TransportState::playing; }

    void stop()
    {
        const bool wasRecording = isRecording;

        transportState = TransportState::stopped;
        playhead = 0.0;
        isRecording = false;

        if (wasRecording)
            addRecordedClip();
    }

    void pause()   { transportState = TransportState::paused; }

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

    void addTrack() { ++trackCount; }

    void removeTrack()
    {
        trackCount = std::max (1, trackCount - 1);
    }

    void removeTrackAt (int)
    {
        removeTrack();
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