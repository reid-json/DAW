#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <jive_layouts/jive_layouts.h>

#include "dawstate.h"
#include "timelinecomponent.h"
#include "arrangementcomponent.h"
#include "mixermastercomponent.h"
#include "tracklistcomponent.h"
#include "recentclipscomponent.h"
#include "tempocontrolcomponent.h"
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

    DAWState& getState() noexcept                     { return state; }
    const DAWState& getState() const noexcept         { return state; }
    void refreshExternalState(bool shouldRefreshControls, bool shouldRebuildTrackList);
    void repaintDynamicViews();
    juce::ValueTree getUITree() const                 { return uiTree; }
    juce::Component* getRenderedRootComponent() const noexcept
    {
        return root != nullptr ? root->getComponent().get() : nullptr;
    }

    std::function<void()> onRecordToggleRequested;
    std::function<void()> onMonitoringToggleRequested;
    std::function<void()> onImportAudioRequested;
    std::function<void()> onPlayRequested;
    std::function<void()> onStopRequested;
    std::function<void()> onPauseRequested;
    std::function<void()> onRestartRequested;
    std::function<void(int assetId, int trackIndex, double startSeconds)> onRecentClipDropped;
    std::function<void(int placementId, int trackIndex, double startSeconds)> onTimelineClipMoved;
    std::function<void(int placementId)> onTimelineClipDeleteRequested;

private:
    static constexpr int sidebarEdgeGripWidth = 12;

    jive::Interpreter viewInterpreter;
    jive::LookAndFeel lookAndFeel;
    std::unique_ptr<jive::GuiItem> root;
    juce::ValueTree uiTree;
    juce::var stylesheet;

    DAWState state;
    PluginHostManager pluginHostManager;
    TimelineComponent* timelineComponent = nullptr;
    ArrangementComponent* arrangementComponent = nullptr;
    MixerMasterComponent* mixerMasterComponent = nullptr;
    TrackListComponent* trackListComponent = nullptr;
    RecentClipsComponent* recentClipsComponent = nullptr;
    TempoControlComponent* tempoControlComponent = nullptr;
    std::unique_ptr<PianoRollWindow> pianoRollWindow;
    std::unique_ptr<SettingsWindow> settingsWindow;

    juce::AudioDeviceManager& deviceManager;

    enum class WorkspaceResizeEdge
    {
        none,
        left,
        right
    };

    bool draggingSidebar = false;
    bool draggingClipSidebar = false;
    int dragStartScreenX = 0;
    int sidebarStartWidth = 340;
    int clipSidebarStartWidth = 220;
    WorkspaceResizeEdge activeWorkspaceResizeEdge = WorkspaceResizeEdge::none;

    class SidebarResizerListener : public juce::MouseListener
    {
    public:
        explicit SidebarResizerListener (GUIComponent& ownerIn) : owner (ownerIn) {}

        void mouseMove (const juce::MouseEvent& e) override;
        void mouseDown (const juce::MouseEvent& e) override;
        void mouseDrag (const juce::MouseEvent& e) override;
        void mouseUp   (const juce::MouseEvent& e) override;
        void mouseExit (const juce::MouseEvent& e) override;

    private:
        GUIComponent& owner;
    };

    class WorkspaceResizeListener : public juce::MouseListener
    {
    public:
        explicit WorkspaceResizeListener (GUIComponent& ownerIn) : owner (ownerIn) {}

        void mouseMove (const juce::MouseEvent& e) override;
        void mouseDown (const juce::MouseEvent& e) override;
        void mouseDrag (const juce::MouseEvent& e) override;
        void mouseUp   (const juce::MouseEvent& e) override;
        void mouseExit (const juce::MouseEvent& e) override;

    private:
        GUIComponent& owner;
    };

    std::unique_ptr<WorkspaceResizeListener> workspaceResizeListener;

    static jive::GuiItem* findGuiItemById (jive::GuiItem& node, const juce::Identifier& id);
    static int getIntProperty (const juce::ValueTree& v, const juce::Identifier& key, int fallback);
    static bool isNearRightEdge (const juce::MouseEvent& e, juce::Component& target, int gripWidth = 8);
    static bool isNearLeftEdge (const juce::MouseEvent& e, juce::Component& target, int gripWidth = 8);

    void registerCustomComponentTypes();
    void applyManualBodyLayout();
    void setSidebarWidth (int newWidth);
    void setClipSidebarWidth (int newWidth);
    void followTimelinePlayhead();
    void installCallbacks();
    void bindDynamicTrackButtons();
    void refreshFromState();
    void rebuildTrackList();
    void openSettingsWindow();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GUIComponent)
};
