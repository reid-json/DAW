#pragma once

#include <map>

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <jive_layouts/jive_layouts.h>

#include "dawstate.h"
#include "timelinecomponent.h"
#include "arrangementcomponent.h"
#include "tracklistcomponent.h"
#include "recentclipscomponent.h"
#include "tempocontrolcomponent.h"
#include "theme.h"
#include "pianoRoll.h"
#include "settingswindow.h"
#include "../Plugin_Hosting/pluginhostmanager.h"

class GUIComponent : public juce::Component,
                     public juce::DragAndDropContainer
{
public:
    explicit GUIComponent(juce::AudioDeviceManager& sharedDeviceManager);
    ~GUIComponent() override;

    void resized() override;
    void paintOverChildren(juce::Graphics& g) override;

    DAWState& getState() noexcept                     { return state; }
    const DAWState& getState() const noexcept         { return state; }
    PluginHostManager& getPluginHostManager() noexcept { return pluginHostManager; }
    void refreshExternalState(bool shouldRefreshControls, bool shouldRebuildTrackList);
    void repaintDynamicViews();

    std::function<void()> onRecordToggleRequested;
    std::function<void()> onMonitoringToggleRequested;
    std::function<void()> onImportAudioRequested;
    std::function<void()> onSaveProjectRequested;
    std::function<void()> onOpenProjectRequested;
    std::function<void()> onExportWavRequested;
    std::function<void()> onPlayRequested;
    std::function<void()> onStopRequested;
    std::function<void()> onPauseRequested;
    std::function<void()> onRestartRequested;
    std::function<void(int assetId, int trackIndex, double startSeconds)> onRecentClipDropped;
    std::function<void(int assetId, const juce::String& newName)> onAssetRenameRequested;
    std::function<void(int placementId, int trackIndex, double startSeconds)> onTimelineClipMoved;
    std::function<void(int placementId)> onTimelineClipDeleteRequested;
    std::function<void(const std::vector<PianoRoll::Note>&)> onSavePatternRequested;
    std::function<void(const juce::String&)> onPianoRollInstrumentChangeRequested;

private:
    class HeaderButtonOverlay;

    jive::Interpreter viewInterpreter;
    jive::LookAndFeel lookAndFeel;
    std::map<juce::String, std::unique_ptr<HeaderButtonOverlay>> headerButtonOverlays;
    std::unique_ptr<jive::GuiItem> root;
    juce::ValueTree uiTree;
    juce::var stylesheet;
    ThemeData themeData;
    std::map<juce::String, juce::var> spriteAssets;
    DAWState state;
    PluginHostManager pluginHostManager;
    TimelineComponent* timelineComponent = nullptr;
    ArrangementComponent* arrangementComponent = nullptr;
    TrackListComponent* trackListComponent = nullptr;
    RecentClipsComponent* recentClipsComponent = nullptr;
    TempoControlComponent* tempoControlComponent = nullptr;
    std::unique_ptr<PianoRollWindow> pianoRollWindow;
    std::unique_ptr<SettingsWindow> settingsWindow;

    juce::AudioDeviceManager& deviceManager;

    static jive::GuiItem* findGuiItemById (jive::GuiItem& node, const juce::Identifier& id);

    void registerCustomComponentTypes();
    void applyManualBodyLayout();
    void createHeaderButtonOverlays();
    void updateHeaderButtonOverlayBounds();
    void followTimelinePlayhead();
    void installCallbacks();
    void refreshFromState();
    void openSettingsWindow();
    juce::StringArray getAvailableTrackInputs() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GUIComponent)
};
