#include "pluginhostmanager.h"

#include "../Plugin/PluginProcessor.h"

#include <algorithm>

namespace
{
    constexpr const char* externalPluginsRelativePath = "Plugin/Plugins (External)";
}

PluginHostManager::PluginHostManager()
{
    formatManager.addDefaultFormats();
}

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

void PluginHostManager::setProcessingConfig(double sampleRate, int blockSize)
{
    if (sampleRate > 0.0)
        processingSampleRate = sampleRate;

    if (blockSize > 0)
        processingBlockSize = blockSize;
}

void PluginHostManager::ensureProcessorPrepared(HostedPlugin& hostedPlugin, double sampleRate, int blockSize)
{
    if (hostedPlugin.processor == nullptr)
        return;

    if (hostedPlugin.preparedSampleRate == sampleRate && hostedPlugin.preparedBlockSize == blockSize)
        return;

    if (hostedPlugin.preparedSampleRate > 0.0)
        hostedPlugin.processor->releaseResources();

    hostedPlugin.processor->prepareToPlay(sampleRate, blockSize);
    hostedPlugin.preparedSampleRate = sampleRate;
    hostedPlugin.preparedBlockSize = blockSize;
}

juce::StringArray PluginHostManager::getBuiltInPluginNames(PluginRole role)
{
    if (role == PluginRole::effect)
        return { "LowpassHighpassFilter" };

    return {};
}

juce::StringArray PluginHostManager::getAvailablePluginsForRole(PluginRole role) const
{
    externalPluginsScanned = false;
    scanExternalPlugins();

    auto plugins = getBuiltInPluginNames(role);
    for (const auto& plugin : externalPlugins)
        if (plugin.role == role)
            plugins.addIfNotAlreadyThere(plugin.menuName);

    return plugins;
}

juce::StringArray PluginHostManager::getAvailableTrackPlugins() const
{
    return getAvailablePluginsForRole(PluginRole::effect);
}

juce::StringArray PluginHostManager::getAvailableMasterPlugins() const
{
    return getAvailablePluginsForRole(PluginRole::effect);
}

juce::StringArray PluginHostManager::getAvailableTrackInstrumentPlugins() const
{
    return getAvailablePluginsForRole(PluginRole::instrument);
}

void PluginHostManager::rescanExternalPlugins()
{
    externalPluginsScanned = false;
    scanExternalPlugins();
}

