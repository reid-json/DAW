#pragma once

#include <algorithm>
#include <vector>
#include <juce_core/juce_core.h>

struct RecentClipItem
{
    int assetId = 0;
    juce::String name;
    double lengthSeconds = 0.0;

    bool operator== (const RecentClipItem& o) const
    {
        return assetId == o.assetId && name == o.name && lengthSeconds == o.lengthSeconds;
    }
};

struct TimelineClipItem
{
    enum class ContentKind { recording, pattern };

    int placementId = 0;
    int assetId = 0;
    juce::String name;
    ContentKind kind = ContentKind::recording;
    int trackIndex = 0;
    double startSeconds = 0.0;
    double lengthSeconds = 0.0;

    bool operator== (const TimelineClipItem& o) const
    {
        return placementId == o.placementId && assetId == o.assetId && name == o.name
            && kind == o.kind && trackIndex == o.trackIndex
            && startSeconds == o.startSeconds && lengthSeconds == o.lengthSeconds;
    }
};

struct SavedPatternItem
{
    int assetId = 0;
    int trackIndex = 0;
    juce::String name;
    double lengthSeconds = 0.0;

    bool operator== (const SavedPatternItem& o) const
    {
        return assetId == o.assetId && trackIndex == o.trackIndex
            && name == o.name && lengthSeconds == o.lengthSeconds;
    }
};

struct TrackMixerState
{
    enum class ContentType { recording, pattern };

    juce::String name;
    ContentType contentType = ContentType::recording;
    bool muted = false;
    bool armed = false;
    float pan = 0.0f;
    float level = 0.72f;
    juce::String inputAssignment = "Input channel 1";
    juce::String outputAssignment = "Master";

    struct FxSlotState
    {
        juce::String name;
        bool hasPlugin = false;
        bool bypassed = false;
    };

    using InstrumentSlotState = FxSlotState;

    std::vector<FxSlotState> fxSlots;
    InstrumentSlotState instrumentSlot;
};

struct TrackPatternNote
{
    int id = 0;
    double startBeat = 0.0;
    double lengthBeats = 1.0;
    int midiNote = 60;
    juce::String label = "C4";

    bool operator== (const TrackPatternNote& o) const
    {
        return id == o.id && startBeat == o.startBeat
            && lengthBeats == o.lengthBeats && midiNote == o.midiNote && label == o.label;
    }
};

struct TrackPatternState
{
    int assetId = -1;
    juce::String name = "Pattern 1";
    std::vector<TrackPatternNote> notes;

    bool operator== (const TrackPatternState& o) const
    {
        return assetId == o.assetId && name == o.name && notes == o.notes;
    }
};

struct MasterMixerState
{
    bool muted = false;
    float pan = 0.0f;
    float level = 0.8f;
    std::vector<TrackMixerState::FxSlotState> fxSlots;
};

struct DAWState
{
    enum class TransportState { stopped, playing, paused };

    TransportState transportState = TransportState::stopped;

    int trackCount = 5;
    int sidebarWidth = 340;
    int clipSidebarWidth = 220;
    int horizontalScrollOffset = 0;
    int verticalScrollOffset = 0;
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

    DAWState() { ensureTrackCount(); }

    // Transport
    void play()    { transportState = TransportState::playing; }
    void pause()   { transportState = TransportState::paused; }
    void stop()    { transportState = TransportState::stopped; playhead = 0.0; isRecording = false; }
    void toggleAudioMonitoring() { audioMonitoringEnabled = ! audioMonitoringEnabled; }
    void setTempoBpm (double bpm) { tempoBpm = juce::jlimit (40.0, 240.0, bpm); }

    // Track management
    void addTrack()
    {
        ++trackCount;
        ensureTrackCount();
        selectedTrackIndex = trackCount - 1;
    }

    void removeTrackAt (int idx)
    {
        if (trackCount <= 1) return;
        int i = juce::jlimit (0, trackCount - 1, idx);
        --trackCount;

        if (i < (int) trackMixerStates.size())
            trackMixerStates.erase (trackMixerStates.begin() + i);
        if (i < (int) trackPatternStates.size())
            trackPatternStates.erase (trackPatternStates.begin() + i);

        ensureTrackCount();
        selectedTrackIndex = juce::jlimit (0, trackCount - 1,
                                           selectedTrackIndex > i ? selectedTrackIndex - 1 : selectedTrackIndex);
    }

    void toggleClipSidebar() { clipSidebarCollapsed = ! clipSidebarCollapsed; }

