#include "MainComponent.h"
#include "ProjectFile.h"
#include <set>
#include <cmath>

static TimelineClipItem::ContentKind toTimelineContentKind(AssetKind kind)
{
    return kind == AssetKind::pianoRollPattern
        ? TimelineClipItem::ContentKind::pattern
        : TimelineClipItem::ContentKind::recording;
}

MainComponent::MainComponent()
{
    engine.initialise(2, 2);

    tooltipWindow = std::make_unique<juce::TooltipWindow> (this, 500);
    gui = std::make_unique<GUIComponent>(engine.getDeviceManager());
    engine.setPatternTrackRenderer([this] (int /*trackIndex*/,
                                           juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer& midi,
                                           double sampleRate)
    {
        return gui != nullptr
            && gui->getPluginHostManager().renderPianoRollInstrument(buffer, midi, sampleRate);
    });
    engine.getIODeviceManager().setFxProcessCallback([this] (juce::AudioBuffer<float>& buffer)
    {
        if (gui == nullptr) return;
        auto& phm = gui->getPluginHostManager();
        int trackCount = gui->getState().trackCount;

        for (int i = 0; i < trackCount; ++i)
            phm.processTrackEffects(i, buffer);

        phm.processMasterEffects(buffer);
    });
    gui->onRecordToggleRequested = [this] { handleRecordToggle(); };
    gui->onMonitoringToggleRequested = [this] { handleMonitoringToggle(); };
    gui->onImportAudioRequested = [this] { handleImportAudio(); };
    gui->onSaveProjectRequested = [this] { handleSaveProject(); };
    gui->onOpenProjectRequested = [this] { handleOpenProject(); };
    gui->onExportWavRequested = [this] { handleExportWav(); };
    gui->onPlayRequested = [this] { handlePlay(); };
    gui->onStopRequested = [this] { handleStop(); };
    gui->onPauseRequested = [this] { handlePause(); };
    gui->onRestartRequested = [this] { handleRestart(); };
    gui->onRecentClipDropped = [this] (int a, int t, double s) { handleRecentClipDropped(a, t, s); };
    gui->onAssetRenameRequested = [this] (int a, const juce::String& n) { handleAssetRenamed(a, n); };
    gui->onPatternEditRequested = [this] (int a) { handlePatternEditRequested(a); };
    gui->onTimelineClipMoved = [this] (int p, int t, double s) { handleTimelineClipMoved(p, t, s); };
    gui->onTimelineClipDeleteRequested = [this] (int p) { handleTimelineClipRemoved(p); };
    gui->onSavePatternRequested = [this] (int a, const std::vector<PianoRoll::Note>& n) { handleSavePattern(a, n); };
    gui->onPianoRollInstrumentChangeRequested = [this] (const juce::String& n) { handlePianoRollInstrumentChange(n); };

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
    syncMixerStateToEngine();
    syncStateFromEngine();
}

void MainComponent::syncMixerStateToEngine()
{
    if (gui == nullptr) return;

    const auto& guiState = gui->getState();
    juce::ScopedLock lock(engine.getDeviceManager().getAudioCallbackLock());
    auto& arrangement = engine.getArrangementState();

    float masterGain = guiState.masterMixerState.muted ? 0.0f : guiState.masterMixerState.level;
    arrangement.setMasterGain(masterGain);

    for (int i = 0; i < guiState.trackCount; ++i)
    {
        float gain = resolveRoutedGain(guiState, i, 0);
        arrangement.setMixerTrackGain(i, gain);
        arrangement.setMixerTrackPan(i, guiState.getTrackMixerState(i).pan);
    }
}

float MainComponent::resolveRoutedGain(const DAWState& dawState, int trackIndex, int depth) const
{
    if (depth > 8 || trackIndex < 0 || trackIndex >= dawState.trackCount)
        return 0.0f;

    const auto& ts = dawState.getTrackMixerState(trackIndex);
    float gain = ts.muted ? 0.0f : ts.level;

    const auto& output = dawState.getTrackOutputAssignment(trackIndex);
    if (!output.containsIgnoreCase("Master"))
    {
        for (int dest = 0; dest < dawState.trackCount; ++dest)
        {
            if (dest == trackIndex) continue;
            if (dawState.getTrackName(dest) == output)
            {
                gain *= resolveRoutedGain(dawState, dest, depth + 1);
                break;
            }
        }
    }

    return gain;
}