bool PluginHostManager::loadTrackPlugin(const juce::String& pluginName, int trackIndex, int slotIndex)
{
    auto processor = createProcessor(pluginName);
    if (processor == nullptr)
        return false;

    processor->prepareToPlay(processingSampleRate, processingBlockSize);

    HostedPlugin hostedPlugin;
    hostedPlugin.pluginName = pluginName;
    hostedPlugin.preparedSampleRate = processingSampleRate;
    hostedPlugin.preparedBlockSize = processingBlockSize;
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

bool PluginHostManager::loadTrackInstrumentPlugin(const juce::String& pluginName, int trackIndex)
{
    auto processor = createProcessor(pluginName);
    if (processor == nullptr)
        return false;

    processor->prepareToPlay(processingSampleRate, processingBlockSize);

    HostedPlugin hostedPlugin;
    hostedPlugin.pluginName = pluginName;
    hostedPlugin.preparedSampleRate = processingSampleRate;
    hostedPlugin.preparedBlockSize = processingBlockSize;
    hostedPlugin.processor = std::move(processor);
    hostedTrackInstrumentPlugins[trackIndex] = std::move(hostedPlugin);
    return true;
}

bool PluginHostManager::hasTrackInstrumentPlugin(int trackIndex) const
{
    return getHostedTrackInstrumentPlugin(trackIndex) != nullptr;
}

bool PluginHostManager::removeTrackInstrumentPlugin(int trackIndex)
{
    const auto it = hostedTrackInstrumentPlugins.find(trackIndex);
    if (it == hostedTrackInstrumentPlugins.end())
        return false;

    if (it->second.processor != nullptr)
        it->second.processor->releaseResources();

    hostedTrackInstrumentPlugins.erase(it);
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

    std::map<int, HostedPlugin> updatedInstrumentPlugins;
    for (auto& [existingTrackIndex, hostedPlugin] : hostedTrackInstrumentPlugins)
    {
        if (existingTrackIndex == trackIndex)
        {
            if (hostedPlugin.processor != nullptr)
                hostedPlugin.processor->releaseResources();
            continue;
        }

        const int updatedTrackIndex = existingTrackIndex > trackIndex ? existingTrackIndex - 1 : existingTrackIndex;
        updatedInstrumentPlugins.emplace(updatedTrackIndex, std::move(hostedPlugin));
    }

    hostedTrackInstrumentPlugins = std::move(updatedInstrumentPlugins);
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

bool PluginHostManager::setTrackInstrumentPluginBypassed(int trackIndex, bool shouldBeBypassed)
{
    if (auto* hostedPlugin = getHostedTrackInstrumentPlugin(trackIndex))
    {
        hostedPlugin->bypassed = shouldBeBypassed;
        return true;
    }

    return false;
}

bool PluginHostManager::showTrackInstrumentPluginEditor(int trackIndex)
{
    auto* hostedPlugin = getHostedTrackInstrumentPlugin(trackIndex);
    if (hostedPlugin == nullptr || hostedPlugin->processor == nullptr || !hostedPlugin->processor->hasEditor())
        return false;

    if (hostedPlugin->editorWindow == nullptr)
    {
        auto editor = std::unique_ptr<juce::AudioProcessorEditor>(hostedPlugin->processor->createEditor());
        if (editor == nullptr)
            return false;

        const auto windowTitle = hostedPlugin->pluginName + " - Track " + juce::String(trackIndex + 1) + " Instrument";
        hostedPlugin->editorWindow = std::make_unique<PluginEditorWindow>(windowTitle, std::move(editor));
    }

    hostedPlugin->editorWindow->setVisible(true);
    hostedPlugin->editorWindow->toFront(true);
    return true;
}

bool PluginHostManager::renderTrackInstrument(int trackIndex,
                                              juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midi,
                                              double sampleRate)
{
    juce::ignoreUnused(sampleRate);

    auto* hostedPlugin = getHostedTrackInstrumentPlugin(trackIndex);
    if (hostedPlugin == nullptr || hostedPlugin->processor == nullptr || hostedPlugin->bypassed)
        return false;

    const int numSamples = buffer.getNumSamples();
    const int requiredChannels = juce::jmax(2,
                                            hostedPlugin->processor->getTotalNumInputChannels(),
                                            hostedPlugin->processor->getTotalNumOutputChannels());

    juce::AudioBuffer<float> pluginBuffer(requiredChannels, numSamples);
    pluginBuffer.clear();

    const int channelsToCopyIn = juce::jmin(buffer.getNumChannels(), pluginBuffer.getNumChannels());
    for (int channel = 0; channel < channelsToCopyIn; ++channel)
        pluginBuffer.copyFrom(channel, 0, buffer, channel, 0, numSamples);

    hostedPlugin->processor->processBlock(pluginBuffer, midi);

    buffer.clear();
    if (pluginBuffer.getNumChannels() >= 2)
    {
        buffer.addFrom(0, 0, pluginBuffer, 0, 0, numSamples);
        buffer.addFrom(1, 0, pluginBuffer, 1, 0, numSamples);
    }
    else if (pluginBuffer.getNumChannels() == 1)
    {
        buffer.addFrom(0, 0, pluginBuffer, 0, 0, numSamples);
        if (buffer.getNumChannels() > 1)
            buffer.addFrom(1, 0, pluginBuffer, 0, 0, numSamples);
    }

    return true;
}

bool PluginHostManager::loadMasterPlugin(const juce::String& pluginName, int slotIndex)
{
    auto processor = createProcessor(pluginName);
    if (processor == nullptr)
        return false;

    processor->prepareToPlay(processingSampleRate, processingBlockSize);

    HostedPlugin hostedPlugin;
    hostedPlugin.pluginName = pluginName;
    hostedPlugin.preparedSampleRate = processingSampleRate;
    hostedPlugin.preparedBlockSize = processingBlockSize;
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

    scanExternalPlugins();

    const auto it = std::find_if (externalPlugins.begin(), externalPlugins.end(),
                                  [&pluginName] (const ExternalPluginInfo& plugin)
                                  {
                                      return plugin.menuName == pluginName;
                                  });

    if (it == externalPlugins.end())
        return nullptr;

    juce::String errorMessage;
    auto instance = formatManager.createPluginInstance (it->description,
                                                        processingSampleRate,
                                                        processingBlockSize,
                                                        errorMessage);
    return std::unique_ptr<juce::AudioProcessor> (std::move (instance));
}

juce::File PluginHostManager::findExternalPluginsDirectory() const
{
    const auto cwdCandidate = juce::File::getCurrentWorkingDirectory().getChildFile (externalPluginsRelativePath);
    if (cwdCandidate.isDirectory())
        return cwdCandidate;

    auto exeDirectory = juce::File::getSpecialLocation (juce::File::currentExecutableFile).getParentDirectory();
    for (int i = 0; i < 6; ++i)
    {
        const auto candidate = exeDirectory.getChildFile (externalPluginsRelativePath);
        if (candidate.isDirectory())
            return candidate;

        exeDirectory = exeDirectory.getParentDirectory();
    }

    return {};
}

juce::String PluginHostManager::makeExternalPluginMenuName(const juce::PluginDescription& description)
{
    const auto vendor = description.manufacturerName.trim();
    return vendor.isNotEmpty() ? description.name + " (" + vendor + ")" : description.name;
}

void PluginHostManager::scanExternalPlugins() const
{
    if (externalPluginsScanned)
        return;

    externalPluginsScanned = true;
    externalPlugins.clear();

    const auto pluginDirectory = findExternalPluginsDirectory();
    if (! pluginDirectory.isDirectory())
        return;

    juce::Array<juce::File> vst3Files;
    pluginDirectory.findChildFiles (vst3Files, juce::File::findFiles, true, "*.vst3");

    for (const auto& pluginFile : vst3Files)
    {
        for (auto* format : formatManager.getFormats())
        {
            if (format == nullptr || ! format->fileMightContainThisPluginType (pluginFile.getFullPathName()))
                continue;

            juce::OwnedArray<juce::PluginDescription> descriptions;
            format->findAllTypesForFile (descriptions, pluginFile.getFullPathName());

            for (const auto* description : descriptions)
            {
                if (description == nullptr)
                    continue;

                externalPlugins.push_back ({
                    makeExternalPluginMenuName (*description),
                    description->isInstrument ? PluginRole::instrument : PluginRole::effect,
                    *description
                });
            }
        }
    }
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

PluginHostManager::HostedPlugin* PluginHostManager::getHostedTrackInstrumentPlugin(int trackIndex)
{
    const auto it = hostedTrackInstrumentPlugins.find(trackIndex);
    return it != hostedTrackInstrumentPlugins.end() ? &it->second : nullptr;
}

const PluginHostManager::HostedPlugin* PluginHostManager::getHostedTrackInstrumentPlugin(int trackIndex) const
{
    const auto it = hostedTrackInstrumentPlugins.find(trackIndex);
    return it != hostedTrackInstrumentPlugins.end() ? &it->second : nullptr;
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
