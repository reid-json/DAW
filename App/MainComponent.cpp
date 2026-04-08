#include "MainComponent.h"
#include <algorithm>
#include <cmath>

namespace
{
TimelineClipItem::ContentKind toTimelineContentKind(AssetKind assetKind)
{
    return assetKind == AssetKind::pianoRollPattern
        ? TimelineClipItem::ContentKind::pattern
        : TimelineClipItem::ContentKind::recording;
}

std::vector<PatternNote> toPatternNotes(const DAWState& state, const TrackPatternState& pattern)
{
    std::vector<PatternNote> patternNotes;
    patternNotes.reserve(pattern.notes.size());

    const auto secondsPerBeat = 60.0 / juce::jmax(1.0, state.tempoBpm);
    for (const auto& note : pattern.notes)
    {
        PatternNote patternNote;
        patternNote.startSeconds = juce::jmax(0.0, note.startBeat * secondsPerBeat);
        patternNote.lengthSeconds = juce::jmax(0.01, note.lengthBeats * secondsPerBeat);
        patternNote.midiNote = note.midiNote;
        patternNote.velocity = 0.85f;
        patternNotes.push_back(patternNote);
    }

    return patternNotes;
}
}

MainComponent::MainComponent()
{
    engine.initialise(2, 2);

    gui = std::make_unique<GUIComponent>(engine.getDeviceManager());
    engine.setPatternTrackRenderer([this] (int trackIndex,
                                           juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer& midi,
                                           double sampleRate)
    {
        return gui != nullptr
            && gui->getPluginHostManager().renderTrackInstrument(trackIndex, buffer, midi, sampleRate);
    });
    gui->onRecordToggleRequested = [this] { handleRecordToggle(); };
    gui->onMonitoringToggleRequested = [this] { handleMonitoringToggle(); };
    gui->onImportAudioRequested = [this] { handleImportAudio(); };
    gui->onPlayRequested = [this] { handlePlay(); };
    gui->onStopRequested = [this] { handleStop(); };
    gui->onPauseRequested = [this] { handlePause(); };
    gui->onRestartRequested = [this] { handleRestart(); };
    gui->onRecentClipDropped = [this] (int assetId, int trackIndex, double startSeconds)
    {
        handleRecentClipDropped(assetId, trackIndex, startSeconds);
    };
    gui->onAssetRenameRequested = [this] (int assetId, const juce::String& newName)
    {
        handleAssetRenamed(assetId, newName);
    };
    gui->onTimelineClipMoved = [this] (int placementId, int trackIndex, double startSeconds)
    {
        handleTimelineClipMoved(placementId, trackIndex, startSeconds);
    };
    gui->onTimelineClipDeleteRequested = [this] (int placementId)
    {
        handleTimelineClipRemoved(placementId);
    };

    addAndMakeVisible(gui.get());
    setSize(1440, 700);
    startTimerHz(30);
    syncStateFromEngine();
}

MainComponent::~MainComponent()
{
    stopTimer();
    engine.shutdown();
}

void MainComponent::resized()
{
    if (gui != nullptr)
        gui->setBounds(getLocalBounds());
}

void MainComponent::timerCallback()
{
    syncStateFromEngine();
}

