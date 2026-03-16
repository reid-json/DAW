#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <jive_layouts/jive_layouts.h>

#include "dawstate.h"
#include "timelinecomponent.h"
#include "arrangementcomponent.h"
#include "pianoRoll.h"
#include "settingswindow.h"

class GUIComponent : public juce::Component,
                     private juce::Timer
{
public:
    GUIComponent();
    ~GUIComponent() override;

    void resized() override;

    DAWState& getState() noexcept                     { return state; }
    const DAWState& getState() const noexcept         { return state; }
    juce::ValueTree getUITree() const                 { return uiTree; }
    juce::Component* getRenderedRootComponent() const noexcept
    {
        return root != nullptr ? root->getComponent().get() : nullptr;
    }

private:
    jive::Interpreter viewInterpreter;
    std::unique_ptr<jive::GuiItem> root;
    juce::ValueTree uiTree;
    juce::var stylesheet;

    DAWState state;
    std::unique_ptr<TimelineComponent> timelineComponent;
    std::unique_ptr<ArrangementComponent> arrangementComponent;
    std::unique_ptr<PianoRollWindow> pianoRollWindow;
    std::unique_ptr<SettingsWindow> settingsWindow;

    juce::AudioDeviceManager deviceManager;

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

    void setSidebarWidth (int newWidth);
    void installCallbacks();
    void bindDynamicTrackButtons();
    void refreshFromState();
    void rebuildTrackList();
    void rebuildClipList();
    void openSettingsWindow();
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GUIComponent)
};