#include "pluginhostmanager.h"

#include "../Plugin/PluginProcessor.h"

namespace
{
    constexpr double pluginSampleRate = 44100.0;
    constexpr int pluginBlockSize = 512;
}

PluginHostManager::PluginHostManager() = default;
PluginHostManager::~PluginHostManager() = default;

PluginHostManager::PluginEditorWindow::PluginEditorWindow(const juce::String& windowTitle,
                                                          std::unique_ptr<juce::AudioProcessorEditor> editorIn)
    : juce::DocumentWindow(windowTitle,
                           juce::Colours::black,
                           juce::DocumentWindow::closeButton),
      ownedEditor(std::move(editorIn))
{
    setUsingNativeTitleBar(true);
    setResizable(true, true);
    setContentOwned(ownedEditor.release(), true);
    centreWithSize(getWidth(), getHeight());
}

void PluginHostManager::PluginEditorWindow::closeButtonPressed()
{
    setVisible(false);
}

juce::StringArray PluginHostManager::getAvailableTrackPlugins() const
{
    return { "LowpassHighpassFilter" };
}

juce::StringArray PluginHostManager::getAvailableMasterPlugins() const
{
    return getAvailableTrackPlugins();
}

bool PluginHostManager::loadTrackPlugin(const juce::String& pluginName, int trackIndex, int slotIndex)
{
    auto processor = createProcessor(pluginName);
    if (processor == nullptr)
        return false;

    processor->prepareToPlay(pluginSampleRate, pluginBlockSize);

    HostedPlugin hostedPlugin;
    hostedPlugin.pluginName = pluginName;
    hostedPlugin.processor = std::move(processor);
    hostedTrackPlugins[{ trackIndex, slotIndex }] = std::move(hostedPlugin);
    return true;
}

bool PluginHostManager::hasTrackPlugin(int trackIndex, int slotIndex) const
{
    return getHostedTrackPlugin(trackIndex, slotIndex) != nullptr;
}

bool PluginHostManager::removeTrackPlugin(int trackIndex, int slotIndex)
{
    const auto it = hostedTrackPlugins.find({ trackIndex, slotIndex });
    if (it == hostedTrackPlugins.end())
        return false;

    if (it->second.processor != nullptr)
        it->second.processor->releaseResources();

    hostedTrackPlugins.erase(it);
    return true;
}

void PluginHostManager::removeTrackPluginSlot(int trackIndex, int slotIndex)
{
    std::map<SlotKey, HostedPlugin> updatedPlugins;

    for (auto& [slotKey, hostedPlugin] : hostedTrackPlugins)
    {
        if (slotKey.trackIndex != trackIndex)
        {
            updatedPlugins.emplace(slotKey, std::move(hostedPlugin));
            continue;
        }

        if (slotKey.slotIndex == slotIndex)
        {
            if (hostedPlugin.processor != nullptr)
                hostedPlugin.processor->releaseResources();
            continue;
        }

        auto updatedKey = slotKey;
        if (updatedKey.slotIndex > slotIndex)
            --updatedKey.slotIndex;

        updatedPlugins.emplace(updatedKey, std::move(hostedPlugin));
    }

    hostedTrackPlugins = std::move(updatedPlugins);
}

void PluginHostManager::removeTrack(int trackIndex)
{
    std::map<SlotKey, HostedPlugin> updatedPlugins;

    for (auto& [slotKey, hostedPlugin] : hostedTrackPlugins)
    {
        if (slotKey.trackIndex == trackIndex)
        {
            if (hostedPlugin.processor != nullptr)
                hostedPlugin.processor->releaseResources();
            continue;
        }

        auto updatedKey = slotKey;
        if (updatedKey.trackIndex > trackIndex)
            --updatedKey.trackIndex;

        updatedPlugins.emplace(updatedKey, std::move(hostedPlugin));
    }

    hostedTrackPlugins = std::move(updatedPlugins);
}

bool PluginHostManager::setTrackPluginBypassed(int trackIndex, int slotIndex, bool shouldBeBypassed)
{
    if (auto* hostedPlugin = getHostedTrackPlugin(trackIndex, slotIndex))
    {
        hostedPlugin->bypassed = shouldBeBypassed;
        return true;
    }

    return false;
}

bool PluginHostManager::showTrackPluginEditor(int trackIndex, int slotIndex)
{
    auto* hostedPlugin = getHostedTrackPlugin(trackIndex, slotIndex);
    if (hostedPlugin == nullptr || hostedPlugin->processor == nullptr || !hostedPlugin->processor->hasEditor())
        return false;

    if (hostedPlugin->editorWindow == nullptr)
    {
        auto editor = std::unique_ptr<juce::AudioProcessorEditor>(hostedPlugin->processor->createEditor());
        if (editor == nullptr)
            return false;

        const auto windowTitle = hostedPlugin->pluginName + " - Track " + juce::String(trackIndex + 1);
        hostedPlugin->editorWindow = std::make_unique<PluginEditorWindow>(windowTitle, std::move(editor));
    }

    hostedPlugin->editorWindow->setVisible(true);
    hostedPlugin->editorWindow->toFront(true);
    return true;
}

