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
    void placeNewestRecentAssetOnNextEmptyTrack();
    void handleRecordToggle();
    void handleImportAudio();
    void handlePlay();
    void handleStop();
    void handlePause();
    void handleRestart();
    void handleRecentClipDropped(int assetId, int trackIndex, double startSeconds);
    void handleTimelineClipMoved(int placementId, int trackIndex, double startSeconds);
    void handleTimelineClipRemoved(int placementId);
    int getNewestRecentAssetId() const;

    std::unique_ptr<GUIComponent> gui;
    AudioEngine engine;
    std::unique_ptr<juce::FileChooser> audioFileChooser;
};