    // Track state accessors
    TrackMixerState& getTrackMixerState (int i)
    {
        ensureTrackCount();
        return trackMixerStates[juce::jlimit (0, trackCount - 1, i)];
    }

    const TrackMixerState& getTrackMixerState (int i) const
    {
        return trackMixerStates[juce::jlimit (0, trackCount - 1, i)];
    }

    TrackPatternState& getTrackPatternState (int i)
    {
        ensureTrackCount();
        return trackPatternStates[juce::jlimit (0, trackCount - 1, i)];
    }

    const TrackPatternState& getTrackPatternState (int i) const
    {
        return trackPatternStates[juce::jlimit (0, trackCount - 1, i)];
    }

    void setTrackPatternName (int i, juce::String name)
    {
        auto& p = getTrackPatternState (i);
        p.name = name.trim();
        if (p.name.isEmpty()) p.name = "Pattern 1";
    }

    // Track toggles
    void toggleTrackMuted  (int i) { getTrackMixerState (i).muted  = ! getTrackMixerState (i).muted; }
    void toggleTrackArmed  (int i) { getTrackMixerState (i).armed  = ! getTrackMixerState (i).armed; }

    void armOnlyTrack (int target)
    {
        ensureTrackCount();
        int t = juce::jlimit (0, trackCount - 1, target);
        for (int i = 0; i < trackCount; ++i)
            trackMixerStates[i].armed = (i == t);
    }

    void disarmAllTracks()
    {
        ensureTrackCount();
        for (int i = 0; i < trackCount; ++i)
            trackMixerStates[i].armed = false;
    }

    int getFirstArmedTrackIndex() const
    {
        for (int i = 0; i < trackCount; ++i)
            if (getTrackMixerState (i).armed) return i;
        return -1;
    }

    void setTrackPan   (int i, float v) { getTrackMixerState (i).pan   = juce::jlimit (-1.0f, 1.0f, v); }
    void setTrackLevel (int i, float v) { getTrackMixerState (i).level = juce::jlimit (0.0f, 1.0f, v); }

    // Selection / focus
    void selectTrack (int i)               { selectedTrackIndex = juce::jlimit (0, trackCount - 1, i); }
    void showMasterMixerFocus()            { focusedMixerShowsMaster = true; }
    void showSelectedTrackMixerFocus()     { focusedMixerShowsMaster = false; selectTrack (selectedTrackIndex); }
    bool isMasterMixerFocused() const      { return focusedMixerShowsMaster; }
    bool isTrackSelected (int i) const     { return juce::jlimit (0, trackCount - 1, selectedTrackIndex) == i; }

    bool isTrackAudible (int i) const
    {
        return ! getTrackMixerState (i).muted;
    }

    // Track naming
    juce::String getTrackName (int i) const
    {
        auto& t = getTrackMixerState (i);
        return t.name.isNotEmpty() ? t.name : "Track " + juce::String (i + 1);
    }

    void setTrackName (int i, juce::String name)
    {
        auto& t = getTrackMixerState (i);
        t.name = name.trim();
        if (t.name.isEmpty()) t.name = "Track " + juce::String (i + 1);
    }

    // I/O assignments
    juce::String getTrackInputAssignment (int i) const
    {
        auto& t = getTrackMixerState (i);
        return t.inputAssignment.isNotEmpty() ? t.inputAssignment : "Input channel 1";
    }

    juce::String getTrackOutputAssignment (int i) const
    {
        auto& t = getTrackMixerState (i);
        return t.outputAssignment.isNotEmpty() ? t.outputAssignment : "Output channel 1";
    }

    void setTrackInputAssignment (int i, juce::String s)
    {
        auto& t = getTrackMixerState (i);
        t.inputAssignment = s.trim();
        if (t.inputAssignment.isEmpty()) t.inputAssignment = "Input channel 1";
    }

    void setTrackOutputAssignment (int i, juce::String s)
    {
        auto& t = getTrackMixerState (i);
        t.outputAssignment = s.trim();
        if (t.outputAssignment.isEmpty()) t.outputAssignment = "Master";
    }

    // Track FX slots
    int getTrackFxSlotCount (int i) const { return (int) getTrackMixerState (i).fxSlots.size(); }

    void addTrackFxSlot (int i)
    {
        auto& t = getTrackMixerState (i);
        if ((int) t.fxSlots.size() < maxFxSlots) t.fxSlots.emplace_back();
    }

    void removeTrackFxSlot (int i, int slot)
    {
        auto& t = getTrackMixerState (i);
        if ((int) t.fxSlots.size() <= minFxSlots) return;
        int s = juce::jlimit (0, (int) t.fxSlots.size() - 1, slot);
        t.fxSlots.erase (t.fxSlots.begin() + s);
    }

