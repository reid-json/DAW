#include "ProjectFile.h"
#include <fstream>

static void writeString(std::ofstream& out, const juce::String& str)
{
    auto utf8 = str.toStdString();
    int32_t len = (int32_t) utf8.size();
    out.write(reinterpret_cast<const char*>(&len), sizeof(len));
    if (len > 0) out.write(utf8.data(), len);
}

static juce::String readString(std::ifstream& in)
{
    int32_t len = 0;
    in.read(reinterpret_cast<char*>(&len), sizeof(len));
    if (len <= 0) return {};
    std::string buf((size_t) len, '\0');
    in.read(buf.data(), len);
    return juce::String(buf);
}

static void writeMemoryBlock(std::ofstream& out, const juce::MemoryBlock& block)
{
    const auto size = (int32_t) block.getSize();
    out.write(reinterpret_cast<const char*>(&size), sizeof(size));
    if (size > 0)
        out.write(static_cast<const char*>(block.getData()), size);
}

static juce::MemoryBlock readMemoryBlock(std::ifstream& in)
{
    int32_t size = 0;
    in.read(reinterpret_cast<char*>(&size), sizeof(size));
    if (size <= 0)
        return {};

    juce::MemoryBlock block((size_t) size);
    in.read(static_cast<char*>(block.getData()), size);
    return block;
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

bool saveProject(const juce::File& file,
                 const DAWState& dawState,
                 const ArrangementState& arrangement)
{
    std::ofstream out(file.getFullPathName().toStdString(), std::ios::binary);
    if (!out.is_open()) return false;

    out.write("DAWP", 4);
    writeVal<uint32_t>(out, 4);

    writeVal(out, dawState.tempoBpm);
    writeVal(out, (int32_t) dawState.trackCount);

    writeVal(out, arrangement.getMasterTrack().gainLinear);
    writeVal(out, dawState.masterMixerState.level);
    writeVal(out, dawState.masterMixerState.muted);
    writeVal(out, dawState.masterMixerState.pan);

    writeVal(out, (int32_t) dawState.masterMixerState.fxSlots.size());
    for (const auto& slot : dawState.masterMixerState.fxSlots)
    {
        writeString(out, slot.name);
        writeVal(out, slot.hasPlugin);
        writeVal(out, slot.bypassed);
        writeMemoryBlock(out, slot.pluginState);
    }

    writeVal(out, (int32_t) dawState.trackMixerStates.size());
    for (const auto& track : dawState.trackMixerStates)
    {
        writeString(out, track.name);
        writeVal(out, (int32_t) track.contentType);
        writeVal(out, track.muted);
        writeVal(out, track.armed);
        writeVal(out, track.pan);
        writeVal(out, track.level);
        writeString(out, track.inputAssignment);
        writeString(out, track.outputAssignment);

        writeVal(out, (int32_t) track.fxSlots.size());
        for (const auto& slot : track.fxSlots)
        {
            writeString(out, slot.name);
            writeVal(out, slot.hasPlugin);
            writeVal(out, slot.bypassed);
            writeMemoryBlock(out, slot.pluginState);
        }
    }

    writeVal(out, (int32_t) dawState.trackPatternStates.size());
    for (const auto& pattern : dawState.trackPatternStates)
    {
        writeVal(out, (int32_t) pattern.assetId);
        writeString(out, pattern.name);
        writeString(out, pattern.instrumentName);
        writeMemoryBlock(out, pattern.instrumentState);

        writeVal(out, (int32_t) pattern.notes.size());
        for (const auto& note : pattern.notes)
        {
            writeVal(out, (int32_t) note.id);
            writeVal(out, note.startBeat);
            writeVal(out, note.lengthBeats);
            writeVal(out, (int32_t) note.midiNote);
            writeString(out, note.label);
        }
    }

    writeVal(out, (int32_t) arrangement.getNextAssetId());
    writeVal(out, (int32_t) arrangement.getNextPlacementId());

    const auto& assets = arrangement.getAssets();
    writeVal(out, (int32_t) assets.size());
    for (const auto& asset : assets)
    {
        writeVal(out, (int32_t) asset.assetId);
        writeString(out, asset.name);
        writeVal(out, (int32_t) asset.kind);
        writeString(out, asset.instrumentName);
        writeVal(out, asset.clip.sampleRate);

        int32_t sampleCount = (int32_t) asset.clip.leftChannel.size();
        writeVal(out, sampleCount);
        if (sampleCount > 0)
        {
            out.write(reinterpret_cast<const char*>(asset.clip.leftChannel.data()), sampleCount * (std::streamsize) sizeof(float));
            out.write(reinterpret_cast<const char*>(asset.clip.rightChannel.data()), sampleCount * (std::streamsize) sizeof(float));
        }

        writeVal(out, (int32_t) asset.patternNotes.size());
        for (const auto& note : asset.patternNotes)
        {
            writeVal(out, note.startSeconds);
            writeVal(out, note.lengthSeconds);
            writeVal(out, (int32_t) note.midiNote);
            writeVal(out, note.velocity);
        }
    }

    const auto& tTracks = arrangement.getTimelineTracks();
    writeVal(out, (int32_t) tTracks.size());
    for (const auto& t : tTracks)
    {
        writeVal(out, (int32_t) t.timelineTrackId);
        writeString(out, t.name);
    }

    const auto& mTracks = arrangement.getMixerTracks();
    writeVal(out, (int32_t) mTracks.size());
    for (const auto& m : mTracks)
    {
        writeVal(out, (int32_t) m.mixerTrackId);
        writeString(out, m.name);
        writeVal(out, m.gainLinear);
        writeVal(out, m.panLinear);
    }

    const auto& placements = arrangement.getTimelinePlacements();
    writeVal(out, (int32_t) placements.size());
    for (const auto& p : placements)
    {
        writeVal(out, (int32_t) p.placementId);
        writeVal(out, (int32_t) p.assetId);
        writeVal(out, (int32_t) p.timelineTrackId);
        writeVal(out, (int64_t) p.startSample);
        writeVal(out, p.directToMaster);
    }

    out.close();
    return out.good() || !out.fail();
}

bool loadProject(const juce::File& file,
                 DAWState& dawState,
                 ArrangementState& arrangement)
{
    std::ifstream in(file.getFullPathName().toStdString(), std::ios::binary);
    if (!in.is_open()) return false;

    char magic[4] = {};
    in.read(magic, 4);
    if (std::string(magic, 4) != "DAWP") return false;
    const auto fileVersion = readVal<uint32_t>(in);
    if (fileVersion < 2 || fileVersion > 4) return false;

    dawState.tempoBpm = readVal<double>(in);
    dawState.trackCount = readVal<int32_t>(in);

    const float masterGain = readVal<float>(in);
    dawState.masterMixerState.level = readVal<float>(in);
    dawState.masterMixerState.muted = readVal<bool>(in);
    dawState.masterMixerState.pan = readVal<float>(in);

    dawState.masterMixerState.fxSlots.resize((size_t) readVal<int32_t>(in));
    for (auto& slot : dawState.masterMixerState.fxSlots)
    {
        slot.name = readString(in);
        slot.hasPlugin = readVal<bool>(in);
        slot.bypassed = readVal<bool>(in);
        if (fileVersion >= 4)
            slot.pluginState = readMemoryBlock(in);
    }

    dawState.trackMixerStates.resize((size_t) readVal<int32_t>(in));
    for (auto& track : dawState.trackMixerStates)
    {
        track.name = readString(in);
        track.contentType = (TrackMixerState::ContentType) readVal<int32_t>(in);
        track.muted = readVal<bool>(in);
        track.armed = readVal<bool>(in);
        track.pan = readVal<float>(in);
        track.level = readVal<float>(in);
        track.inputAssignment = readString(in);
        track.outputAssignment = readString(in);

        track.fxSlots.resize((size_t) readVal<int32_t>(in));
        for (auto& slot : track.fxSlots)
        {
            slot.name = readString(in);
            slot.hasPlugin = readVal<bool>(in);
            slot.bypassed = readVal<bool>(in);
            if (fileVersion >= 4)
                slot.pluginState = readMemoryBlock(in);
        }
    }

    dawState.trackPatternStates.resize((size_t) readVal<int32_t>(in));
    for (auto& pattern : dawState.trackPatternStates)
    {
        pattern.assetId = readVal<int32_t>(in);
        pattern.name = readString(in);
        if (fileVersion >= 4)
        {
            pattern.instrumentName = readString(in);
            pattern.instrumentState = readMemoryBlock(in);
        }

        pattern.notes.resize((size_t) readVal<int32_t>(in));
        for (auto& note : pattern.notes)
        {
            note.id = readVal<int32_t>(in);
            note.startBeat = readVal<double>(in);
            note.lengthBeats = readVal<double>(in);
            note.midiNote = readVal<int32_t>(in);
            note.label = readString(in);
        }
    }

    const int savedNextAssetId = readVal<int32_t>(in);
    const int savedNextPlacementId = readVal<int32_t>(in);

    std::vector<SourceAsset> savedAssets((size_t) readVal<int32_t>(in));
    for (auto& asset : savedAssets)
    {
        asset.assetId = readVal<int32_t>(in);
        asset.name = readString(in);
        asset.kind = (AssetKind) readVal<int32_t>(in);
        if (fileVersion >= 3)
            asset.instrumentName = readString(in);
        asset.clip.sampleRate = readVal<double>(in);
        asset.clip.clipId = asset.assetId;
        asset.clip.name = asset.name;

        int32_t sampleCount = readVal<int32_t>(in);
        if (sampleCount > 0)
        {
            asset.clip.leftChannel.resize((size_t) sampleCount);
            asset.clip.rightChannel.resize((size_t) sampleCount);
            in.read(reinterpret_cast<char*>(asset.clip.leftChannel.data()), sampleCount * (std::streamsize) sizeof(float));
            in.read(reinterpret_cast<char*>(asset.clip.rightChannel.data()), sampleCount * (std::streamsize) sizeof(float));
        }

        asset.patternNotes.resize((size_t) readVal<int32_t>(in));
        for (auto& note : asset.patternNotes)
        {
            note.startSeconds = readVal<double>(in);
            note.lengthSeconds = readVal<double>(in);
            note.midiNote = readVal<int32_t>(in);
            note.velocity = readVal<float>(in);
        }
    }

    std::vector<TimelineTrack> savedTimelineTracks((size_t) readVal<int32_t>(in));
    for (auto& t : savedTimelineTracks)
    {
        t.timelineTrackId = readVal<int32_t>(in);
        t.name = readString(in);
    }

    std::vector<MixerTrack> savedMixerTracks((size_t) readVal<int32_t>(in));
    for (auto& m : savedMixerTracks)
    {
        m.mixerTrackId = readVal<int32_t>(in);
        m.name = readString(in);
        m.gainLinear = readVal<float>(in);
        m.panLinear = readVal<float>(in);
    }

    std::vector<TimelineClipPlacement> savedPlacements((size_t) readVal<int32_t>(in));
    for (auto& p : savedPlacements)
    {
        p.placementId = readVal<int32_t>(in);
        p.assetId = readVal<int32_t>(in);
        p.timelineTrackId = readVal<int32_t>(in);
        p.startSample = (juce::int64) readVal<int64_t>(in);
        p.directToMaster = readVal<bool>(in);
    }

    if (in.fail()) return false;

    MasterTrack savedMasterTrack;
    savedMasterTrack.gainLinear = masterGain;

    arrangement.loadFromSavedData(std::move(savedAssets),
                                  std::move(savedTimelineTracks),
                                  std::move(savedPlacements),
                                  std::move(savedMixerTracks),
                                  savedMasterTrack,
                                  savedNextAssetId,
                                  savedNextPlacementId);

    dawState.transportState = DAWState::TransportState::stopped;
    dawState.playhead = 0.0;
    dawState.isRecording = false;
    dawState.selectedTrackIndex = 0;
    dawState.horizontalScrollOffset = 0;

    return true;
}