void MainComponent::syncStateFromEngine()
{
    if (gui == nullptr)
        return;

    syncTrackPatternsToAssets();

    std::vector<RecentClipItem> recentClips;
    std::vector<SavedPatternItem> savedPatterns;
    std::vector<TimelineClipItem> timelineClips;
    auto transportState = DAWState::TransportState::stopped;
    double playhead = 0.0;
    int maxTrackIndex = 0;

    {
        juce::ScopedLock audioLock(engine.getDeviceManager().getAudioCallbackLock());
        const auto& arrangement = engine.getArrangementState();
        const auto timelineSampleRate = engine.getTimelineSampleRate();

        if (engine.isPlaybackPaused())
            transportState = DAWState::TransportState::paused;
        else if (engine.isPlaying())
            transportState = DAWState::TransportState::playing;

        playhead = engine.getCurrentTransportPositionSeconds();

        recentClips.reserve(arrangement.getRecentAssetIds().size());
        for (const auto assetId : arrangement.getRecentAssetIds())
        {
            if (const auto* asset = arrangement.findAsset(assetId))
            {
                if (asset->kind == AssetKind::pianoRollPattern)
                    continue;

                recentClips.push_back({ asset->assetId, asset->name, asset->clip.getLengthSeconds() });
            }
        }

        timelineClips.reserve(arrangement.getTimelinePlacements().size());
        for (const auto& placement : arrangement.getTimelinePlacements())
        {
            if (const auto* asset = arrangement.findAsset(placement.assetId))
            {
                const int trackIndex = juce::jmax(0, placement.timelineTrackId - 1);
                maxTrackIndex = juce::jmax(maxTrackIndex, trackIndex);
                timelineClips.push_back({
                    placement.placementId,
                    placement.assetId,
                    asset->name,
                    toTimelineContentKind(asset->kind),
                    trackIndex,
                    placement.startSample / timelineSampleRate,
                    asset->clip.getLengthSeconds()
                });
            }
        }
    }

    auto& state = gui->getState();
    savedPatterns.reserve(state.trackPatternStates.size());
    for (int trackIndex = 0; trackIndex < state.trackCount; ++trackIndex)
    {
        if (! state.isTrackPatternMode(trackIndex))
            continue;

        const auto& pattern = state.getTrackPatternState(trackIndex);
        if (pattern.assetId > 0 && ! pattern.notes.empty())
            savedPatterns.push_back({ pattern.assetId, trackIndex, state.getTrackName(trackIndex), getPatternLengthSeconds(state, pattern) });
    }

    const auto previousTransportState = state.transportState;
    const auto previousRecordingState = state.isRecording;
    const auto previousMonitoringState = state.audioMonitoringEnabled;
    const auto previousTrackCount = state.trackCount;
    const bool recentClipsChanged = state.recentClips != recentClips;
    const bool savedPatternsChanged = state.savedPatterns != savedPatterns;
    const bool timelineClipsChanged = state.timelineClips != timelineClips;

    state.isRecording = engine.isRecording();
    state.audioMonitoringEnabled = engine.isInputMonitoringEnabled();
    state.transportState = transportState;
    state.playhead = playhead;
    state.recentClips = std::move(recentClips);
    state.savedPatterns = std::move(savedPatterns);
    state.timelineClips = std::move(timelineClips);
    state.trackCount = juce::jmax(state.trackCount, maxTrackIndex + 1, 1);

    const bool controlsChanged = previousTransportState != state.transportState
        || previousRecordingState != state.isRecording
        || previousMonitoringState != state.audioMonitoringEnabled;
    const bool trackCountChanged = previousTrackCount != state.trackCount;

    gui->refreshExternalState(controlsChanged, trackCountChanged);

    if (!controlsChanged && !trackCountChanged && !recentClipsChanged && !savedPatternsChanged && !timelineClipsChanged)
        gui->repaintDynamicViews();
}

void MainComponent::placeNewestRecentAssetOnNextEmptyTrack()
{
    if (gui == nullptr)
        return;

    const int newestAssetId = getNewestRecentAssetId();
    if (newestAssetId <= 0)
        return;

    auto& state = gui->getState();
    int nextTrackIndex = state.trackCount;
    for (int trackIndex = 0; trackIndex < state.trackCount; ++trackIndex)
    {
        const bool isEmpty = std::none_of(state.timelineClips.begin(), state.timelineClips.end(),
                                          [trackIndex](const TimelineClipItem& clip) { return clip.trackIndex == trackIndex; });
        if (isEmpty && state.getTrackContentType(trackIndex) == TrackMixerState::ContentType::recording)
        {
            nextTrackIndex = trackIndex;
            break;
        }
    }

    if (nextTrackIndex >= state.trackCount)
        nextTrackIndex = state.findNextEmptyTrackIndex();

    engine.placeRecentAssetDirectToTimeline(newestAssetId, nextTrackIndex + 1, 0);
}

void MainComponent::handleRecordToggle()
{
    if (!engine.isRecording())
        engine.startRecording();

    syncStateFromEngine();
}

void MainComponent::handleMonitoringToggle()
{
    engine.setInputMonitoringEnabled(!engine.isInputMonitoringEnabled());
    syncStateFromEngine();
}

void MainComponent::handleImportAudio()
{
    audioFileChooser = std::make_unique<juce::FileChooser>(
        "Choose an audio file",
        juce::File(),
        "*.wav;*.mp3;*.aif;*.aiff;*.flac");

    audioFileChooser->launchAsync(juce::FileBrowserComponent::openMode
            | juce::FileBrowserComponent::canSelectFiles
            | juce::FileBrowserComponent::canSelectMultipleItems,
        [this](const juce::FileChooser& chooser)
        {
            for (const auto& file : chooser.getResults())
            {
                if (file.existsAsFile())
                    engine.importAudioFileAsRecentAsset(file);
            }

            audioFileChooser.reset();
            syncStateFromEngine();
        });
}

