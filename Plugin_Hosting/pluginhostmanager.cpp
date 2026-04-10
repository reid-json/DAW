#include "pluginhostmanager.h"
#include "../Plugin/PluginProcessor.h"
#include <algorithm>

namespace { constexpr const char* externalPluginsPath = "Plugin/Plugins (External)"; }

PluginHostManager::PluginHostManager()  { formatManager.addDefaultFormats(); }
PluginHostManager::~PluginHostManager() = default;

PluginHostManager::PluginEditorWindow::PluginEditorWindow(const juce::String& title,
                                                          std::unique_ptr<juce::AudioProcessorEditor> ed)
    : juce::DocumentWindow(title, juce::Colours::black, juce::DocumentWindow::closeButton),
      ownedEditor(std::move(ed))
{
    setUsingNativeTitleBar(true);
    setResizable(true, true);
    setContentOwned(ownedEditor.release(), true);
    centreWithSize(getWidth(), getHeight());
}

void PluginHostManager::PluginEditorWindow::closeButtonPressed() { setVisible(false); }

// plugin lists

juce::StringArray PluginHostManager::getBuiltInPluginNames(PluginRole role)
{
    return role == PluginRole::effect ? juce::StringArray { "LowpassHighpassFilter" } : juce::StringArray {};
}

juce::StringArray PluginHostManager::getAvailablePluginsForRole(PluginRole role) const
{
    scanExternalPlugins();
    auto list = getBuiltInPluginNames(role);
    for (auto& ep : externalPlugins)
        if (ep.role == role) list.addIfNotAlreadyThere(ep.menuName);
    return list;
}

juce::StringArray PluginHostManager::getAvailableTrackPlugins() const           { return getAvailablePluginsForRole(PluginRole::effect); }
juce::StringArray PluginHostManager::getAvailableMasterPlugins() const          { return getAvailablePluginsForRole(PluginRole::effect); }
juce::StringArray PluginHostManager::getAvailableTrackInstrumentPlugins() const { return getAvailablePluginsForRole(PluginRole::instrument); }

// track plugins

bool PluginHostManager::loadTrackPlugin(const juce::String& name, int trackIndex, int slotIndex)
{
    auto proc = createProcessor(name);
    if (proc == nullptr) return false;
    proc->prepareToPlay(processingSampleRate, processingBlockSize);

    HostedPlugin hp;
    hp.pluginName = name;
    hp.preparedSampleRate = processingSampleRate;
    hp.preparedBlockSize = processingBlockSize;
    hp.processor = std::move(proc);
    hostedTrackPlugins[{ trackIndex, slotIndex }] = std::move(hp);
    return true;
}

juce::String PluginHostManager::getTrackPluginName(int trackIndex, int slotIndex) const
{
    if (auto* p = getHostedTrackPlugin(trackIndex, slotIndex)) return p->pluginName;
    return {};
}

bool PluginHostManager::removeTrackPlugin(int trackIndex, int slotIndex)
{
    auto it = hostedTrackPlugins.find({ trackIndex, slotIndex });
    if (it == hostedTrackPlugins.end()) return false;
    if (it->second.processor) it->second.processor->releaseResources();
    hostedTrackPlugins.erase(it);
    return true;
}

bool PluginHostManager::showTrackPluginEditor(int trackIndex, int slotIndex)
{
    auto* hp = getHostedTrackPlugin(trackIndex, slotIndex);
    if (! hp || ! hp->processor || ! hp->processor->hasEditor()) return false;

    if (! hp->editorWindow)
    {
        auto ed = std::unique_ptr<juce::AudioProcessorEditor>(hp->processor->createEditor());
        if (! ed) return false;
        hp->editorWindow = std::make_unique<PluginEditorWindow>(
            hp->pluginName + " - Track " + juce::String(trackIndex + 1), std::move(ed));
    }
    hp->editorWindow->setVisible(true);
    hp->editorWindow->toFront(true);
    return true;
}

// track instruments

bool PluginHostManager::loadTrackInstrumentPlugin(const juce::String& name, int trackIndex)
{
    auto proc = createProcessor(name);
    if (proc == nullptr) return false;
    proc->prepareToPlay(processingSampleRate, processingBlockSize);

    HostedPlugin hp;
    hp.pluginName = name;
    hp.preparedSampleRate = processingSampleRate;
    hp.preparedBlockSize = processingBlockSize;
    hp.processor = std::move(proc);
    hostedTrackInstrumentPlugins[trackIndex] = std::move(hp);
    return true;
}

bool PluginHostManager::renderTrackInstrument(int trackIndex,
                                              juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midi,
                                              double /*sampleRate*/)
{
    auto* hp = getHostedTrackInstrumentPlugin(trackIndex);
    if (! hp || ! hp->processor || hp->bypassed) return false;

    int numSamples = buffer.getNumSamples();
    int numCh = juce::jmax(2, hp->processor->getTotalNumInputChannels(),
                              hp->processor->getTotalNumOutputChannels());

    juce::AudioBuffer<float> plugBuf(numCh, numSamples);
    plugBuf.clear();
    for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), plugBuf.getNumChannels()); ++ch)
        plugBuf.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    hp->processor->processBlock(plugBuf, midi);

    buffer.clear();
    for (int ch = 0; ch < juce::jmin(2, plugBuf.getNumChannels()); ++ch)
        buffer.addFrom(ch, 0, plugBuf, juce::jmin(ch, plugBuf.getNumChannels() - 1), 0, numSamples);

    return true;
}

