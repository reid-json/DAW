#include "MainComponent.h"
#include <cmath>

MainComponent::MainComponent()
{
    engine.initialise(2, 2);

    gui = std::make_unique<GUIComponent>(engine.getDeviceManager());
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

    std::vector<RecentClipItem> recentClips;
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
                recentClips.push_back({ asset->assetId, asset->name, asset->clip.getLengthSeconds() });
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
                    trackIndex,
                    placement.startSample / timelineSampleRate,
                    asset->clip.getLengthSeconds()
                });
            }
        }
    }

    auto& state = gui->getState();
    const auto previousTransportState = state.transportState;
    const auto previousRecordingState = state.isRecording;
    const auto previousMonitoringState = state.audioMonitoringEnabled;
    const auto previousTrackCount = state.trackCount;
    const bool recentClipsChanged = state.recentClips != recentClips;
    const bool timelineClipsChanged = state.timelineClips != timelineClips;

    state.isRecording = engine.isRecording();
    state.audioMonitoringEnabled = engine.isInputMonitoringEnabled();
    state.transportState = transportState;
    state.playhead = playhead;
    state.recentClips = std::move(recentClips);
    state.timelineClips = std::move(timelineClips);
    state.trackCount = juce::jmax(state.trackCount, maxTrackIndex + 1, 1);

    const bool controlsChanged = previousTransportState != state.transportState
        || previousRecordingState != state.isRecording
        || previousMonitoringState != state.audioMonitoringEnabled;
    const bool trackCountChanged = previousTrackCount != state.trackCount;

    gui->refreshExternalState(controlsChanged, trackCountChanged);

    if (!controlsChanged && !trackCountChanged && !recentClipsChanged && !timelineClipsChanged)
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
    const int nextTrackIndex = state.findNextEmptyTrackIndex();
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
    const auto sampleRate = engine.getTimelineSampleRate();
    engine.placeRecentAssetDirectToTimeline(assetId,
                                            trackIndex + 1,
                                            static_cast<juce::int64>(std::llround(startSeconds * sampleRate)));
    syncStateFromEngine();
}

void MainComponent::handleTimelineClipMoved(int placementId, int trackIndex, double startSeconds)
{
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
