#include "ProjectFile.h"
#include <fstream>

// helpers for binary read/write

static void writeString(std::ofstream& out, const juce::String& str)
{
    auto utf8 = str.toStdString();
    int32_t len = static_cast<int32_t>(utf8.size());
    out.write(reinterpret_cast<const char*>(&len), sizeof(len));
    if (len > 0)
        out.write(utf8.data(), len);
}

static juce::String readString(std::ifstream& in)
{
    int32_t len = 0;
    in.read(reinterpret_cast<char*>(&len), sizeof(len));
    if (len <= 0) return {};
    std::string buf(static_cast<size_t>(len), '\0');
    in.read(buf.data(), len);
    return juce::String(buf);
}

template <typename T>
static void writeVal(std::ofstream& out, const T& val)
{
    out.write(reinterpret_cast<const char*>(&val), sizeof(T));
}

template <typename T>
static T readVal(std::ifstream& in)
{
    T val{};
    in.read(reinterpret_cast<char*>(&val), sizeof(T));
    return val;
}

// ---- save ----

bool saveProject(const juce::File& file,
                 const DAWState& dawState,
                 const ArrangementState& arrangement)
{
    std::ofstream out(file.getFullPathName().toStdString(), std::ios::binary);
    if (!out.is_open()) return false;

    // Header
    out.write("DAWP", 4);
    writeVal<uint32_t>(out, 2); // version

    // Global state
    writeVal(out, dawState.tempoBpm);
    writeVal(out, static_cast<int32_t>(dawState.trackCount));

    // Master settings
    auto masterGain = arrangement.getMasterTrack().gainLinear;
    writeVal(out, masterGain);
    writeVal(out, dawState.masterMixerState.level);
    writeVal(out, dawState.masterMixerState.muted);
    writeVal(out, dawState.masterMixerState.pan);
    writeString(out, dawState.masterOutputAssignment);

    // Master FX slots
    writeVal(out, static_cast<int32_t>(dawState.masterMixerState.fxSlots.size()));
    for (const auto& slot : dawState.masterMixerState.fxSlots)
    {
        writeString(out, slot.name);
        writeVal(out, slot.hasPlugin);
        writeVal(out, slot.bypassed);
    }

    // Per-track mixer state
    writeVal(out, static_cast<int32_t>(dawState.trackMixerStates.size()));
    for (const auto& track : dawState.trackMixerStates)
    {
        writeString(out, track.name);
        writeVal(out, static_cast<int32_t>(track.contentType));
        writeVal(out, track.muted);
        writeVal(out, track.armed);
        writeVal(out, track.pan);
        writeVal(out, track.level);
        writeString(out, track.inputAssignment);
        writeString(out, track.outputAssignment);

        writeVal(out, static_cast<int32_t>(track.fxSlots.size()));
        for (const auto& slot : track.fxSlots)
        {
            writeString(out, slot.name);
            writeVal(out, slot.hasPlugin);
            writeVal(out, slot.bypassed);
        }

        // Instrument slot
        writeString(out, track.instrumentSlot.name);
        writeVal(out, track.instrumentSlot.hasPlugin);
        writeVal(out, track.instrumentSlot.bypassed);
    }

    // Per-track pattern state
    writeVal(out, static_cast<int32_t>(dawState.trackPatternStates.size()));
    for (const auto& pattern : dawState.trackPatternStates)
    {
        writeVal(out, static_cast<int32_t>(pattern.assetId));
        writeString(out, pattern.name);

        writeVal(out, static_cast<int32_t>(pattern.notes.size()));
        for (const auto& note : pattern.notes)
        {
            writeVal(out, static_cast<int32_t>(note.id));
            writeVal(out, note.startBeat);
            writeVal(out, note.lengthBeats);
            writeVal(out, static_cast<int32_t>(note.midiNote));
            writeString(out, note.label);
        }
    }

    // ArrangementState IDs
    writeVal(out, static_cast<int32_t>(arrangement.getNextAssetId()));
    writeVal(out, static_cast<int32_t>(arrangement.getNextPlacementId()));

    // Assets
    const auto& assets = arrangement.getAssets();
    writeVal(out, static_cast<int32_t>(assets.size()));
    for (const auto& asset : assets)
    {
        writeVal(out, static_cast<int32_t>(asset.assetId));
        writeString(out, asset.name);
        writeVal(out, static_cast<int32_t>(asset.kind));
        writeVal(out, asset.clip.sampleRate);

        int32_t sampleCount = static_cast<int32_t>(asset.clip.leftChannel.size());
        writeVal(out, sampleCount);
        if (sampleCount > 0)
        {
            out.write(reinterpret_cast<const char*>(asset.clip.leftChannel.data()),
                      sampleCount * static_cast<std::streamsize>(sizeof(float)));
            out.write(reinterpret_cast<const char*>(asset.clip.rightChannel.data()),
                      sampleCount * static_cast<std::streamsize>(sizeof(float)));
        }

        // Pattern notes
        writeVal(out, static_cast<int32_t>(asset.patternNotes.size()));
        for (const auto& note : asset.patternNotes)
        {
            writeVal(out, note.startSeconds);
            writeVal(out, note.lengthSeconds);
            writeVal(out, static_cast<int32_t>(note.midiNote));
            writeVal(out, note.velocity);
        }
    }

    // Timeline tracks
    const auto& tTracks = arrangement.getTimelineTracks();
    writeVal(out, static_cast<int32_t>(tTracks.size()));
    for (const auto& t : tTracks)
    {
        writeVal(out, static_cast<int32_t>(t.timelineTrackId));
        writeString(out, t.name);
    }

    // Mixer tracks
    const auto& mTracks = arrangement.getMixerTracks();
    writeVal(out, static_cast<int32_t>(mTracks.size()));
    for (const auto& m : mTracks)
    {
        writeVal(out, static_cast<int32_t>(m.mixerTrackId));
        writeString(out, m.name);
        writeVal(out, m.gainLinear);
        writeVal(out, m.panLinear);
    }

    // Timeline placements
    const auto& placements = arrangement.getTimelinePlacements();
    writeVal(out, static_cast<int32_t>(placements.size()));
    for (const auto& p : placements)
    {
        writeVal(out, static_cast<int32_t>(p.placementId));
        writeVal(out, static_cast<int32_t>(p.assetId));
        writeVal(out, static_cast<int32_t>(p.timelineTrackId));
        writeVal(out, static_cast<int64_t>(p.startSample));
        writeVal(out, p.directToMaster);
    }

    out.close();
    return out.good() || !out.fail();
}