// track removal

void PluginHostManager::removeTrack(int trackIndex)
{
    // Re-index track effect plugins
    std::map<SlotKey, HostedPlugin> updated;
    for (auto& [key, hp] : hostedTrackPlugins)
    {
        if (key.trackIndex == trackIndex)
        {
            if (hp.processor) hp.processor->releaseResources();
            continue;
        }
        auto k = key;
        if (k.trackIndex > trackIndex) --k.trackIndex;
        updated.emplace(k, std::move(hp));
    }
    hostedTrackPlugins = std::move(updated);

    // Re-index track instrument plugins
    std::map<int, HostedPlugin> updatedInst;
    for (auto& [idx, hp] : hostedTrackInstrumentPlugins)
    {
        if (idx == trackIndex)
        {
            if (hp.processor) hp.processor->releaseResources();
            continue;
        }
        updatedInst.emplace(idx > trackIndex ? idx - 1 : idx, std::move(hp));
    }
    hostedTrackInstrumentPlugins = std::move(updatedInst);
}

// master plugins

bool PluginHostManager::loadMasterPlugin(const juce::String& name, int slotIndex)
{
    auto proc = createProcessor(name);
    if (proc == nullptr) return false;
    proc->prepareToPlay(processingSampleRate, processingBlockSize);

    HostedPlugin hp;
    hp.pluginName = name;
    hp.preparedSampleRate = processingSampleRate;
    hp.preparedBlockSize = processingBlockSize;
    hp.processor = std::move(proc);
    hostedMasterPlugins[slotIndex] = std::move(hp);
    return true;
}

juce::String PluginHostManager::getMasterPluginName(int slotIndex) const
{
    if (auto* p = getHostedMasterPlugin(slotIndex)) return p->pluginName;
    return {};
}

bool PluginHostManager::removeMasterPlugin(int slotIndex)
{
    auto it = hostedMasterPlugins.find(slotIndex);
    if (it == hostedMasterPlugins.end()) return false;
    if (it->second.processor) it->second.processor->releaseResources();
    hostedMasterPlugins.erase(it);
    return true;
}

bool PluginHostManager::showMasterPluginEditor(int slotIndex)
{
    auto* hp = getHostedMasterPlugin(slotIndex);
    if (! hp || ! hp->processor || ! hp->processor->hasEditor()) return false;

    if (! hp->editorWindow)
    {
        auto ed = std::unique_ptr<juce::AudioProcessorEditor>(hp->processor->createEditor());
        if (! ed) return false;
        hp->editorWindow = std::make_unique<PluginEditorWindow>(hp->pluginName + " - Master", std::move(ed));
    }
    hp->editorWindow->setVisible(true);
    hp->editorWindow->toFront(true);
    return true;
}

// FX processing

void PluginHostManager::processTrackEffects(int trackIndex, juce::AudioBuffer<float>& buffer)
{
    for (int slot = 0; slot < 5; ++slot)
    {
        auto* hp = getHostedTrackPlugin(trackIndex, slot);
        if (hp == nullptr || hp->processor == nullptr || hp->bypassed)
            continue;

        int numSamples = buffer.getNumSamples();
        int numCh = juce::jmax(2, hp->processor->getTotalNumInputChannels(),
                                  hp->processor->getTotalNumOutputChannels());

        juce::AudioBuffer<float> plugBuf(numCh, numSamples);
        plugBuf.clear();
        for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), plugBuf.getNumChannels()); ++ch)
            plugBuf.copyFrom(ch, 0, buffer, ch, 0, numSamples);

        juce::MidiBuffer emptyMidi;
        hp->processor->processBlock(plugBuf, emptyMidi);

        for (int ch = 0; ch < juce::jmin(2, plugBuf.getNumChannels()); ++ch)
            buffer.copyFrom(ch, 0, plugBuf, juce::jmin(ch, plugBuf.getNumChannels() - 1), 0, numSamples);
    }
}

void PluginHostManager::processMasterEffects(juce::AudioBuffer<float>& buffer)
{
    for (int slot = 0; slot < 5; ++slot)
    {
        auto* hp = getHostedMasterPlugin(slot);
        if (hp == nullptr || hp->processor == nullptr || hp->bypassed)
            continue;

        int numSamples = buffer.getNumSamples();
        int numCh = juce::jmax(2, hp->processor->getTotalNumInputChannels(),
                                  hp->processor->getTotalNumOutputChannels());

        juce::AudioBuffer<float> plugBuf(numCh, numSamples);
        plugBuf.clear();
        for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), plugBuf.getNumChannels()); ++ch)
            plugBuf.copyFrom(ch, 0, buffer, ch, 0, numSamples);

        juce::MidiBuffer emptyMidi;
        hp->processor->processBlock(plugBuf, emptyMidi);

        for (int ch = 0; ch < juce::jmin(2, plugBuf.getNumChannels()); ++ch)
            buffer.copyFrom(ch, 0, plugBuf, juce::jmin(ch, plugBuf.getNumChannels() - 1), 0, numSamples);
    }
}