void MainComponent::syncStateFromEngine()
{
    if (gui == nullptr)
        return;

    const auto currentTempoBpm = juce::jlimit(40.0, 240.0, gui->getState().tempoBpm);
    if (std::abs(currentTempoBpm - lastSyncedTempoBpm) > 0.001)
    {
        rescalePatternsForTempo(lastSyncedTempoBpm, currentTempoBpm);
        lastSyncedTempoBpm = currentTempoBpm;
    }

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
                {
                    savedPatterns.push_back({ asset->assetId, -1, asset->name, asset->clip.getLengthSeconds() });
                    continue;
                }

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

    gui->syncFromEngine(controlsChanged, trackCountChanged);

    if (!controlsChanged && !trackCountChanged && !recentClipsChanged && !savedPatternsChanged && !timelineClipsChanged)
        gui->repaintDynamicViews();
}

void MainComponent::placeRecordingOnArmedTrack()
{
    if (gui == nullptr)
        return;

    const int newestAssetId = getNewestRecentAssetId();
    if (newestAssetId <= 0)
        return;

    const auto& guiState = gui->getState();
    const int armedTrack = guiState.getFirstArmedTrackIndex();
    if (armedTrack < 0)
        return;

    engine.placeRecentAssetDirectToTimeline(newestAssetId, armedTrack + 1, 0);
}

void MainComponent::handleRecordToggle()
{
    if (!engine.isRecording())
    {
        const auto& guiState = gui->getState();
        if (guiState.getFirstArmedTrackIndex() < 0)
            return;

        engine.startRecording();
    }

    syncStateFromEngine();
}

void MainComponent::handleMonitoringToggle()
{
    engine.setInputMonitoringEnabled(! engine.isInputMonitoringEnabled());
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

void MainComponent::handleSaveProject()
{
    projectFileChooser = std::make_unique<juce::FileChooser>(
        "Save Project", juce::File(), "*.dawproj");

    projectFileChooser->launchAsync(
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser)
        {
            auto file = chooser.getResult();
            if (file != juce::File())
            {
                if (!file.hasFileExtension(".dawproj"))
                    file = file.withFileExtension("dawproj");

                syncTrackPatternsToAssets();
                juce::ScopedLock lock(engine.getDeviceManager().getAudioCallbackLock());
                saveProject(file, gui->getState(), engine.getArrangementState());
            }
            projectFileChooser.reset();
        });
}

void MainComponent::handleOpenProject()
{
    projectFileChooser = std::make_unique<juce::FileChooser>(
        "Open Project", juce::File(), "*.dawproj");

    projectFileChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser)
        {
            auto file = chooser.getResult();
            if (file.existsAsFile())
            {
                engine.stopPlayback();
                juce::ScopedLock lock(engine.getDeviceManager().getAudioCallbackLock());
                loadProject(file, gui->getState(), engine.getArrangementState());
            }
            projectFileChooser.reset();
            lastSyncedTempoBpm = gui->getState().tempoBpm;
            syncStateFromEngine();
        });
}

void MainComponent::handleExportWav()
{
    projectFileChooser = std::make_unique<juce::FileChooser>(
        "Export as WAV", juce::File(), "*.wav");

    projectFileChooser->launchAsync(
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser)
        {
            auto file = chooser.getResult();
            if (file == juce::File()) { projectFileChooser.reset(); return; }

            if (!file.hasFileExtension(".wav"))
                file = file.withFileExtension("wav");

            // Find total length of the arrangement
            juce::int64 totalSamples = 0;
            {
                juce::ScopedLock lock(engine.getDeviceManager().getAudioCallbackLock());
                const auto& arr = engine.getArrangementState();
                for (const auto& placement : arr.getTimelinePlacements())
                {
                    const auto* asset = arr.findAsset(placement.assetId);
                    if (asset != nullptr)
                    {
                        auto end = placement.startSample + asset->clip.getNumSamples();
                        if (end > totalSamples) totalSamples = end;
                    }
                }
            }

            if (totalSamples <= 0) { projectFileChooser.reset(); return; }

            double sampleRate = engine.getTimelineSampleRate();
            juce::AudioBuffer<float> fullBuffer(2, static_cast<int>(totalSamples));
            fullBuffer.clear();

            constexpr int blockSize = 4096;
            {
                juce::ScopedLock lock(engine.getDeviceManager().getAudioCallbackLock());
                const auto& arr = engine.getArrangementState();
                for (juce::int64 pos = 0; pos < totalSamples; pos += blockSize)
                {
                    int thisBlock = static_cast<int>(juce::jmin(static_cast<juce::int64>(blockSize),
                                                                totalSamples - pos));
                    juce::AudioBuffer<float> block(2, thisBlock);
                    block.clear();
                    arr.render(block, pos);

                    for (int ch = 0; ch < 2; ++ch)
                        fullBuffer.copyFrom(ch, static_cast<int>(pos), block, ch, 0, thisBlock);
                }
            }

            file.deleteFile();
            auto stream = std::unique_ptr<juce::FileOutputStream>(file.createOutputStream());
            if (stream != nullptr)
            {
                juce::WavAudioFormat wav;
                auto* writer = wav.createWriterFor(stream.get(), sampleRate, 2, 16, {}, 0);
                if (writer != nullptr)
                {
                    stream.release();
                    writer->writeFromAudioSampleBuffer(fullBuffer, 0, fullBuffer.getNumSamples());
                    delete writer;
                }
            }

            projectFileChooser.reset();
        });
}

