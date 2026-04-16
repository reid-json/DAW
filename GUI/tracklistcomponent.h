#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "dawstate.h"
#include "theme.h"

class TrackListComponent : public juce::Component
{
public:
    TrackListComponent(DAWState& stateIn, ThemeData& themeIn);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    std::function<void()> onAddTrackRequested;
    std::function<void(int trackIndex)> onRemoveTrackRequested;
    std::function<void(int trackIndex)> onTrackSelected;
    std::function<void()> onMixerFocusChanged;

    // Plugin management
    std::function<juce::StringArray(bool isMaster)> onGetAvailablePlugins;
    std::function<juce::String(bool isMaster, int trackIndex, int slotIndex)> onGetPluginName;
    std::function<void(bool isMaster, int trackIndex, int slotIndex, const juce::String&)> onLoadPlugin;
    std::function<void(bool isMaster, int trackIndex, int slotIndex)> onRemovePlugin;
    std::function<void(bool isMaster, int trackIndex, int slotIndex)> onShowPluginEditor;

    // Input device selection
    std::function<juce::StringArray()> onGetAvailableInputs;

private:
    static constexpr int rowHeight = 92;
    static constexpr int rowGap = 8;

    int getVisualRowCount() const;
    int getVisualRowAt(juce::Point<float> point) const;
    bool isMasterRow(int rowIndex) const;
    int getTrackIndexForRow(int rowIndex) const;

    juce::Rectangle<float> getRowBounds(int rowIndex) const;
    juce::Rectangle<float> getAddTrackButtonBounds() const;

    // Hit-test regions within a row
    juce::Rectangle<float> getIndicatorBounds(int rowIndex, int indicatorIndex) const;
    juce::Rectangle<float> getRemoveButtonBounds(int trackIndex) const;
    juce::Rectangle<float> getFaderBounds(int rowIndex) const;
    juce::Rectangle<float> getPanKnobBounds(int rowIndex) const;
    juce::Rectangle<float> getRoutingButtonBounds(int rowIndex) const;

    // Drawing
    void drawStrip(juce::Graphics& g, int rowIndex);
    void drawIndicatorPill(juce::Graphics& g, juce::Rectangle<float> bounds,
                           const char* label, bool active, juce::Colour activeColour,
                           bool roundLeft, bool roundRight,
                           const juce::Image* sprite = nullptr) const;
    void drawFader(juce::Graphics& g, juce::Rectangle<float> bounds, float level, bool active, int rowIndex) const;
    void drawPanKnob(juce::Graphics& g, juce::Rectangle<float> bounds, float pan, bool active, int rowIndex) const;

    void promptRenameTrack(int trackIndex);
    void showTrackContextMenu(int trackIndex);
    void showRoutingMenu(int rowIndex);
    void showFxMenu(int rowIndex);
    void showInputMenu(int rowIndex);

    int getContentHeight() const;
    int getMaxScroll() const;

    void startInlineRename (int trackIndex);
    void commitInlineRename();
    void cancelInlineRename();

    DAWState& state;
    ThemeData& theme;
    int draggingFaderRow = -1;
    int draggingPanRow = -1;
    std::unique_ptr<juce::TextEditor> inlineEditor;
    int editingTrackIndex = -1;
    juce::Image mutedSprite;
    juce::Image unmutedSprite;
    juce::Image armSprite;
    juce::Image bodySpiceImage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackListComponent)
};
