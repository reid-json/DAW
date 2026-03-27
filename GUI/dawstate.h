#pragma once

#include <algorithm>
#include <vector>
#include <juce_core/juce_core.h>

struct RecentClipItem
{
    int assetId = 0;
    juce::String name;
    double lengthSeconds = 0.0;

    bool operator==(const RecentClipItem& other) const
    {
        return assetId == other.assetId
            && name == other.name
            && lengthSeconds == other.lengthSeconds;
    }

    bool operator!=(const RecentClipItem& other) const
    {
        return !(*this == other);
    }
};

struct TimelineClipItem
{
    int placementId = 0;
    int assetId = 0;
    juce::String name;
    int trackIndex = 0;
    double startSeconds = 0.0;
    double lengthSeconds = 0.0;

    bool operator==(const TimelineClipItem& other) const
    {
        return placementId == other.placementId
            && assetId == other.assetId
            && name == other.name
            && trackIndex == other.trackIndex
            && startSeconds == other.startSeconds
            && lengthSeconds == other.lengthSeconds;
    }

    bool operator!=(const TimelineClipItem& other) const
    {
        return !(*this == other);
    }
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

    int trackCount = 5;
    int sidebarWidth = 240;
    int clipSidebarWidth = 220;

    double playhead = 0.0;

    bool isRecording = false;
    bool clipSidebarCollapsed = false;
    bool audioMonitoringEnabled = false;

    std::vector<RecentClipItem> recentClips;
    std::vector<TimelineClipItem> timelineClips;

    void play()    { transportState = TransportState::playing; }
    void pause()   { transportState = TransportState::paused; }

    void stop()
    {
        transportState = TransportState::stopped;
        playhead = 0.0;
        isRecording = false;
    }

    void restart()
    {
        transportState = TransportState::stopped;
        playhead = 0.0;
    }

    void setRecording(bool shouldRecord)
    {
        isRecording = shouldRecord;
        transportState = shouldRecord ? TransportState::playing : TransportState::stopped;
        if (!shouldRecord)
            playhead = 0.0;
    }

    void toggleAudioMonitoring()
    {
        audioMonitoringEnabled = ! audioMonitoringEnabled;
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

    int findNextEmptyTrackIndex() const
    {
        for (int track = 0; track < trackCount; ++track)
        {
            const bool hasClip = std::any_of (timelineClips.begin(), timelineClips.end(),
                                              [track] (const TimelineClipItem& clip) { return clip.trackIndex == track; });
            if (! hasClip)
                return track;
        }

        return trackCount;
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
