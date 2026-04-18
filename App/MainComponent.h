#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

#include "guicomponent.h"
#include "../Audio_Engine/Root/AudioEngine.h"

class MainComponent : public juce::Component,
                      private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void resized() override;

private:
    void timerCallback() override;
    void syncStateFromEngine();
    void syncMixerStateToEngine();
    float resolveRoutedGain(const DAWState& dawState, int trackIndex, int depth) const;
    void placeRecordingOnArmedTrack();
    void handleRecordToggle();
    void handleMonitoringToggle();
    void handleImportAudio();
    void handleSaveProject();
    void handleOpenProject();
    void handleExportWav();
    void handlePlay();
    void handleStop();
    void handlePause();
    void handleRestart();
    void handleRecentClipDropped(int assetId, int trackIndex, double startSeconds);
    void handleAssetRenamed(int assetId, const juce::String& newName);
    void handlePatternEditRequested(int assetId);
    void syncTrackPatternsToAssets();
    void rescaleStandalonePatternAssetsForTempoChange(double oldTempoBpm, double newTempoBpm);
    double getSecondsPerBeat() const;
    std::vector<PatternNote> toPatternNotes(const TrackPatternState& pattern) const;
    std::vector<PianoRoll::Note> toPianoRollNotes(const SourceAsset& asset) const;
    double getPatternLengthSeconds(const TrackPatternState& pattern) const;
    void handleTimelineClipMoved(int placementId, int trackIndex, double startSeconds);
    void handleTimelineClipRemoved(int placementId);
    void handleSavePattern(int assetId, const std::vector<PianoRoll::Note>& notes);
    void handlePianoRollInstrumentChange(const juce::String& pluginName);
    int getNewestRecentAssetId() const;

    std::unique_ptr<GUIComponent> gui;
    std::unique_ptr<juce::TooltipWindow> tooltipWindow;
    AudioEngine engine;
    std::unique_ptr<juce::FileChooser> audioFileChooser;
    std::unique_ptr<juce::FileChooser> projectFileChooser;
    double lastSyncedTempoBpm = 120.0;
};