void MainComponent::handlePlay()    { engine.startPlayback();        syncStateFromEngine(); }
void MainComponent::handlePause()   { engine.togglePlaybackPause();  syncStateFromEngine(); }
void MainComponent::handleRestart() { engine.stopPlayback();         syncStateFromEngine(); }

void MainComponent::handleStop()
{
    if (engine.isRecording())
    {
        engine.stopRecording();
        placeRecordingOnArmedTrack();
    }
    engine.stopPlayback();
    syncStateFromEngine();
}

void MainComponent::handleRecentClipDropped(int assetId, int trackIndex, double startSeconds)
{
    const auto& arrangement = engine.getArrangementState();
    const auto* asset = arrangement.findAsset(assetId);
    if (asset == nullptr)
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
            engine.updatePatternAsset(assetId, trimmedName, getPatternLengthSeconds(pattern), toPatternNotes(pattern));
            syncStateFromEngine();
            return;
        }
    }

    if (engine.renameAsset(assetId, trimmedName))
        syncStateFromEngine();
}

void MainComponent::handlePatternEditRequested(int assetId)
{
    if (gui == nullptr || assetId <= 0)
        return;

    const auto* asset = engine.getArrangementState().findAsset(assetId);
    if (asset == nullptr || asset->kind != AssetKind::pianoRollPattern)
        return;

    gui->openPianoRollForPattern(toPianoRollNotes(*asset), assetId);
}

void MainComponent::syncTrackPatternsToAssets()
{
    if (gui == nullptr)
        return;

    auto& state = gui->getState();

    for (int trackIndex = 0; trackIndex < state.trackCount; ++trackIndex)
    {
        auto& pattern = state.getTrackPatternState(trackIndex);

        if (pattern.notes.empty())
            continue;

        const auto patternName = state.getTrackName(trackIndex);
        state.setTrackPatternName(trackIndex, patternName);
        const auto lengthSeconds = getPatternLengthSeconds(pattern);
        auto patternNotes = toPatternNotes(pattern);

        if (pattern.assetId <= 0)
            pattern.assetId = engine.createPatternAsset(patternName, lengthSeconds, std::move(patternNotes));
        else
            engine.updatePatternAsset(pattern.assetId, patternName, lengthSeconds, std::move(patternNotes));
    }
}

void MainComponent::rescalePatternsForTempo(double oldTempoBpm, double newTempoBpm)
{
    if (gui == nullptr) return;

    oldTempoBpm = juce::jlimit(40.0, 240.0, oldTempoBpm);
    newTempoBpm = juce::jlimit(40.0, 240.0, newTempoBpm);
    if (std::abs(oldTempoBpm - newTempoBpm) <= 0.001) return;

    const double scale = oldTempoBpm / newTempoBpm;

    std::set<int> boundAssetIds;
    const auto& state = gui->getState();
    for (int i = 0; i < state.trackCount; ++i)
    {
        const int id = state.getTrackPatternState(i).assetId;
        if (id > 0)
            boundAssetIds.insert(id);
    }

    struct Update { int assetId; juce::String name; double lengthSeconds; std::vector<PatternNote> notes; };
    std::vector<Update> updates;

    {
        juce::ScopedLock lock(engine.getDeviceManager().getAudioCallbackLock());
        for (const auto& asset : engine.getArrangementState().getAssets())
        {
            if (asset.kind != AssetKind::pianoRollPattern || boundAssetIds.count(asset.assetId))
                continue;

            std::vector<PatternNote> notes;
            notes.reserve(asset.patternNotes.size());
            for (const auto& n : asset.patternNotes)
                notes.push_back({ juce::jmax(0.0, n.startSeconds * scale),
                                  juce::jmax(0.01, n.lengthSeconds * scale),
                                  n.midiNote, n.velocity });

            updates.push_back({ asset.assetId, asset.name,
                                juce::jmax(0.25, asset.clip.getLengthSeconds() * scale),
                                std::move(notes) });
        }
    }

    for (auto& u : updates)
        engine.updatePatternAsset(u.assetId, u.name, u.lengthSeconds, std::move(u.notes));
}