    TrackMixerState::FxSlotState& getTrackFxSlot (int i, int slot)
    {
        auto& t = getTrackMixerState (i);
        ensureFxSlots (t.fxSlots);
        return t.fxSlots[juce::jlimit (0, (int) t.fxSlots.size() - 1, slot)];
    }

    const TrackMixerState::FxSlotState& getTrackFxSlot (int i, int slot) const
    {
        auto& t = getTrackMixerState (i);
        return t.fxSlots[juce::jlimit (0, (int) t.fxSlots.size() - 1, slot)];
    }

    void loadTrackPluginIntoFxSlot (int i, int slot, const juce::String& name)
    {
        auto& s = getTrackFxSlot (i, slot);
        s.hasPlugin = true; s.bypassed = false; s.name = name;
    }

    void toggleTrackFxSlotBypassed (int i, int slot)
    {
        auto& s = getTrackFxSlot (i, slot);
        if (s.hasPlugin) s.bypassed = ! s.bypassed;
    }

    void clearTrackFxSlot (int i, int slot) { getTrackFxSlot (i, slot) = {}; }

    // Master
    void toggleMasterMuted()  { masterMixerState.muted  = ! masterMixerState.muted; }

    juce::String getMasterOutputAssignment() const
    {
        return masterOutputAssignment.isNotEmpty() ? masterOutputAssignment : "Output channel 1";
    }

    void setMasterOutputAssignment (juce::String s)
    {
        masterOutputAssignment = s.trim();
        if (masterOutputAssignment.isEmpty()) masterOutputAssignment = "Output channel 1";
    }

    int getMasterFxSlotCount() const { return (int) masterMixerState.fxSlots.size(); }

    void addMasterFxSlot()
    {
        ensureFxSlots (masterMixerState.fxSlots);
        if ((int) masterMixerState.fxSlots.size() < maxFxSlots)
            masterMixerState.fxSlots.emplace_back();
    }

    void removeMasterFxSlot (int slot)
    {
        ensureFxSlots (masterMixerState.fxSlots);
        if ((int) masterMixerState.fxSlots.size() <= minFxSlots) return;
        int s = juce::jlimit (0, (int) masterMixerState.fxSlots.size() - 1, slot);
        masterMixerState.fxSlots.erase (masterMixerState.fxSlots.begin() + s);
    }

    TrackMixerState::FxSlotState& getMasterFxSlot (int slot)
    {
        ensureFxSlots (masterMixerState.fxSlots);
        return masterMixerState.fxSlots[juce::jlimit (0, (int) masterMixerState.fxSlots.size() - 1, slot)];
    }

    const TrackMixerState::FxSlotState& getMasterFxSlot (int slot) const
    {
        return masterMixerState.fxSlots[juce::jlimit (0, (int) masterMixerState.fxSlots.size() - 1, slot)];
    }

    void loadMasterPluginIntoFxSlot (int slot, const juce::String& name)
    {
        auto& s = getMasterFxSlot (slot);
        s.hasPlugin = true; s.bypassed = false; s.name = name;
    }

    void toggleMasterFxSlotBypassed (int slot)
    {
        auto& s = getMasterFxSlot (slot);
        if (s.hasPlugin) s.bypassed = ! s.bypassed;
    }

    void clearMasterFxSlot (int slot) { getMasterFxSlot (slot) = {}; }

private:
    static constexpr int minFxSlots = 1;
    static constexpr int maxFxSlots = 5;

    static void ensureFxSlots (std::vector<TrackMixerState::FxSlotState>& slots)
    {
        if ((int) slots.size() < minFxSlots) slots.resize (minFxSlots);
        else if ((int) slots.size() > maxFxSlots) slots.resize (maxFxSlots);
    }

    void ensureTrackCount()
    {
        trackCount = juce::jmax (1, trackCount);
        trackMixerStates.resize (trackCount);
        trackPatternStates.resize (trackCount);

        for (int i = 0; i < trackCount; ++i)
        {
            auto& t = trackMixerStates[i];
            ensureFxSlots (t.fxSlots);
            if (t.name.isEmpty()) t.name = "Track " + juce::String (i + 1);
        }

        for (auto& p : trackPatternStates)
            if (p.name.isEmpty()) p.name = "Pattern 1";

        ensureFxSlots (masterMixerState.fxSlots);
        selectedTrackIndex = juce::jlimit (0, trackCount - 1, selectedTrackIndex);
    }
};