bool PluginHostManager::loadMasterPlugin(const juce::String& pluginName, int slotIndex)
{
    auto processor = createProcessor(pluginName);
    if (processor == nullptr)
        return false;

    processor->prepareToPlay(pluginSampleRate, pluginBlockSize);

    HostedPlugin hostedPlugin;
    hostedPlugin.pluginName = pluginName;
    hostedPlugin.processor = std::move(processor);
    hostedMasterPlugins[slotIndex] = std::move(hostedPlugin);
    return true;
}

bool PluginHostManager::hasMasterPlugin(int slotIndex) const
{
    return getHostedMasterPlugin(slotIndex) != nullptr;
}

bool PluginHostManager::removeMasterPlugin(int slotIndex)
{
    const auto it = hostedMasterPlugins.find(slotIndex);
    if (it == hostedMasterPlugins.end())
        return false;

    if (it->second.processor != nullptr)
        it->second.processor->releaseResources();

    hostedMasterPlugins.erase(it);
    return true;
}

void PluginHostManager::removeMasterPluginSlot(int slotIndex)
{
    std::map<int, HostedPlugin> updatedPlugins;

    for (auto& [existingSlotIndex, hostedPlugin] : hostedMasterPlugins)
    {
        if (existingSlotIndex == slotIndex)
        {
            if (hostedPlugin.processor != nullptr)
                hostedPlugin.processor->releaseResources();
            continue;
        }

        const int updatedSlotIndex = existingSlotIndex > slotIndex ? existingSlotIndex - 1 : existingSlotIndex;
        updatedPlugins.emplace(updatedSlotIndex, std::move(hostedPlugin));
    }

    hostedMasterPlugins = std::move(updatedPlugins);
}

bool PluginHostManager::setMasterPluginBypassed(int slotIndex, bool shouldBeBypassed)
{
    if (auto* hostedPlugin = getHostedMasterPlugin(slotIndex))
    {
        hostedPlugin->bypassed = shouldBeBypassed;
        return true;
    }

    return false;
}

bool PluginHostManager::showMasterPluginEditor(int slotIndex)
{
    auto* hostedPlugin = getHostedMasterPlugin(slotIndex);
    if (hostedPlugin == nullptr || hostedPlugin->processor == nullptr || !hostedPlugin->processor->hasEditor())
        return false;

    if (hostedPlugin->editorWindow == nullptr)
    {
        auto editor = std::unique_ptr<juce::AudioProcessorEditor>(hostedPlugin->processor->createEditor());
        if (editor == nullptr)
            return false;

        const auto windowTitle = hostedPlugin->pluginName + " - Master";
        hostedPlugin->editorWindow = std::make_unique<PluginEditorWindow>(windowTitle, std::move(editor));
    }

    hostedPlugin->editorWindow->setVisible(true);
    hostedPlugin->editorWindow->toFront(true);
    return true;
}

std::unique_ptr<juce::AudioProcessor> PluginHostManager::createProcessor(const juce::String& pluginName) const
{
    if (pluginName == "LowpassHighpassFilter")
        return std::make_unique<LowpassHighpassFilterAudioProcessor>();

    return nullptr;
}

PluginHostManager::HostedPlugin* PluginHostManager::getHostedTrackPlugin(int trackIndex, int slotIndex)
{
    const auto it = hostedTrackPlugins.find({ trackIndex, slotIndex });
    return it != hostedTrackPlugins.end() ? &it->second : nullptr;
}

const PluginHostManager::HostedPlugin* PluginHostManager::getHostedTrackPlugin(int trackIndex, int slotIndex) const
{
    const auto it = hostedTrackPlugins.find({ trackIndex, slotIndex });
    return it != hostedTrackPlugins.end() ? &it->second : nullptr;
}

PluginHostManager::HostedPlugin* PluginHostManager::getHostedMasterPlugin(int slotIndex)
{
    const auto it = hostedMasterPlugins.find(slotIndex);
    return it != hostedMasterPlugins.end() ? &it->second : nullptr;
}

const PluginHostManager::HostedPlugin* PluginHostManager::getHostedMasterPlugin(int slotIndex) const
{
    const auto it = hostedMasterPlugins.find(slotIndex);
    return it != hostedMasterPlugins.end() ? &it->second : nullptr;
}
