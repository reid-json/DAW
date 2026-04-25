#include "ArrangementState.h"
#include <algorithm>
#include <cmath>
#include <map>

void ArrangementState::initialiseDefaults(int numMixerTracks, int numTimelineTracks)
{
    if (!mixerTracks.empty() || !timelineTracks.empty())
        return;

    for (int i = 0; i < numMixerTracks; ++i)
        mixerTracks.push_back({ i + 1, "Mixer " + juce::String(i + 1), 1.0f });

    for (int i = 0; i < numTimelineTracks; ++i)
        timelineTracks.push_back({ i + 1, "Timeline " + juce::String(i + 1) });
}

void ArrangementState::loadFromSavedData(std::vector<SourceAsset> savedAssets,
                                         std::vector<TimelineTrack> savedTracks,
                                         std::vector<TimelineClipPlacement> savedPlacements,
                                         std::vector<MixerTrack> savedMixerTracks,
                                         MasterTrack savedMaster,
                                         int savedNextAssetId,
                                         int savedNextPlacementId)
{
    assets = std::move(savedAssets);
    timelineTracks = std::move(savedTracks);
    timelinePlacements = std::move(savedPlacements);
    mixerTracks = std::move(savedMixerTracks);
    masterTrack = savedMaster;
    nextAssetId = savedNextAssetId;
    nextPlacementId = savedNextPlacementId;

    recentAssetIds.clear();
    for (const auto& a : assets)
        recentAssetIds.push_back(a.assetId);

    activePatternNotesByTrack.clear();
    lastPatternRenderEndSample = -1;
}

SourceAsset* ArrangementState::addAsset(const juce::String& name, AssetKind kind, const RecordedClip& clip)
{
    SourceAsset asset;
    asset.assetId = nextAssetId++;
    asset.name = name;
    asset.kind = kind;
    asset.clip = clip;
    asset.clip.clipId = asset.assetId;
    assets.push_back(asset);
    recentAssetIds.push_back(asset.assetId);
    return &assets.back();
}

SourceAsset* ArrangementState::addRecentRecording(const RecordedClip& clip)
{
    return addAsset(clip.name, AssetKind::recording, clip);
}

bool ArrangementState::renameAsset(int assetId, const juce::String& newName)
{
    auto* asset = findAsset(assetId);
    if (asset == nullptr)
        return false;

    asset->name = newName;
    asset->clip.name = newName;
    return true;
}

bool ArrangementState::updateAssetClip(int assetId, const RecordedClip& clip)
{
    auto* asset = findAsset(assetId);
    if (asset == nullptr)
        return false;

    asset->clip = clip;
    asset->clip.clipId = assetId;
    return true;
}

bool ArrangementState::placeRecentAssetDirectToTimeline(int assetId, int timelineTrackId, juce::int64 startSample)
{
    auto* asset = findAsset(assetId);
    if (asset == nullptr || findTimelineTrack(timelineTrackId) == nullptr)
        return false;

    TimelineClipPlacement placement;
    placement.placementId = nextPlacementId++;
    placement.assetId = assetId;
    placement.timelineTrackId = timelineTrackId;
    placement.startSample = startSample;
    placement.directToMaster = true;
    timelinePlacements.push_back(placement);
    return true;
}

bool ArrangementState::moveTimelinePlacement(int placementId, int timelineTrackId, juce::int64 startSample)
{
    if (findTimelineTrack(timelineTrackId) == nullptr)
        return false;

    for (auto& placement : timelinePlacements)
    {
        if (placement.placementId == placementId)
        {
            placement.timelineTrackId = timelineTrackId;
            placement.startSample = juce::jmax<juce::int64>(0, startSample);
            return true;
        }
    }

    return false;
}

bool ArrangementState::removeTimelinePlacement(int placementId)
{
    auto it = std::remove_if(timelinePlacements.begin(), timelinePlacements.end(),
        [placementId](auto& p) { return p.placementId == placementId; });
    if (it == timelinePlacements.end()) return false;
    timelinePlacements.erase(it, timelinePlacements.end());
    return true;
}

void ArrangementState::setPatternTrackRenderer(PatternTrackRenderer renderer)
{
    patternTrackRenderer = std::move(renderer);
    activePatternNotesByTrack.clear();
    lastPatternRenderEndSample = -1;
}

void ArrangementState::setMixerTrackGain(int i, float gain)
{
    if (i >= 0 && i < (int) mixerTracks.size()) mixerTracks[(size_t) i].gainLinear = gain;
}