void MainComponent::handlePlay()
{
    engine.startPlayback();
    syncStateFromEngine();
}

void MainComponent::handleStop()
{
    if (engine.isRecording())
    {
        engine.stopRecording();
        placeNewestRecentAssetOnNextEmptyTrack();
    }

    engine.stopPlayback();
    syncStateFromEngine();
}

void MainComponent::handlePause()
{
    engine.togglePlaybackPause();
    syncStateFromEngine();
}

void MainComponent::handleRestart()
{
    engine.stopPlayback();
    syncStateFromEngine();
}

void MainComponent::handleRecentClipDropped(int assetId, int trackIndex, double startSeconds)
{
    const auto& arrangement = engine.getArrangementState();
    const auto* asset = arrangement.findAsset(assetId);
    if (asset == nullptr)
        return;

    const auto& state = gui->getState();
    if (! state.canTrackAcceptTimelineKind(trackIndex, toTimelineContentKind(asset->kind)))
        return;

    const auto sampleRate = engine.getTimelineSampleRate();
    engine.placeRecentAssetDirectToTimeline(assetId,
                                            trackIndex + 1,
                                            static_cast<juce::int64>(std::llround(startSeconds * sampleRate)));
    syncStateFromEngine();
}

void MainComponent::handleAssetRenamed(int assetId, const juce::String& newName)
{
    const auto trimmedName = newName.trim();
    if (trimmedName.isEmpty())
        return;

    auto& state = gui->getState();
    for (int trackIndex = 0; trackIndex < state.trackCount; ++trackIndex)
    {
        auto& pattern = state.getTrackPatternState(trackIndex);
        if (pattern.assetId == assetId)
        {
            state.setTrackName(trackIndex, trimmedName);
            state.setTrackPatternName(trackIndex, trimmedName);
            engine.updatePatternAsset(assetId, trimmedName, getPatternLengthSeconds(state, pattern), toPatternNotes(state, pattern));
            syncStateFromEngine();
            return;
        }
    }

    if (engine.renameAsset(assetId, trimmedName))
        syncStateFromEngine();
}

void MainComponent::syncTrackPatternsToAssets()
{
    if (gui == nullptr)
        return;

    auto& state = gui->getState();

    for (int trackIndex = 0; trackIndex < state.trackCount; ++trackIndex)
    {
        if (! state.isTrackPatternMode(trackIndex))
            continue;

        auto& pattern = state.getTrackPatternState(trackIndex);

        if (pattern.notes.empty())
            continue;

        const auto patternName = state.getTrackName(trackIndex);
        state.setTrackPatternName(trackIndex, patternName);
        const auto lengthSeconds = getPatternLengthSeconds(state, pattern);
        auto patternNotes = toPatternNotes(state, pattern);

        if (pattern.assetId <= 0)
            pattern.assetId = engine.createPatternAsset(patternName, lengthSeconds, std::move(patternNotes));
        else
            engine.updatePatternAsset(pattern.assetId, patternName, lengthSeconds, std::move(patternNotes));
    }
}

double MainComponent::getPatternLengthSeconds(const DAWState& state, const TrackPatternState& pattern)
{
    double endBeat = 1.0;

    for (const auto& note : pattern.notes)
        endBeat = juce::jmax(endBeat, note.startBeat + note.lengthBeats);

    const auto secondsPerBeat = 60.0 / juce::jmax(1.0, state.tempoBpm);
    return juce::jmax(0.25, endBeat * secondsPerBeat);
}

void MainComponent::handleTimelineClipMoved(int placementId, int trackIndex, double startSeconds)
{
    const auto& arrangement = engine.getArrangementState();
    const auto* placement = arrangement.findTimelinePlacement(placementId);
    const auto* asset = placement != nullptr ? arrangement.findAsset(placement->assetId) : nullptr;
    if (asset == nullptr)
        return;

    const auto& state = gui->getState();
    if (! state.canTrackAcceptTimelineKind(trackIndex, toTimelineContentKind(asset->kind)))
        return;

    const auto sampleRate = engine.getTimelineSampleRate();
    engine.moveTimelinePlacement(placementId,
                                 trackIndex + 1,
                                 static_cast<juce::int64>(std::llround(startSeconds * sampleRate)));
    syncStateFromEngine();
}

void MainComponent::handleTimelineClipRemoved(int placementId)
{
    engine.removeTimelinePlacement(placementId);
    syncStateFromEngine();
}

int MainComponent::getNewestRecentAssetId() const
{
    const auto& recent = engine.getArrangementState().getRecentAssetIds();
    return recent.empty() ? -1 : recent.back();
}
