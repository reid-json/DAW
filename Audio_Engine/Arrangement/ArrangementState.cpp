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

CentralTrackSlot* ArrangementState::createCentralTrackSlot()
{
    CentralTrackSlot slot;
    slot.slotId = nextCentralTrackSlotId++;
    slot.name = "Central Track " + juce::String(slot.slotId);
    if (!timelineTracks.empty())
        slot.timelineTrackId = timelineTracks.front().timelineTrackId;

    centralTrackSlots.push_back(slot);
    return &centralTrackSlots.back();
}

bool ArrangementState::assignRecentAssetToCentralTrack(int assetId, int slotId)
{
    return assignAssetToCentralTrack(assetId, slotId);
}

bool ArrangementState::assignAssetToCentralTrack(int assetId, int slotId)
{
    auto* asset = findAsset(assetId);
    auto* slot = const_cast<CentralTrackSlot*>(findCentralTrackSlot(slotId));
    if (asset == nullptr || slot == nullptr)
        return false;

    slot->sourceAssetId = assetId;
    return true;
}

bool ArrangementState::placeCentralTrackOnTimeline(int slotId, int timelineTrackId, juce::int64 startSample)
{
    auto* slot = const_cast<CentralTrackSlot*>(findCentralTrackSlot(slotId));
    if (slot == nullptr || slot->sourceAssetId < 0 || findTimelineTrack(timelineTrackId) == nullptr)
        return false;

    slot->timelineTrackId = timelineTrackId;

    auto it = std::find_if(timelinePlacements.begin(), timelinePlacements.end(),
        [slotId](const TimelineClipPlacement& placement) { return placement.centralTrackSlotId == slotId; });

    if (it == timelinePlacements.end())
    {
        TimelineClipPlacement placement;
        placement.placementId = nextPlacementId++;
        placement.assetId = slot->sourceAssetId;
        placement.timelineTrackId = timelineTrackId;
        placement.startSample = startSample;
        placement.centralTrackSlotId = slotId;
        placement.directToMaster = false;
        timelinePlacements.push_back(placement);
    }
    else
    {
        it->assetId = slot->sourceAssetId;
        it->timelineTrackId = timelineTrackId;
        it->startSample = startSample;
        it->directToMaster = false;
    }

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
    auto it = std::find_if(timelinePlacements.begin(), timelinePlacements.end(),
        [placementId](const TimelineClipPlacement& placement) { return placement.placementId == placementId; });
    if (it == timelinePlacements.end() || findTimelineTrack(timelineTrackId) == nullptr)
        return false;

    it->timelineTrackId = timelineTrackId;
    it->startSample = juce::jmax<juce::int64>(0, startSample);
    return true;
}

bool ArrangementState::removeTimelinePlacement(int placementId)
{
    const auto originalSize = timelinePlacements.size();
    auto it = std::remove_if(timelinePlacements.begin(), timelinePlacements.end(),
        [placementId](const TimelineClipPlacement& placement) { return placement.placementId == placementId; });

    if (it == timelinePlacements.end())
        return false;

    timelinePlacements.erase(it, timelinePlacements.end());
    return timelinePlacements.size() != originalSize;
}

bool ArrangementState::setCentralTrackMixerTrack(int slotId, int mixerTrackId)
{
    auto* slot = const_cast<CentralTrackSlot*>(findCentralTrackSlot(slotId));
    if (slot == nullptr)
        return false;

    slot->mixerTrackId = mixerTrackId;
    return true;
}

bool ArrangementState::setCentralTrackTimelineTrack(int slotId, int timelineTrackId)
{
    auto* slot = const_cast<CentralTrackSlot*>(findCentralTrackSlot(slotId));
    if (slot == nullptr || findTimelineTrack(timelineTrackId) == nullptr)
        return false;

    slot->timelineTrackId = timelineTrackId;
    return true;
}

bool ArrangementState::setCentralTrackOutputToMaster(int slotId)
{
    auto* slot = const_cast<CentralTrackSlot*>(findCentralTrackSlot(slotId));
    if (slot == nullptr)
        return false;

    slot->outputTarget = CentralTrackSlot::OutputTarget::master;
    slot->outputCentralTrackId = -1;
    return true;
}

bool ArrangementState::setCentralTrackOutputToTrack(int slotId, int destinationSlotId)
{
    auto* slot = const_cast<CentralTrackSlot*>(findCentralTrackSlot(slotId));
    if (slot == nullptr || findCentralTrackSlot(destinationSlotId) == nullptr || slotId == destinationSlotId)
        return false;

    slot->outputTarget = CentralTrackSlot::OutputTarget::centralTrack;
    slot->outputCentralTrackId = destinationSlotId;
    return true;
}

bool ArrangementState::setCentralTrackLiveInputArmed(int slotId, bool shouldArm)
{
    auto* slot = const_cast<CentralTrackSlot*>(findCentralTrackSlot(slotId));
    if (slot == nullptr)
        return false;

    slot->liveInputArmed = shouldArm;
    return true;
}

void ArrangementState::setPatternTrackRenderer(PatternTrackRenderer renderer)
{
    patternTrackRenderer = std::move(renderer);
    activePatternNotesByTrack.clear();
    lastPatternRenderEndSample = -1;
}

