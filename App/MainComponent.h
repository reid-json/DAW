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
    void syncTrackPatternsToAssets();
    static double getPatternLengthSeconds(const TrackPatternState& pattern);
    void handleTimelineClipMoved(int placementId, int trackIndex, double startSeconds);
    void handleTimelineClipRemoved(int placementId);
    void handleSavePattern(const std::vector<PianoRoll::Note>& notes);
    void handlePianoRollInstrumentChange(const juce::String& pluginName);
    int getNewestRecentAssetId() const;

    std::unique_ptr<GUIComponent> gui;
    AudioEngine engine;
    std::unique_ptr<juce::FileChooser> audioFileChooser;
    std::unique_ptr<juce::FileChooser> projectFileChooser;
};
