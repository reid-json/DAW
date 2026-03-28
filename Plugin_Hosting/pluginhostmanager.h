#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <map>
#include <memory>

class PluginHostManager
{
public:
    PluginHostManager();
    ~PluginHostManager();

    juce::StringArray getAvailableTrackPlugins() const;
    juce::StringArray getAvailableMasterPlugins() const;

    bool loadTrackPlugin(const juce::String& pluginName, int trackIndex, int slotIndex);
    bool hasTrackPlugin(int trackIndex, int slotIndex) const;
    bool removeTrackPlugin(int trackIndex, int slotIndex);
    void removeTrackPluginSlot(int trackIndex, int slotIndex);
    void removeTrack(int trackIndex);
    bool setTrackPluginBypassed(int trackIndex, int slotIndex, bool shouldBeBypassed);
    bool showTrackPluginEditor(int trackIndex, int slotIndex);

    bool loadMasterPlugin(const juce::String& pluginName, int slotIndex);
    bool hasMasterPlugin(int slotIndex) const;
    bool removeMasterPlugin(int slotIndex);
    void removeMasterPluginSlot(int slotIndex);
    bool setMasterPluginBypassed(int slotIndex, bool shouldBeBypassed);
    bool showMasterPluginEditor(int slotIndex);

private:
    class PluginEditorWindow : public juce::DocumentWindow
    {
    public:
        PluginEditorWindow(const juce::String& windowTitle,
                           std::unique_ptr<juce::AudioProcessorEditor> editorIn);

        void closeButtonPressed() override;

    private:
        std::unique_ptr<juce::AudioProcessorEditor> ownedEditor;
    };

    struct SlotKey
    {
        int trackIndex = -1;
        int slotIndex = -1;

        auto tie() const noexcept { return std::tie(trackIndex, slotIndex); }
        bool operator<(const SlotKey& other) const noexcept { return tie() < other.tie(); }
    };

    struct HostedPlugin
    {
        juce::String pluginName;
        bool bypassed = false;
        std::unique_ptr<juce::AudioProcessor> processor;
        std::unique_ptr<PluginEditorWindow> editorWindow;
    };

    std::unique_ptr<juce::AudioProcessor> createProcessor(const juce::String& pluginName) const;
    HostedPlugin* getHostedTrackPlugin(int trackIndex, int slotIndex);
    const HostedPlugin* getHostedTrackPlugin(int trackIndex, int slotIndex) const;
    HostedPlugin* getHostedMasterPlugin(int slotIndex);
    const HostedPlugin* getHostedMasterPlugin(int slotIndex) const;

    std::map<SlotKey, HostedPlugin> hostedTrackPlugins;
    std::map<int, HostedPlugin> hostedMasterPlugins;
};