void ArrangementState::render(juce::AudioBuffer<float>& buffer, juce::int64 playbackStartSample) const
{
    const auto blockEndSample = playbackStartSample + buffer.getNumSamples();
    const bool patternRenderDiscontinuity = lastPatternRenderEndSample != playbackStartSample;

    if (patternRenderDiscontinuity)
        activePatternNotesByTrack.clear();

    std::map<int, juce::MidiBuffer> midiByTrack;
    std::map<int, float> gainByTrack;
    std::map<int, double> sampleRateByTrack;
    std::map<int, bool> trackHasPatternFallback;

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
        if (!placement.directToMaster && placement.centralTrackSlotId >= 0)
        {
            std::vector<int> visitedSlotIds;
            gain *= resolveRouteGainForSlot(placement.centralTrackSlotId, visitedSlotIds);
        }

        if (asset->kind == AssetKind::pianoRollPattern && !asset->patternNotes.empty())
        {
            const int trackIndex = juce::jmax(0, placement.timelineTrackId - 1);
            gainByTrack.emplace(trackIndex, gain);
            sampleRateByTrack.emplace(trackIndex, asset->clip.sampleRate > 0.0 ? asset->clip.sampleRate : 44100.0);

            auto& midi = midiByTrack[trackIndex];
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

            trackHasPatternFallback[trackIndex] = true;
            continue;
        }

        buffer.addFrom(0, bufferOffset, clip.leftChannel.data() + clipOffset, overlapLength, gain);
        buffer.addFrom(1, bufferOffset, clip.rightChannel.data() + clipOffset, overlapLength, gain);
    }

    for (auto& [trackIndex, midi] : midiByTrack)
    {
        juce::AudioBuffer<float> tempBuffer(2, buffer.getNumSamples());
        tempBuffer.clear();

        const auto gain = gainByTrack.count(trackIndex) > 0 ? gainByTrack[trackIndex] : 1.0f;
        const auto sampleRate = sampleRateByTrack.count(trackIndex) > 0 ? sampleRateByTrack[trackIndex] : 44100.0;
        const bool renderedByPlugin = patternTrackRenderer != nullptr
            && patternTrackRenderer(trackIndex, tempBuffer, midi, sampleRate);

        if (renderedByPlugin)
        {
            tempBuffer.applyGain(gain);
            buffer.addFrom(0, 0, tempBuffer, 0, 0, buffer.getNumSamples());
            buffer.addFrom(1, 0, tempBuffer, 1, 0, buffer.getNumSamples());
            trackHasPatternFallback[trackIndex] = false;
        }
    }

    for (const auto& placement : timelinePlacements)
    {
        const auto* asset = findAsset(placement.assetId);
        if (asset == nullptr || asset->kind != AssetKind::pianoRollPattern || asset->patternNotes.empty())
            continue;

        const int trackIndex = juce::jmax(0, placement.timelineTrackId - 1);
        const auto fallbackIt = trackHasPatternFallback.find(trackIndex);
        if (fallbackIt != trackHasPatternFallback.end() && !fallbackIt->second)
            continue;

        float gain = masterTrack.gainLinear;
        if (!placement.directToMaster && placement.centralTrackSlotId >= 0)
        {
            std::vector<int> visitedSlotIds;
            gain *= resolveRouteGainForSlot(placement.centralTrackSlotId, visitedSlotIds);
        }

        renderPatternAsset(*asset, placement, buffer, playbackStartSample, gain);
    }

    lastPatternRenderEndSample = blockEndSample;
}

const SourceAsset* ArrangementState::findAsset(int assetId) const
{
    auto it = std::find_if(assets.begin(), assets.end(),
        [assetId](const SourceAsset& asset) { return asset.assetId == assetId; });
    return it != assets.end() ? &(*it) : nullptr;
}

SourceAsset* ArrangementState::findAsset(int assetId)
{
    auto it = std::find_if(assets.begin(), assets.end(),
        [assetId](const SourceAsset& asset) { return asset.assetId == assetId; });
    return it != assets.end() ? &(*it) : nullptr;
}

const CentralTrackSlot* ArrangementState::findCentralTrackSlot(int slotId) const
{
    auto it = std::find_if(centralTrackSlots.begin(), centralTrackSlots.end(),
        [slotId](const CentralTrackSlot& slot) { return slot.slotId == slotId; });
    return it != centralTrackSlots.end() ? &(*it) : nullptr;
}

const TimelineTrack* ArrangementState::findTimelineTrack(int timelineTrackId) const
{
    auto it = std::find_if(timelineTracks.begin(), timelineTracks.end(),
        [timelineTrackId](const TimelineTrack& track) { return track.timelineTrackId == timelineTrackId; });
    return it != timelineTracks.end() ? &(*it) : nullptr;
}

const TimelineClipPlacement* ArrangementState::findTimelinePlacement(int placementId) const
{
    auto it = std::find_if(timelinePlacements.begin(), timelinePlacements.end(),
        [placementId](const TimelineClipPlacement& placement) { return placement.placementId == placementId; });
    return it != timelinePlacements.end() ? &(*it) : nullptr;
}

float ArrangementState::resolveRouteGainForSlot(int slotId, std::vector<int>& visitedSlotIds) const
{
    if (std::find(visitedSlotIds.begin(), visitedSlotIds.end(), slotId) != visitedSlotIds.end())
        return 1.0f;

    visitedSlotIds.push_back(slotId);
    const auto* slot = findCentralTrackSlot(slotId);
    if (slot == nullptr)
        return 1.0f;

    float gain = 1.0f;
    if (slot->mixerTrackId > 0)
    {
        auto mixerIt = std::find_if(mixerTracks.begin(), mixerTracks.end(),
            [slot](const MixerTrack& track) { return track.mixerTrackId == slot->mixerTrackId; });
        if (mixerIt != mixerTracks.end())
            gain *= mixerIt->gainLinear;
    }

    if (slot->outputTarget == CentralTrackSlot::OutputTarget::centralTrack && slot->outputCentralTrackId > 0)
        gain *= resolveRouteGainForSlot(slot->outputCentralTrackId, visitedSlotIds);

    return gain;
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