double MainComponent::getPatternLengthSeconds(const TrackPatternState& pattern) const
{
    double endBeat = 1.0;
    for (auto& n : pattern.notes)
        endBeat = juce::jmax(endBeat, n.startBeat + n.lengthBeats);
    return juce::jmax(0.25, endBeat * getSecondsPerBeat());
}

void MainComponent::handleTimelineClipMoved(int placementId, int trackIndex, double startSeconds)
{
    const auto& arrangement = engine.getArrangementState();
    const auto* placement = arrangement.findTimelinePlacement(placementId);
    const auto* asset = placement != nullptr ? arrangement.findAsset(placement->assetId) : nullptr;
    if (asset == nullptr)
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

void MainComponent::handleSavePattern(int assetId, const std::vector<PianoRoll::Note>& pianoNotes)
{
    if (pianoNotes.empty()) return;

    const auto secondsPerBeat = getSecondsPerBeat();
    std::vector<PatternNote> patternNotes;
    patternNotes.reserve(pianoNotes.size());
    double endBeat = 1.0;

    for (const auto& n : pianoNotes)
    {
        patternNotes.push_back({ juce::jmax(0.0, n.startBeat * secondsPerBeat),
                                 juce::jmax(0.01, n.lengthBeats * secondsPerBeat),
                                 n.midiNote, 0.85f });
        endBeat = juce::jmax(endBeat, n.startBeat + n.lengthBeats);
    }

    const auto lengthSeconds = juce::jmax(0.25, endBeat * secondsPerBeat);
    juce::ScopedLock lock(engine.getDeviceManager().getAudioCallbackLock());

    if (assetId > 0)
    {
        const auto* asset = engine.getArrangementState().findAsset(assetId);
        if (asset == nullptr || asset->kind != AssetKind::pianoRollPattern) return;
        engine.updatePatternAsset(assetId, asset->name, lengthSeconds, std::move(patternNotes));
    }
    else
    {
        const auto name = "Pattern " + juce::String((int) gui->getState().savedPatterns.size() + 1);
        engine.createPatternAsset(name, lengthSeconds, std::move(patternNotes));
    }
    syncStateFromEngine();
}

void MainComponent::handlePianoRollInstrumentChange(const juce::String& pluginName)
{
    juce::ScopedLock lock(engine.getDeviceManager().getAudioCallbackLock());
    gui->getPluginHostManager().loadPianoRollInstrument(pluginName);
}

int MainComponent::getNewestRecentAssetId() const {
    auto& ids = engine.getArrangementState().getRecentAssetIds();
    return ids.empty() ? -1 : ids.back();
}

double MainComponent::getSecondsPerBeat() const
{
    if (gui == nullptr)
        return 0.5;

    const auto bpm = juce::jlimit(40.0, 240.0, gui->getState().tempoBpm);
    return 60.0 / bpm;
}

std::vector<PatternNote> MainComponent::toPatternNotes(const TrackPatternState& pattern) const
{
    const auto secondsPerBeat = getSecondsPerBeat();
    std::vector<PatternNote> out;
    out.reserve(pattern.notes.size());

    for (auto& n : pattern.notes)
        out.push_back({ juce::jmax(0.0, n.startBeat * secondsPerBeat),
                        juce::jmax(0.01, n.lengthBeats * secondsPerBeat),
                        n.midiNote, 0.85f });

    return out;
}

std::vector<PianoRoll::Note> MainComponent::toPianoRollNotes(const SourceAsset& asset) const
{
    std::vector<PianoRoll::Note> out;
    out.reserve(asset.patternNotes.size());

    const auto secondsPerBeat = getSecondsPerBeat();
    int nextId = 1;
    for (const auto& note : asset.patternNotes)
    {
        const auto startBeat = juce::jmax(0.0, note.startSeconds / secondsPerBeat);
        const auto lengthBeats = juce::jmax(0.25, note.lengthSeconds / secondsPerBeat);
        out.push_back({ nextId, startBeat, lengthBeats, note.midiNote, PianoRoll::getNoteName(note.midiNote) });
        ++nextId;
    }

    return out;
}