void ArrangementState::setMixerTrackPan(int i, float pan)
{
    if (i >= 0 && i < (int) mixerTracks.size()) mixerTracks[(size_t) i].panLinear = juce::jlimit(-1.0f, 1.0f, pan);
}

void ArrangementState::setMasterGain(float gain) { masterTrack.gainLinear = gain; }

void ArrangementState::render(juce::AudioBuffer<float>& buffer, juce::int64 playbackStartSample) const
{
    struct PatternRenderKey
    {
        int trackIndex = 0;
        juce::String instrumentName;

        bool operator<(const PatternRenderKey& other) const
        {
            if (trackIndex != other.trackIndex) return trackIndex < other.trackIndex;
            return instrumentName < other.instrumentName;
        }
    };

    const auto blockEndSample = playbackStartSample + buffer.getNumSamples();
    const bool patternRenderDiscontinuity = lastPatternRenderEndSample != playbackStartSample;

    if (patternRenderDiscontinuity)
        activePatternNotesByTrack.clear();

    std::map<PatternRenderKey, juce::MidiBuffer> midiByPatternRenderer;
    std::map<PatternRenderKey, float> gainByPatternRenderer;
    std::map<PatternRenderKey, float> panByPatternRenderer;
    std::map<PatternRenderKey, double> sampleRateByPatternRenderer;
    std::map<PatternRenderKey, bool> patternRendererHasFallback;

    for (const auto& placement : timelinePlacements)
    {
        const auto* asset = findAsset(placement.assetId);
        if (asset == nullptr)
            continue;

        const auto& clip = asset->clip;
        const auto clipStart = placement.startSample;
        const auto clipEnd = placement.startSample + clip.getNumSamples();

        if (clipEnd <= playbackStartSample || clipStart >= blockEndSample)
            continue;

        const auto overlapStart = juce::jmax(playbackStartSample, clipStart);
        const auto overlapEnd = juce::jmin(blockEndSample, clipEnd);
        const auto overlapLength = static_cast<int>(overlapEnd - overlapStart);
        const auto bufferOffset = static_cast<int>(overlapStart - playbackStartSample);
        const auto clipOffset = static_cast<int>(overlapStart - clipStart);

        if (overlapLength <= 0)
            continue;

        float gain = masterTrack.gainLinear;
        const int mixTrackIdx = juce::jmax(0, placement.timelineTrackId - 1);
        if (mixTrackIdx >= 0 && mixTrackIdx < static_cast<int>(mixerTracks.size()))
            gain *= mixerTracks[static_cast<size_t>(mixTrackIdx)].gainLinear;

        if (asset->kind == AssetKind::pianoRollPattern && !asset->patternNotes.empty())
        {
            const int trackIndex = juce::jmax(0, placement.timelineTrackId - 1);
            const PatternRenderKey renderKey { trackIndex, asset->instrumentName };
            gainByPatternRenderer.emplace(renderKey, gain);
            if (mixTrackIdx >= 0 && mixTrackIdx < static_cast<int>(mixerTracks.size()))
                panByPatternRenderer.emplace(renderKey, mixerTracks[static_cast<size_t>(mixTrackIdx)].panLinear);
            sampleRateByPatternRenderer.emplace(renderKey, asset->clip.sampleRate > 0.0 ? asset->clip.sampleRate : 44100.0);

            auto& midi = midiByPatternRenderer[renderKey];
            auto& activeNotes = activePatternNotesByTrack[trackIndex];

            for (int noteIndex = 0; noteIndex < static_cast<int>(asset->patternNotes.size()); ++noteIndex)
            {
                const auto& note = asset->patternNotes[static_cast<size_t>(noteIndex)];
                const auto noteStartSample = placement.startSample + static_cast<juce::int64>(std::llround(note.startSeconds * asset->clip.sampleRate));
                const auto noteLengthSamples = static_cast<juce::int64>(std::llround(juce::jmax(0.01, note.lengthSeconds) * asset->clip.sampleRate));
                const auto noteEndSample = noteStartSample + noteLengthSamples;
                const ActivePatternNote key { placement.placementId, noteIndex };
                const bool isActive = activeNotes.find(key) != activeNotes.end();

                if (noteEndSample <= playbackStartSample || noteStartSample >= blockEndSample)
                    continue;

                if (asset->instrumentName.containsIgnoreCase("drum"))
                {
                    if (noteStartSample >= playbackStartSample && noteStartSample < blockEndSample)
                    {
                        const auto offset = static_cast<int>(noteStartSample - playbackStartSample);
                        midi.addEvent(juce::MidiMessage::noteOn(1, note.midiNote, note.velocity), offset);
                        midi.addEvent(juce::MidiMessage::noteOff(1, note.midiNote),
                                      juce::jmin(buffer.getNumSamples() - 1, offset + 96));
                    }
                    else if (patternRenderDiscontinuity && noteStartSample < playbackStartSample && noteEndSample > playbackStartSample)
                    {
                        midi.addEvent(juce::MidiMessage::noteOn(1, note.midiNote, note.velocity), 0);
                        midi.addEvent(juce::MidiMessage::noteOff(1, note.midiNote),
                                      juce::jmin(buffer.getNumSamples() - 1, 96));
                    }
                    continue;
                }

                if (noteStartSample >= playbackStartSample && noteStartSample < blockEndSample)
                {
                    midi.addEvent(juce::MidiMessage::noteOn(1, note.midiNote, note.velocity),
                                  static_cast<int>(noteStartSample - playbackStartSample));
                    activeNotes.insert(key);
                }
                else if ((patternRenderDiscontinuity || !isActive) && noteStartSample < playbackStartSample && noteEndSample > playbackStartSample)
                {
                    midi.addEvent(juce::MidiMessage::noteOn(1, note.midiNote, note.velocity), 0);
                    activeNotes.insert(key);
                }

                if (noteEndSample > playbackStartSample && noteEndSample <= blockEndSample)
                {
                    midi.addEvent(juce::MidiMessage::noteOff(1, note.midiNote),
                                  static_cast<int>(noteEndSample - playbackStartSample));
                    activeNotes.erase(key);
                }
            }

            patternRendererHasFallback[renderKey] = true;
            continue;
        }

        float trackPan = 0.0f;
        if (mixTrackIdx >= 0 && mixTrackIdx < static_cast<int>(mixerTracks.size()))
            trackPan = mixerTracks[static_cast<size_t>(mixTrackIdx)].panLinear;
        const float gainL = gain * juce::jmin(1.0f, 1.0f - trackPan);
        const float gainR = gain * juce::jmin(1.0f, 1.0f + trackPan);
        buffer.addFrom(0, bufferOffset, clip.leftChannel.data() + clipOffset, overlapLength, gainL);
        buffer.addFrom(1, bufferOffset, clip.rightChannel.data() + clipOffset, overlapLength, gainR);
    }

    for (auto& entry : midiByPatternRenderer)
    {
        const auto& renderKey = entry.first;
        auto& midi = entry.second;

        juce::AudioBuffer<float> tempBuffer(2, buffer.getNumSamples());
        tempBuffer.clear();

        const auto gain = gainByPatternRenderer.count(renderKey) > 0 ? gainByPatternRenderer[renderKey] : 1.0f;
        const auto sampleRate = sampleRateByPatternRenderer.count(renderKey) > 0 ? sampleRateByPatternRenderer[renderKey] : 44100.0;
        const bool renderedByPlugin = patternTrackRenderer != nullptr
            && patternTrackRenderer(renderKey.trackIndex, renderKey.instrumentName, tempBuffer, midi, sampleRate);

        if (renderedByPlugin)
        {
            const float pan = panByPatternRenderer.count(renderKey) > 0 ? panByPatternRenderer[renderKey] : 0.0f;
            const float gL = gain * juce::jmin(1.0f, 1.0f - pan);
            const float gR = gain * juce::jmin(1.0f, 1.0f + pan);
            tempBuffer.applyGain(0, 0, tempBuffer.getNumSamples(), gL);
            tempBuffer.applyGain(1, 0, tempBuffer.getNumSamples(), gR);
            buffer.addFrom(0, 0, tempBuffer, 0, 0, buffer.getNumSamples());
            buffer.addFrom(1, 0, tempBuffer, 1, 0, buffer.getNumSamples());
            patternRendererHasFallback[renderKey] = false;
        }
    }

    for (const auto& placement : timelinePlacements)
    {
        const auto* asset = findAsset(placement.assetId);
        if (asset == nullptr || asset->kind != AssetKind::pianoRollPattern || asset->patternNotes.empty())
            continue;

        const int trackIndex = juce::jmax(0, placement.timelineTrackId - 1);
        const PatternRenderKey renderKey { trackIndex, asset->instrumentName };
        const auto fallbackIt = patternRendererHasFallback.find(renderKey);
        if (fallbackIt != patternRendererHasFallback.end() && !fallbackIt->second)
            continue;

        float gain = masterTrack.gainLinear;
        if (trackIndex >= 0 && trackIndex < static_cast<int>(mixerTracks.size()))
            gain *= mixerTracks[static_cast<size_t>(trackIndex)].gainLinear;

        renderPatternAsset(*asset, placement, buffer, playbackStartSample, gain);
    }

    lastPatternRenderEndSample = blockEndSample;
}

