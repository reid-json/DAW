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
    enum class ThemePreset
    {
        orange,
        blue,
        purple,
        green
    };

    explicit GUIComponent(juce::AudioDeviceManager& sharedDeviceManager);
    ~GUIComponent() override;

    void resized() override;
    void paintOverChildren(juce::Graphics& g) override;

    DAWState& getState() { return state; }
    const DAWState& getState() const { return state; }
    PluginHostManager& getPluginHostManager() { return pluginHostManager; }
    void syncFromEngine(bool shouldRefreshControls, bool shouldRebuildTrackList);
    void repaintDynamicViews();
    void openPianoRollForPattern(std::vector<PianoRoll::Note> notes, int assetId);

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
    std::function<void(int assetId)> onPatternEditRequested;
    std::function<void(int placementId, int trackIndex, double startSeconds)> onTimelineClipMoved;
    std::function<void(int placementId)> onTimelineClipDeleteRequested;
    std::function<void(int assetId, const std::vector<PianoRoll::Note>&)> onSavePatternRequested;
    std::function<void(const juce::String&)> onPianoRollInstrumentChangeRequested;

private:
    class HeaderButtonOverlay;
    jive::Interpreter viewInterpreter;
    jive::LookAndFeel lookAndFeel;
    std::unique_ptr<SharedPopupMenuLookAndFeel> fileMenuLookAndFeel;
    std::map<juce::String, std::unique_ptr<HeaderButtonOverlay>> headerButtonOverlays;
    std::unique_ptr<jive::GuiItem> root;
    juce::ValueTree uiTree;
    juce::var stylesheet;
    juce::var baseStylesheet;
    ThemeData themeData;
    ThemePreset currentTheme = ThemePreset::orange;
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
    int currentPianoRollAssetId = -1;

    juce::AudioDeviceManager& deviceManager;

    static jive::GuiItem* findGuiItemById (jive::GuiItem& node, const juce::Identifier& id);
    juce::Component* getCompById (const juce::String& id);
    juce::Button* getBtnById (const juce::String& id);

    void registerCustomComponentTypes();
    void layoutBody();
    void createHeaderButtonOverlays();
    void updateHeaderButtonOverlayBounds();
    void refreshHeaderButtonTooltips();
    void followTimelinePlayhead();
    void installCallbacks();
    void applyThemePreset (ThemePreset preset);
    void refreshFromState();
    void openSettingsWindow();
    juce::StringArray getAvailableTrackInputs() const;
    std::vector<PianoRoll::Note> getSelectedTrackPatternNotes() const;
    int getSelectedTrackPatternAssetId() const;
    void ensurePianoRollWindow();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GUIComponent)
};
