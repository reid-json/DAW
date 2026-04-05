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

struct SavedPatternItem
{
    int assetId = 0;
    int trackIndex = 0;
    juce::String name;
    double lengthSeconds = 0.0;

    bool operator==(const SavedPatternItem& other) const
    {
        return assetId == other.assetId
            && trackIndex == other.trackIndex
            && name == other.name
            && lengthSeconds == other.lengthSeconds;
    }

    bool operator!=(const SavedPatternItem& other) const
    {
        return !(*this == other);
    }
};

struct TrackMixerState
{
    juce::String name;
    bool muted = false;
    bool soloed = false;
    bool armed = false;
    bool monitoringEnabled = false;
    float pan = 0.0f;
    float level = 0.72f;
    juce::String inputAssignment = "Input channel 1";
    juce::String outputAssignment = "Output channel 1";
    struct FxSlotState
    {
        juce::String name;
        bool hasPlugin = false;
        bool bypassed = false;
    };
    std::vector<FxSlotState> fxSlots;
};

struct TrackPatternNote
{
    int id = 0;
    double startBeat = 0.0;
    double lengthBeats = 1.0;
    int midiNote = 60;
    juce::String label = "C4";

    bool operator==(const TrackPatternNote& other) const
    {
        return id == other.id
            && startBeat == other.startBeat
            && lengthBeats == other.lengthBeats
            && midiNote == other.midiNote
            && label == other.label;
    }

    bool operator!=(const TrackPatternNote& other) const
    {
        return !(*this == other);
    }
};

struct TrackPatternState
{
    int assetId = -1;
    juce::String name = "Pattern 1";
    std::vector<TrackPatternNote> notes;

    bool operator==(const TrackPatternState& other) const
    {
        return assetId == other.assetId
            && name == other.name
            && notes == other.notes;
    }

    bool operator!=(const TrackPatternState& other) const
    {
        return !(*this == other);
    }
};

struct MasterMixerState
{
    bool muted = false;
    bool soloed = false;
    bool armed = false;
    float pan = 0.0f;
    float level = 0.8f;
    std::vector<TrackMixerState::FxSlotState> fxSlots;
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
    int sidebarWidth = 340;
    int clipSidebarWidth = 220;
    int horizontalScrollOffset = 0;
    int selectedTrackIndex = 0;
    double tempoBpm = 120.0;
    bool focusedMixerShowsMaster = true;

    double playhead = 0.0;

    bool isRecording = false;
    bool clipSidebarCollapsed = false;
    bool audioMonitoringEnabled = false;
    bool isDraggingHorizontalScrollbar = false;

    MasterMixerState masterMixerState;
    juce::String masterOutputAssignment = "Output channel 1";
    std::vector<TrackMixerState> trackMixerStates;
    std::vector<TrackPatternState> trackPatternStates;
    std::vector<RecentClipItem> recentClips;
    std::vector<SavedPatternItem> savedPatterns;
    std::vector<TimelineClipItem> timelineClips;

    DAWState()
    {
        ensureTrackMixerStateCount();
    }

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

    void setTempoBpm (double newTempoBpm)
    {
        tempoBpm = juce::jlimit (40.0, 240.0, newTempoBpm);
    }

    void addTrack()
    {
        ++trackCount;
        ensureTrackMixerStateCount();
        selectedTrackIndex = trackCount - 1;
    }

    void removeTrack()
    {
        trackCount = std::max (1, trackCount - 1);
        ensureTrackMixerStateCount();
    }