const SourceAsset* ArrangementState::findAsset(int id) const
{
    for (auto& a : assets)
        if (a.assetId == id)
            return &a;
    return nullptr;
}

SourceAsset* ArrangementState::findAsset(int id)
{
    for (auto& a : assets)
        if (a.assetId == id)
            return &a;
    return nullptr;
}

const TimelineTrack* ArrangementState::findTimelineTrack(int id) const
{
    for (auto& t : timelineTracks)
        if (t.timelineTrackId == id)
            return &t;
    return nullptr;
}

const TimelineClipPlacement* ArrangementState::findTimelinePlacement(int id) const
{
    for (auto& p : timelinePlacements)
        if (p.placementId == id)
            return &p;
    return nullptr;
}

void ArrangementState::renderPatternAsset(const SourceAsset& asset,
                                          const TimelineClipPlacement& placement,
                                          juce::AudioBuffer<float>& buffer,
                                          juce::int64 playbackStartSample,
                                          float gain) const
{
    const auto sampleRate = asset.clip.sampleRate > 0.0 ? asset.clip.sampleRate : 44100.0;
    const auto blockEndSample = playbackStartSample + buffer.getNumSamples();

    for (const auto& note : asset.patternNotes)
    {
        const auto noteStartSample = placement.startSample + static_cast<juce::int64>(std::llround(note.startSeconds * sampleRate));
        const auto noteLengthSamples = static_cast<juce::int64>(std::llround(juce::jmax(0.01, note.lengthSeconds) * sampleRate));
        const auto noteEndSample = noteStartSample + noteLengthSamples;

        if (noteEndSample <= playbackStartSample || noteStartSample >= blockEndSample)
            continue;

        const auto overlapStart = juce::jmax(playbackStartSample, noteStartSample);
        const auto overlapEnd = juce::jmin(blockEndSample, noteEndSample);
        const auto overlapLength = static_cast<int>(overlapEnd - overlapStart);
        const auto bufferOffset = static_cast<int>(overlapStart - playbackStartSample);
        const auto noteOffsetSamples = static_cast<int>(overlapStart - noteStartSample);

        if (overlapLength <= 0)
            continue;

        const auto frequency = juce::MidiMessage::getMidiNoteInHertz(note.midiNote);
        const auto angularFrequency = 2.0 * juce::MathConstants<double>::pi * frequency;
        const auto attackSeconds = 0.01;
        const auto releaseSeconds = 0.08;

        for (int sampleIndex = 0; sampleIndex < overlapLength; ++sampleIndex)
        {
            const auto absoluteSample = noteOffsetSamples + sampleIndex;
            const auto timeSeconds = static_cast<double>(absoluteSample) / sampleRate;
            const auto noteProgressSeconds = static_cast<double>(absoluteSample) / sampleRate;
            const auto noteRemainingSeconds = juce::jmax(0.0, note.lengthSeconds - noteProgressSeconds);

            double envelope = 1.0;
            if (timeSeconds < attackSeconds)
                envelope = timeSeconds / attackSeconds;
            else if (noteRemainingSeconds < releaseSeconds)
                envelope = noteRemainingSeconds / releaseSeconds;

            envelope = juce::jlimit(0.0, 1.0, envelope);

            const auto phase = angularFrequency * timeSeconds;
            const float sampleValue = static_cast<float>(
                (0.55 * std::sin(phase))
                + (0.18 * std::sin(phase * 2.0))
                + (0.08 * std::sin(phase * 3.0)));
            const float output = sampleValue * note.velocity * static_cast<float>(envelope) * gain * 0.25f;

            buffer.addSample(0, bufferOffset + sampleIndex, output);
            buffer.addSample(1, bufferOffset + sampleIndex, output);
        }
    }
}
