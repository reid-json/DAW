#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "dawstate.h"

class MixerMasterComponent : public juce::Component
{
public:
    explicit MixerMasterComponent(DAWState& stateIn);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    std::function<juce::StringArray()> getAvailableMasterPlugins;
    std::function<juce::StringArray()> getAvailableMasterOutputs;
    std::function<juce::String()> getMasterOutputDeviceName;
    std::function<juce::StringArray()> getAvailableTrackPlugins;
    std::function<juce::StringArray()> getAvailableTrackInputs;
    std::function<juce::StringArray()> getAvailableTrackOutputs;
    std::function<juce::String()> getTrackInputDeviceName;
    std::function<juce::String()> getTrackOutputDeviceName;
    std::function<bool(int slotIndex, const juce::String& pluginName)> onMasterPluginLoadRequested;
    std::function<void(int slotIndex, bool shouldBeBypassed)> onMasterPluginBypassRequested;
    std::function<void(int slotIndex)> onMasterPluginRemoveRequested;
    std::function<void(int slotIndex)> onMasterPluginEditorRequested;
    std::function<void(int slotIndex)> onMasterPluginSlotRemoveRequested;
    std::function<bool(int trackIndex, int slotIndex, const juce::String& pluginName)> onTrackPluginLoadRequested;
    std::function<void(int trackIndex, int slotIndex, bool shouldBeBypassed)> onTrackPluginBypassRequested;
    std::function<void(int trackIndex, int slotIndex)> onTrackPluginRemoveRequested;
    std::function<void(int trackIndex, int slotIndex)> onTrackPluginEditorRequested;
    std::function<void(int trackIndex, int slotIndex)> onTrackPluginSlotRemoveRequested;

private:
    bool isShowingMaster() const;
    int getFocusedTrackIndex() const;
    juce::String getFocusedTitle() const;
    juce::Rectangle<float> getInnerBounds() const;
    juce::Rectangle<float> getHeaderActionBounds() const;
    juce::Rectangle<float> getControlButtonsBounds() const;
    juce::Rectangle<float> getButtonBounds(int buttonIndex) const;
    juce::Rectangle<float> getIoButtonBounds() const;
    juce::Rectangle<float> getPanKnobBounds() const;
    juce::Rectangle<float> getFaderBounds() const;
    juce::Rectangle<float> getPluginLaneBounds() const;
    juce::Rectangle<float> getPluginSlotBounds(int slotIndex) const;
    juce::Rectangle<float> getAddFxSlotBounds() const;
    void drawTransportButtons(juce::Graphics& g, juce::Rectangle<float> bounds) const;
    void drawPanKnob(juce::Graphics& g, juce::Rectangle<float> bounds) const;
    void drawMeter(juce::Graphics& g, juce::Rectangle<float> bounds) const;
    void drawFader(juce::Graphics& g, juce::Rectangle<float> bounds) const;
    void drawPluginLane(juce::Graphics& g, juce::Rectangle<float> bounds) const;
    void showIoMenu();
    void showFxSlotMenu(int slotIndex);
    void paintMasterView(juce::Graphics& g, juce::Rectangle<float> innerBounds);
    void paintTrackView(juce::Graphics& g, juce::Rectangle<float> innerBounds, int trackIndex);
    void updatePanFromPosition(juce::Point<float> position);
    void updateLevelFromPosition(juce::Point<float> position);

    enum class DragTarget
    {
        none,
        pan,
        level
    };

    DAWState& state;
    DragTarget activeDragTarget = DragTarget::none;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerMasterComponent)
};