    void removeTrackAt (int trackIndex)
    {
        if (trackCount <= 1)
            return;

        const int clampedIndex = juce::jlimit (0, trackCount - 1, trackIndex);
        --trackCount;

        if (clampedIndex >= 0 && clampedIndex < static_cast<int> (trackMixerStates.size()))
            trackMixerStates.erase (trackMixerStates.begin() + clampedIndex);

        if (clampedIndex >= 0 && clampedIndex < static_cast<int> (trackPatternStates.size()))
            trackPatternStates.erase (trackPatternStates.begin() + clampedIndex);

        ensureTrackMixerStateCount();
        selectedTrackIndex = juce::jlimit (0, trackCount - 1, selectedTrackIndex > clampedIndex ? selectedTrackIndex - 1 : selectedTrackIndex);
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

    TrackMixerState& getTrackMixerState (int trackIndex)
    {
        ensureTrackMixerStateCount();
        return trackMixerStates[juce::jlimit (0, trackCount - 1, trackIndex)];
    }

    const TrackMixerState& getTrackMixerState (int trackIndex) const
    {
        return trackMixerStates[juce::jlimit (0, trackCount - 1, trackIndex)];
    }

    TrackPatternState& getTrackPatternState (int trackIndex)
    {
        ensureTrackMixerStateCount();
        return trackPatternStates[juce::jlimit (0, trackCount - 1, trackIndex)];
    }

    const TrackPatternState& getTrackPatternState (int trackIndex) const
    {
        return trackPatternStates[juce::jlimit (0, trackCount - 1, trackIndex)];
    }

    TrackPatternState& getSelectedTrackPatternState()
    {
        return getTrackPatternState (selectedTrackIndex);
    }

    const TrackPatternState& getSelectedTrackPatternState() const
    {
        return getTrackPatternState (selectedTrackIndex);
    }

    juce::String getTrackPatternName (int trackIndex) const
    {
        const auto& pattern = getTrackPatternState (trackIndex);
        return pattern.name.isNotEmpty() ? pattern.name : "Pattern 1";
    }

    void setTrackPatternName (int trackIndex, juce::String newName)
    {
        auto& pattern = getTrackPatternState (trackIndex);
        pattern.name = newName.trim();

        if (pattern.name.isEmpty())
            pattern.name = "Pattern 1";
    }

    void toggleTrackMuted (int trackIndex)
    {
        auto& track = getTrackMixerState (trackIndex);
        track.muted = ! track.muted;
    }

    void toggleTrackSoloed (int trackIndex)
    {
        auto& track = getTrackMixerState (trackIndex);
        track.soloed = ! track.soloed;
    }

    void toggleTrackArmed (int trackIndex)
    {
        auto& track = getTrackMixerState (trackIndex);
        track.armed = ! track.armed;
    }

    void toggleTrackMonitoringEnabled (int trackIndex)
    {
        auto& track = getTrackMixerState (trackIndex);
        track.monitoringEnabled = ! track.monitoringEnabled;
    }

    void armOnlyTrack (int trackIndex)
    {
        const int targetIndex = juce::jlimit (0, trackCount - 1, trackIndex);
        ensureTrackMixerStateCount();

        for (int i = 0; i < trackCount; ++i)
            trackMixerStates[static_cast<size_t> (i)].armed = (i == targetIndex);
    }

    void disarmAllTracks()
    {
        ensureTrackMixerStateCount();

        for (int i = 0; i < trackCount; ++i)
            trackMixerStates[static_cast<size_t> (i)].armed = false;
    }

    void setTrackPan (int trackIndex, float newPan)
    {
        auto& track = getTrackMixerState (trackIndex);
        track.pan = juce::jlimit (-1.0f, 1.0f, newPan);
    }

    void setTrackLevel (int trackIndex, float newLevel)
    {
        auto& track = getTrackMixerState (trackIndex);
        track.level = juce::jlimit (0.0f, 1.0f, newLevel);
    }

    void selectTrack (int trackIndex)
    {
        selectedTrackIndex = juce::jlimit (0, trackCount - 1, trackIndex);
    }

    void showMasterMixerFocus()
    {
        focusedMixerShowsMaster = true;
    }

    void showSelectedTrackMixerFocus()
    {
        focusedMixerShowsMaster = false;
        selectedTrackIndex = juce::jlimit (0, trackCount - 1, selectedTrackIndex);
    }

    bool isMasterMixerFocused() const
    {
        return focusedMixerShowsMaster;
    }

    bool isTrackSelected (int trackIndex) const
    {
        return juce::jlimit (0, trackCount - 1, selectedTrackIndex) == trackIndex;
    }

    bool anyTrackSoloed() const
    {
        return std::any_of(trackMixerStates.begin(), trackMixerStates.begin() + trackCount,
                           [] (const TrackMixerState& track) { return track.soloed; });
    }

    bool isTrackAudible (int trackIndex) const
    {
        const auto& track = getTrackMixerState (trackIndex);
        if (track.muted)
            return false;

        return ! anyTrackSoloed() || track.soloed;
    }

    juce::String getTrackName (int trackIndex) const
    {
        const auto& track = getTrackMixerState (trackIndex);
        return track.name.isNotEmpty() ? track.name : "Track " + juce::String (trackIndex + 1);
    }

    void setTrackName (int trackIndex, juce::String newName)
    {
        auto& track = getTrackMixerState (trackIndex);
        track.name = newName.trim();
        if (track.name.isEmpty())
            track.name = "Track " + juce::String (trackIndex + 1);
    }

    juce::String getTrackIoAssignment (int trackIndex) const
    {
        const auto& track = getTrackMixerState (trackIndex);
        return track.inputAssignment + " -> " + track.outputAssignment;
    }

    void setTrackIoAssignment (int trackIndex, juce::String newAssignment)
    {
        auto& track = getTrackMixerState (trackIndex);
        const auto parts = juce::StringArray::fromTokens (newAssignment, "->", "");
        if (parts.size() >= 2)
        {
            setTrackInputAssignment (trackIndex, parts[0].trim());
            setTrackOutputAssignment (trackIndex, parts[1].trim());
        }
    }

    juce::String getTrackInputAssignment (int trackIndex) const
    {
        const auto& track = getTrackMixerState (trackIndex);
        return track.inputAssignment.isNotEmpty() ? track.inputAssignment : "Input channel 1";
    }

    juce::String getTrackOutputAssignment (int trackIndex) const
    {
        const auto& track = getTrackMixerState (trackIndex);
        return track.outputAssignment.isNotEmpty() ? track.outputAssignment : "Output channel 1";
    }

    void setTrackInputAssignment (int trackIndex, juce::String newAssignment)
    {
        auto& track = getTrackMixerState (trackIndex);
        track.inputAssignment = newAssignment.trim();
        if (track.inputAssignment.isEmpty())
            track.inputAssignment = "Input channel 1";
    }

    void setTrackOutputAssignment (int trackIndex, juce::String newAssignment)
    {
        auto& track = getTrackMixerState (trackIndex);
        track.outputAssignment = newAssignment.trim();
        if (track.outputAssignment.isEmpty())
            track.outputAssignment = "Output channel 1";
    }

    int getTrackFxSlotCount (int trackIndex) const
    {
        return static_cast<int> (getTrackMixerState (trackIndex).fxSlots.size());
    }

    bool canAddTrackFxSlot (int trackIndex) const
    {
        return getTrackFxSlotCount (trackIndex) < getMaxFxSlotCount();
    }

    bool canRemoveTrackFxSlot (int trackIndex) const
    {
        return getTrackFxSlotCount (trackIndex) > getMinFxSlotCount();
    }

    void addTrackFxSlot (int trackIndex)
    {
        auto& track = getTrackMixerState (trackIndex);
        if (static_cast<int> (track.fxSlots.size()) < getMaxFxSlotCount())
            track.fxSlots.emplace_back();
    }

    void removeTrackFxSlot (int trackIndex, int slotIndex)
    {
        auto& track = getTrackMixerState (trackIndex);
        if (static_cast<int> (track.fxSlots.size()) <= getMinFxSlotCount())
            return;

        const int clampedIndex = juce::jlimit (0, static_cast<int> (track.fxSlots.size()) - 1, slotIndex);
        track.fxSlots.erase (track.fxSlots.begin() + clampedIndex);
    }

    TrackMixerState::FxSlotState& getTrackFxSlot (int trackIndex, int slotIndex)
    {
        auto& track = getTrackMixerState (trackIndex);
        ensureFxSlotCount (track.fxSlots);
        return track.fxSlots[juce::jlimit (0, static_cast<int> (track.fxSlots.size()) - 1, slotIndex)];
    }

    const TrackMixerState::FxSlotState& getTrackFxSlot (int trackIndex, int slotIndex) const
    {
        const auto& track = getTrackMixerState (trackIndex);
        return track.fxSlots[juce::jlimit (0, static_cast<int> (track.fxSlots.size()) - 1, slotIndex)];
    }

    void addDummyPluginToTrackFxSlot (int trackIndex, int slotIndex)
    {
        auto& slot = getTrackFxSlot (trackIndex, slotIndex);
        slot.hasPlugin = true;
        slot.bypassed = false;
        slot.name = "Dummy FX " + juce::String (slotIndex + 1);
    }

    void loadTrackPluginIntoFxSlot (int trackIndex, int slotIndex, const juce::String& pluginName)
    {
        auto& slot = getTrackFxSlot (trackIndex, slotIndex);
        slot.hasPlugin = true;
        slot.bypassed = false;
        slot.name = pluginName;
    }

    void toggleTrackFxSlotBypassed (int trackIndex, int slotIndex)
    {
        auto& slot = getTrackFxSlot (trackIndex, slotIndex);
        if (slot.hasPlugin)
            slot.bypassed = ! slot.bypassed;
    }

    void clearTrackFxSlot (int trackIndex, int slotIndex)
    {
        auto& slot = getTrackFxSlot (trackIndex, slotIndex);
        slot = {};
    }

    void toggleMasterMuted()
    {
        masterMixerState.muted = ! masterMixerState.muted;
    }

    void toggleMasterSoloed()
    {
        masterMixerState.soloed = ! masterMixerState.soloed;
    }

    void toggleMasterArmed()
    {
        masterMixerState.armed = ! masterMixerState.armed;
    }

    juce::String getMasterOutputAssignment() const
    {
        return masterOutputAssignment.isNotEmpty() ? masterOutputAssignment : "Output channel 1";
    }

    void setMasterOutputAssignment(juce::String newAssignment)
    {
        masterOutputAssignment = newAssignment.trim();
        if (masterOutputAssignment.isEmpty())
            masterOutputAssignment = "Output channel 1";
    }

    int getMasterFxSlotCount() const
    {
        return static_cast<int> (masterMixerState.fxSlots.size());
    }

    bool canAddMasterFxSlot() const
    {
        return getMasterFxSlotCount() < getMaxFxSlotCount();
    }

    bool canRemoveMasterFxSlot() const
    {
        return getMasterFxSlotCount() > getMinFxSlotCount();
    }

    void addMasterFxSlot()
    {
        ensureFxSlotCount (masterMixerState.fxSlots);
        if (static_cast<int> (masterMixerState.fxSlots.size()) < getMaxFxSlotCount())
            masterMixerState.fxSlots.emplace_back();
    }

    void removeMasterFxSlot (int slotIndex)
    {
        ensureFxSlotCount (masterMixerState.fxSlots);
        if (static_cast<int> (masterMixerState.fxSlots.size()) <= getMinFxSlotCount())
            return;

        const int clampedIndex = juce::jlimit (0, static_cast<int> (masterMixerState.fxSlots.size()) - 1, slotIndex);
        masterMixerState.fxSlots.erase (masterMixerState.fxSlots.begin() + clampedIndex);
    }

    TrackMixerState::FxSlotState& getMasterFxSlot (int slotIndex)
    {
        ensureFxSlotCount (masterMixerState.fxSlots);
        return masterMixerState.fxSlots[juce::jlimit (0, static_cast<int> (masterMixerState.fxSlots.size()) - 1, slotIndex)];
    }

    const TrackMixerState::FxSlotState& getMasterFxSlot (int slotIndex) const
    {
        return masterMixerState.fxSlots[juce::jlimit (0, static_cast<int> (masterMixerState.fxSlots.size()) - 1, slotIndex)];
    }

    void addDummyPluginToMasterFxSlot (int slotIndex)
    {
        auto& slot = getMasterFxSlot (slotIndex);
        slot.hasPlugin = true;
        slot.bypassed = false;
        slot.name = "Master FX " + juce::String (slotIndex + 1);
    }

    void loadMasterPluginIntoFxSlot (int slotIndex, const juce::String& pluginName)
    {
        auto& slot = getMasterFxSlot (slotIndex);
        slot.hasPlugin = true;
        slot.bypassed = false;
        slot.name = pluginName;
    }

    void toggleMasterFxSlotBypassed (int slotIndex)
    {
        auto& slot = getMasterFxSlot (slotIndex);
        if (slot.hasPlugin)
            slot.bypassed = ! slot.bypassed;
    }

    void clearMasterFxSlot (int slotIndex)
    {
        auto& slot = getMasterFxSlot (slotIndex);
        slot = {};
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

private:
    static constexpr int getMinFxSlotCount() { return 1; }
    static constexpr int getMaxFxSlotCount() { return 2; }

    static void ensureFxSlotCount (std::vector<TrackMixerState::FxSlotState>& slots)
    {
        if (static_cast<int> (slots.size()) < getMinFxSlotCount())
            slots.resize (getMinFxSlotCount());
        else if (static_cast<int> (slots.size()) > getMaxFxSlotCount())
            slots.resize (getMaxFxSlotCount());
    }

    void ensureTrackMixerStateCount()
    {
        if (trackCount < 1)
            trackCount = 1;

        if (static_cast<int> (trackMixerStates.size()) < trackCount)
            trackMixerStates.resize (trackCount);
        else if (static_cast<int> (trackMixerStates.size()) > trackCount)
            trackMixerStates.resize (trackCount);

        if (static_cast<int> (trackPatternStates.size()) < trackCount)
            trackPatternStates.resize (trackCount);
        else if (static_cast<int> (trackPatternStates.size()) > trackCount)
            trackPatternStates.resize (trackCount);

        for (auto& track : trackMixerStates)
        {
            ensureFxSlotCount (track.fxSlots);
            if (track.name.isEmpty())
                track.name = "Track " + juce::String (static_cast<int> (&track - trackMixerStates.data()) + 1);
        }

        for (auto& pattern : trackPatternStates)
            if (pattern.name.isEmpty())
                pattern.name = "Pattern 1";

        ensureFxSlotCount (masterMixerState.fxSlots);
        selectedTrackIndex = juce::jlimit (0, trackCount - 1, selectedTrackIndex);
    }
};