// ---- load ----

bool loadProject(const juce::File& file,
                 DAWState& dawState,
                 ArrangementState& arrangement)
{
    std::ifstream in(file.getFullPathName().toStdString(), std::ios::binary);
    if (!in.is_open()) return false;

    // Header
    char magic[4] = {};
    in.read(magic, 4);
    if (std::string(magic, 4) != "DAWP") return false;

    auto version = readVal<uint32_t>(in);
    if (version != 2) return false;

    // Global state
    dawState.tempoBpm = readVal<double>(in);
    dawState.trackCount = readVal<int32_t>(in);

    // Master settings
    float masterGain = readVal<float>(in);
    dawState.masterMixerState.level = readVal<float>(in);
    dawState.masterMixerState.muted = readVal<bool>(in);
    dawState.masterMixerState.pan = readVal<float>(in);
    dawState.masterOutputAssignment = readString(in);

    // Master FX slots
    int32_t masterFxCount = readVal<int32_t>(in);
    dawState.masterMixerState.fxSlots.resize(static_cast<size_t>(masterFxCount));
    for (int i = 0; i < masterFxCount; ++i)
    {
        dawState.masterMixerState.fxSlots[static_cast<size_t>(i)].name = readString(in);
        dawState.masterMixerState.fxSlots[static_cast<size_t>(i)].hasPlugin = readVal<bool>(in);
        dawState.masterMixerState.fxSlots[static_cast<size_t>(i)].bypassed = readVal<bool>(in);
    }

    // Per-track mixer state
    int32_t trackMixerCount = readVal<int32_t>(in);
    dawState.trackMixerStates.resize(static_cast<size_t>(trackMixerCount));
    for (int i = 0; i < trackMixerCount; ++i)
    {
        auto& track = dawState.trackMixerStates[static_cast<size_t>(i)];
        track.name = readString(in);
        track.contentType = static_cast<TrackMixerState::ContentType>(readVal<int32_t>(in));
        track.muted = readVal<bool>(in);
        track.armed = readVal<bool>(in);
        track.pan = readVal<float>(in);
        track.level = readVal<float>(in);
        track.inputAssignment = readString(in);
        track.outputAssignment = readString(in);

        int32_t fxCount = readVal<int32_t>(in);
        track.fxSlots.resize(static_cast<size_t>(fxCount));
        for (int j = 0; j < fxCount; ++j)
        {
            track.fxSlots[static_cast<size_t>(j)].name = readString(in);
            track.fxSlots[static_cast<size_t>(j)].hasPlugin = readVal<bool>(in);
            track.fxSlots[static_cast<size_t>(j)].bypassed = readVal<bool>(in);
        }

        // Instrument slot
        track.instrumentSlot.name = readString(in);
        track.instrumentSlot.hasPlugin = readVal<bool>(in);
        track.instrumentSlot.bypassed = readVal<bool>(in);
    }

    // Per-track pattern state
    int32_t patternCount = readVal<int32_t>(in);
    dawState.trackPatternStates.resize(static_cast<size_t>(patternCount));
    for (int i = 0; i < patternCount; ++i)
    {
        auto& pattern = dawState.trackPatternStates[static_cast<size_t>(i)];
        pattern.assetId = readVal<int32_t>(in);
        pattern.name = readString(in);

        int32_t noteCount = readVal<int32_t>(in);
        pattern.notes.resize(static_cast<size_t>(noteCount));
        for (int j = 0; j < noteCount; ++j)
        {
            auto& note = pattern.notes[static_cast<size_t>(j)];
            note.id = readVal<int32_t>(in);
            note.startBeat = readVal<double>(in);
            note.lengthBeats = readVal<double>(in);
            note.midiNote = readVal<int32_t>(in);
            note.label = readString(in);
        }
    }

    // ArrangementState IDs
    int savedNextAssetId = readVal<int32_t>(in);
    int savedNextPlacementId = readVal<int32_t>(in);

    // Assets
    int32_t assetCount = readVal<int32_t>(in);
    std::vector<SourceAsset> savedAssets;
    savedAssets.resize(static_cast<size_t>(assetCount));
    for (int i = 0; i < assetCount; ++i)
    {
        auto& asset = savedAssets[static_cast<size_t>(i)];
        asset.assetId = readVal<int32_t>(in);
        asset.name = readString(in);
        asset.kind = static_cast<AssetKind>(readVal<int32_t>(in));
        asset.clip.sampleRate = readVal<double>(in);
        asset.clip.clipId = asset.assetId;
        asset.clip.name = asset.name;

        int32_t sampleCount = readVal<int32_t>(in);
        if (sampleCount > 0)
        {
            asset.clip.leftChannel.resize(static_cast<size_t>(sampleCount));
            asset.clip.rightChannel.resize(static_cast<size_t>(sampleCount));
            in.read(reinterpret_cast<char*>(asset.clip.leftChannel.data()),
                    sampleCount * static_cast<std::streamsize>(sizeof(float)));
            in.read(reinterpret_cast<char*>(asset.clip.rightChannel.data()),
                    sampleCount * static_cast<std::streamsize>(sizeof(float)));
        }

        // Pattern notes
        int32_t pNoteCount = readVal<int32_t>(in);
        asset.patternNotes.resize(static_cast<size_t>(pNoteCount));
        for (int j = 0; j < pNoteCount; ++j)
        {
            auto& note = asset.patternNotes[static_cast<size_t>(j)];
            note.startSeconds = readVal<double>(in);
            note.lengthSeconds = readVal<double>(in);
            note.midiNote = readVal<int32_t>(in);
            note.velocity = readVal<float>(in);
        }
    }

    // Timeline tracks
    int32_t tTrackCount = readVal<int32_t>(in);
    std::vector<TimelineTrack> savedTimelineTracks(static_cast<size_t>(tTrackCount));
    for (int i = 0; i < tTrackCount; ++i)
    {
        savedTimelineTracks[static_cast<size_t>(i)].timelineTrackId = readVal<int32_t>(in);
        savedTimelineTracks[static_cast<size_t>(i)].name = readString(in);
    }

    // Mixer tracks
    int32_t mTrackCount = readVal<int32_t>(in);
    std::vector<MixerTrack> savedMixerTracks(static_cast<size_t>(mTrackCount));
    for (int i = 0; i < mTrackCount; ++i)
    {
        savedMixerTracks[static_cast<size_t>(i)].mixerTrackId = readVal<int32_t>(in);
        savedMixerTracks[static_cast<size_t>(i)].name = readString(in);
        savedMixerTracks[static_cast<size_t>(i)].gainLinear = readVal<float>(in);
        savedMixerTracks[static_cast<size_t>(i)].panLinear = readVal<float>(in);
    }

    // Timeline placements
    int32_t placementCount = readVal<int32_t>(in);
    std::vector<TimelineClipPlacement> savedPlacements(static_cast<size_t>(placementCount));
    for (int i = 0; i < placementCount; ++i)
    {
        auto& p = savedPlacements[static_cast<size_t>(i)];
        p.placementId = readVal<int32_t>(in);
        p.assetId = readVal<int32_t>(in);
        p.timelineTrackId = readVal<int32_t>(in);
        p.startSample = static_cast<juce::int64>(readVal<int64_t>(in));
        p.directToMaster = readVal<bool>(in);
    }

    if (in.fail()) return false;

    // Build the master track
    MasterTrack savedMasterTrack;
    savedMasterTrack.gainLinear = masterGain;

    // Commit everything to the arrangement
    arrangement.loadFromSavedData(std::move(savedAssets),
                                  std::move(savedTimelineTracks),
                                  std::move(savedPlacements),
                                  std::move(savedMixerTracks),
                                  savedMasterTrack,
                                  savedNextAssetId,
                                  savedNextPlacementId);

    // Reset GUI-side transient state
    dawState.transportState = DAWState::TransportState::stopped;
    dawState.playhead = 0.0;
    dawState.isRecording = false;
    dawState.selectedTrackIndex = 0;
    dawState.horizontalScrollOffset = 0;

    return true;
}
