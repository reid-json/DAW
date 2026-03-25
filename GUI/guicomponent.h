#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <jive_layouts/jive_layouts.h>

#include "dawstate.h"
#include "timelinecomponent.h"
#include "arrangementcomponent.h"
#include "recentclipscomponent.h"
#include "pianoRoll.h"
#include "settingswindow.h"

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
    std::function<void()> onImportAudioRequested;
    std::function<void()> onPlayRequested;
    std::function<void()> onStopRequested;
    std::function<void()> onPauseRequested;
    std::function<void()> onRestartRequested;
    std::function<void(int assetId, int trackIndex, double startSeconds)> onRecentClipDropped;
    std::function<void(int placementId, int trackIndex, double startSeconds)> onTimelineClipMoved;
    std::function<void(int placementId)> onTimelineClipDeleteRequested;

private:
    jive::Interpreter viewInterpreter;
    std::unique_ptr<jive::GuiItem> root;
    juce::ValueTree uiTree;
    juce::var stylesheet;

    DAWState state;
    TimelineComponent* timelineComponent = nullptr;
    ArrangementComponent* arrangementComponent = nullptr;
    RecentClipsComponent* recentClipsComponent = nullptr;
    std::unique_ptr<PianoRollWindow> pianoRollWindow;
    std::unique_ptr<SettingsWindow> settingsWindow;

    juce::AudioDeviceManager& deviceManager;

    bool draggingSidebar = false;
    int dragStartScreenX = 0;
    int sidebarStartWidth = 240;

    class SidebarResizerListener : public juce::MouseListener
    {
    public:
        explicit SidebarResizerListener (GUIComponent& ownerIn) : owner (ownerIn) {}

        void mouseDown (const juce::MouseEvent& e) override;
        void mouseDrag (const juce::MouseEvent& e) override;
        void mouseUp   (const juce::MouseEvent& e) override;

    private:
        GUIComponent& owner;
    };

    std::unique_ptr<SidebarResizerListener> resizerListener;

    static jive::GuiItem* findGuiItemById (jive::GuiItem& node, const juce::Identifier& id);
    static int getIntProperty (const juce::ValueTree& v, const juce::Identifier& key, int fallback);

    void registerCustomComponentTypes();
    void setSidebarWidth (int newWidth);
    void installCallbacks();
    void bindDynamicTrackButtons();
    void refreshFromState();
    void rebuildTrackList();
    void openSettingsWindow();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GUIComponent)
};