void PluginHostManager::rescanExternalPlugins()
{
    externalPluginsScanned = false;
    scanExternalPlugins();
}

bool PluginHostManager::loadPianoRollInstrument(const juce::String& pluginName)
{
    return loadTrackInstrumentPlugin(pluginName, -1);
}

juce::String PluginHostManager::getPianoRollInstrumentName() const
{
    auto* hp = getHostedTrackInstrumentPlugin(-1);
    return hp != nullptr ? hp->pluginName : juce::String();
}

bool PluginHostManager::renderPianoRollInstrument(juce::AudioBuffer<float>& buffer,
                                                   juce::MidiBuffer& midi,
                                                   double sampleRate)
{
    return renderTrackInstrument(-1, buffer, midi, sampleRate);
}

// plugin creation and scanning

std::unique_ptr<juce::AudioProcessor> PluginHostManager::createProcessor(const juce::String& name) const
{
    if (name == "LowpassHighpassFilter")
        return std::make_unique<LowpassHighpassFilterAudioProcessor>();

    scanExternalPlugins();

    auto it = std::find_if(externalPlugins.begin(), externalPlugins.end(),
                           [&](auto& ep) { return ep.menuName == name; });
    if (it == externalPlugins.end()) return nullptr;

    juce::String err;
    return std::unique_ptr<juce::AudioProcessor>(
        formatManager.createPluginInstance(it->description, processingSampleRate, processingBlockSize, err));
}

juce::File PluginHostManager::findExternalPluginsDirectory() const
{
    auto cwd = juce::File::getCurrentWorkingDirectory().getChildFile(externalPluginsPath);
    if (cwd.isDirectory()) return cwd;

    auto dir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();
    for (int i = 0; i < 6; ++i)
    {
        auto c = dir.getChildFile(externalPluginsPath);
        if (c.isDirectory()) return c;
        dir = dir.getParentDirectory();
    }
    return {};
}

juce::String PluginHostManager::makeExternalPluginMenuName(const juce::PluginDescription& desc)
{
    auto vendor = desc.manufacturerName.trim();
    return vendor.isNotEmpty() ? desc.name + " (" + vendor + ")" : desc.name;
}

void PluginHostManager::scanExternalPlugins() const
{
    if (externalPluginsScanned) return;
    externalPluginsScanned = true;
    externalPlugins.clear();

    auto dir = findExternalPluginsDirectory();
    if (! dir.isDirectory()) return;

    juce::Array<juce::File> vst3Files;
    dir.findChildFiles(vst3Files, juce::File::findFiles, true, "*.vst3");

    for (auto& file : vst3Files)
    {
        for (auto* fmt : formatManager.getFormats())
        {
            if (! fmt || ! fmt->fileMightContainThisPluginType(file.getFullPathName())) continue;

            juce::OwnedArray<juce::PluginDescription> descs;
            fmt->findAllTypesForFile(descs, file.getFullPathName());

            for (auto* d : descs)
                if (d) externalPlugins.push_back({ makeExternalPluginMenuName(*d),
                                                   d->isInstrument ? PluginRole::instrument : PluginRole::effect,
                                                   *d });
        }
    }
}

// lookup helpers

PluginHostManager::HostedPlugin* PluginHostManager::getHostedTrackPlugin(int ti, int si)
{
    auto it = hostedTrackPlugins.find({ ti, si });
    return it != hostedTrackPlugins.end() ? &it->second : nullptr;
}

const PluginHostManager::HostedPlugin* PluginHostManager::getHostedTrackPlugin(int ti, int si) const
{
    auto it = hostedTrackPlugins.find({ ti, si });
    return it != hostedTrackPlugins.end() ? &it->second : nullptr;
}

PluginHostManager::HostedPlugin* PluginHostManager::getHostedTrackInstrumentPlugin(int ti)
{
    auto it = hostedTrackInstrumentPlugins.find(ti);
    return it != hostedTrackInstrumentPlugins.end() ? &it->second : nullptr;
}

const PluginHostManager::HostedPlugin* PluginHostManager::getHostedTrackInstrumentPlugin(int ti) const
{
    auto it = hostedTrackInstrumentPlugins.find(ti);
    return it != hostedTrackInstrumentPlugins.end() ? &it->second : nullptr;
}

PluginHostManager::HostedPlugin* PluginHostManager::getHostedMasterPlugin(int si)
{
    auto it = hostedMasterPlugins.find(si);
    return it != hostedMasterPlugins.end() ? &it->second : nullptr;
}

const PluginHostManager::HostedPlugin* PluginHostManager::getHostedMasterPlugin(int si) const
{
    auto it = hostedMasterPlugins.find(si);
    return it != hostedMasterPlugins.end() ? &it->second : nullptr;
}
