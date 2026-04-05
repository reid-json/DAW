#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <map>
#include <memory>

class PluginHostManager
{
public:
    enum class PluginRole
    {
        effect,
        instrument
    };

    PluginHostManager();
    ~PluginHostManager();

    juce::StringArray getAvailableTrackPlugins() const;
    juce::StringArray getAvailableMasterPlugins() const;
    juce::StringArray getAvailableTrackInstrumentPlugins() const;
    void setProcessingConfig(double sampleRate, int blockSize);
    void rescanExternalPlugins();

    bool loadTrackPlugin(const juce::String& pluginName, int trackIndex, int slotIndex);
    bool hasTrackPlugin(int trackIndex, int slotIndex) const;
    bool removeTrackPlugin(int trackIndex, int slotIndex);
    bool loadTrackInstrumentPlugin(const juce::String& pluginName, int trackIndex);
    bool hasTrackInstrumentPlugin(int trackIndex) const;
    bool removeTrackInstrumentPlugin(int trackIndex);
    void removeTrackPluginSlot(int trackIndex, int slotIndex);
    void removeTrack(int trackIndex);
    bool setTrackPluginBypassed(int trackIndex, int slotIndex, bool shouldBeBypassed);
    bool showTrackPluginEditor(int trackIndex, int slotIndex);
    bool setTrackInstrumentPluginBypassed(int trackIndex, bool shouldBeBypassed);
    bool showTrackInstrumentPluginEditor(int trackIndex);
    bool renderTrackInstrument(int trackIndex,
                               juce::AudioBuffer<float>& buffer,
                               juce::MidiBuffer& midi,
                               double sampleRate);

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
        double preparedSampleRate = 0.0;
        int preparedBlockSize = 0;
        std::unique_ptr<juce::AudioProcessor> processor;
        std::unique_ptr<PluginEditorWindow> editorWindow;
    };

    struct ExternalPluginInfo
    {
        juce::String menuName;
        PluginRole role = PluginRole::effect;
        juce::PluginDescription description;
    };

    static juce::StringArray getBuiltInPluginNames(PluginRole role);
    juce::StringArray getAvailablePluginsForRole(PluginRole role) const;
    static void ensureProcessorPrepared(HostedPlugin& hostedPlugin, double sampleRate, int blockSize);
    std::unique_ptr<juce::AudioProcessor> createProcessor(const juce::String& pluginName) const;
    juce::File findExternalPluginsDirectory() const;
    static juce::String makeExternalPluginMenuName(const juce::PluginDescription& description);
    void scanExternalPlugins() const;
    HostedPlugin* getHostedTrackPlugin(int trackIndex, int slotIndex);
    const HostedPlugin* getHostedTrackPlugin(int trackIndex, int slotIndex) const;
    HostedPlugin* getHostedTrackInstrumentPlugin(int trackIndex);
    const HostedPlugin* getHostedTrackInstrumentPlugin(int trackIndex) const;
    HostedPlugin* getHostedMasterPlugin(int slotIndex);
    const HostedPlugin* getHostedMasterPlugin(int slotIndex) const;

    mutable juce::AudioPluginFormatManager formatManager;
    mutable std::vector<ExternalPluginInfo> externalPlugins;
    mutable bool externalPluginsScanned = false;
    std::map<SlotKey, HostedPlugin> hostedTrackPlugins;
    std::map<int, HostedPlugin> hostedTrackInstrumentPlugins;
    std::map<int, HostedPlugin> hostedMasterPlugins;
    double processingSampleRate = 44100.0;
    int processingBlockSize = 512;
};
