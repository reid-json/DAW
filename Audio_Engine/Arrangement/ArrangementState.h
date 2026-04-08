#pragma once

#include "../Tracks/TrackInputRouter.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <functional>
#include <map>
#include <set>
#include <vector>

enum class AssetKind
{
    recording,
    audioFile,
    pianoRollPattern,
    liveInput
};

struct PatternNote
{
    double startSeconds = 0.0;
    double lengthSeconds = 0.0;
    int midiNote = 60;
    float velocity = 0.85f;
};

struct SourceAsset
{
    int assetId = 0;
    juce::String name;
    AssetKind kind = AssetKind::recording;
    RecordedClip clip;
    std::vector<PatternNote> patternNotes;
};

struct MixerTrack
{
    int mixerTrackId = 0;
    juce::String name;
    float gainLinear = 1.0f;
};

struct MasterTrack
{
    float gainLinear = 1.0f;
};

struct TimelineTrack
{
    int timelineTrackId = 0;
    juce::String name;
};

struct CentralTrackSlot
{
    enum class OutputTarget
    {
        master,
        centralTrack
    };

    int slotId = 0;
    juce::String name;
    int sourceAssetId = -1;
    int mixerTrackId = -1;
    int timelineTrackId = -1;
    OutputTarget outputTarget = OutputTarget::master;
    int outputCentralTrackId = -1;
    bool liveInputArmed = false;
};

struct TimelineClipPlacement
{
    int placementId = 0;
    int assetId = -1;
    int timelineTrackId = -1;
    juce::int64 startSample = 0;
    int centralTrackSlotId = -1;
    bool directToMaster = false;
};

class ArrangementState
{
public:
    using PatternTrackRenderer = std::function<bool(int trackIndex,
                                                    juce::AudioBuffer<float>& buffer,
                                                    juce::MidiBuffer& midi,
                                                    double sampleRate)>;

    void initialiseDefaults(int numMixerTracks = 4, int numTimelineTracks = 9);

    SourceAsset* addAsset(const juce::String& name, AssetKind kind, const RecordedClip& clip = {});
    SourceAsset* addRecentRecording(const RecordedClip& clip);
    bool renameAsset(int assetId, const juce::String& newName);
    bool updateAssetClip(int assetId, const RecordedClip& clip);

    CentralTrackSlot* createCentralTrackSlot();
    bool assignAssetToCentralTrack(int assetId, int slotId);
    bool assignRecentAssetToCentralTrack(int assetId, int slotId);
    bool placeCentralTrackOnTimeline(int slotId, int timelineTrackId, juce::int64 startSample = 0);
    bool placeRecentAssetDirectToTimeline(int assetId, int timelineTrackId, juce::int64 startSample = 0);
    bool moveTimelinePlacement(int placementId, int timelineTrackId, juce::int64 startSample);
    bool removeTimelinePlacement(int placementId);

    bool setCentralTrackMixerTrack(int slotId, int mixerTrackId);
    bool setCentralTrackTimelineTrack(int slotId, int timelineTrackId);
    bool setCentralTrackOutputToMaster(int slotId);
    bool setCentralTrackOutputToTrack(int slotId, int destinationSlotId);
    bool setCentralTrackLiveInputArmed(int slotId, bool shouldArm);

    void setPatternTrackRenderer(PatternTrackRenderer renderer);
    void render(juce::AudioBuffer<float>& buffer, juce::int64 playbackStartSample) const;

    const std::vector<SourceAsset>& getAssets() const { return assets; }
    const std::vector<int>& getRecentAssetIds() const { return recentAssetIds; }
    const std::vector<CentralTrackSlot>& getCentralTrackSlots() const { return centralTrackSlots; }
    const std::vector<TimelineTrack>& getTimelineTracks() const { return timelineTracks; }
    const std::vector<TimelineClipPlacement>& getTimelinePlacements() const { return timelinePlacements; }
    const std::vector<MixerTrack>& getMixerTracks() const { return mixerTracks; }
    const MasterTrack& getMasterTrack() const { return masterTrack; }

    const SourceAsset* findAsset(int assetId) const;
    SourceAsset* findAsset(int assetId);
    const CentralTrackSlot* findCentralTrackSlot(int slotId) const;
    const TimelineTrack* findTimelineTrack(int timelineTrackId) const;
    const TimelineClipPlacement* findTimelinePlacement(int placementId) const;

private:
    struct ActivePatternNote
    {
        int placementId = 0;
        int noteIndex = 0;

        auto tie() const noexcept { return std::tie(placementId, noteIndex); }
        bool operator<(const ActivePatternNote& other) const noexcept { return tie() < other.tie(); }
    };

    float resolveRouteGainForSlot(int slotId, std::vector<int>& visitedSlotIds) const;
    void renderPatternAsset(const SourceAsset& asset,
                            const TimelineClipPlacement& placement,
                            juce::AudioBuffer<float>& buffer,
                            juce::int64 playbackStartSample,
                            float gain) const;

    mutable PatternTrackRenderer patternTrackRenderer;
    mutable std::map<int, std::set<ActivePatternNote>> activePatternNotesByTrack;
    mutable juce::int64 lastPatternRenderEndSample = -1;
    int nextAssetId = 1;
    int nextCentralTrackSlotId = 1;
    int nextPlacementId = 1;
    std::vector<SourceAsset> assets;
    std::vector<int> recentAssetIds;
    std::vector<CentralTrackSlot> centralTrackSlots;
    std::vector<TimelineTrack> timelineTracks;
    std::vector<TimelineClipPlacement> timelinePlacements;
    std::vector<MixerTrack> mixerTracks;
    MasterTrack masterTrack;
};
