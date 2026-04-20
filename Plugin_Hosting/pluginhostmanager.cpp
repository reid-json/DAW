#include "pluginhostmanager.h"
#include "../Plugin/LowHighFilter/PluginProcessor.h"
#include "../Plugin/Compressor/PluginProcessor.h"
#include "../Plugin/SevenBandEQ/PluginProcessor.h"
#include "../Plugin/GainPanFilter/PluginProcessor.h"
#include <algorithm>

namespace { constexpr const char* externalPluginsPath = "ExternalPlugins"; }

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

juce::StringArray PluginHostManager::getBuiltInPluginNames(PluginRole role)
{
    if (role == PluginRole::effect)
        return juce::StringArray { "LowpassHighpassFilter", "Compressor", "SevenBandEQ", "GainPanFilter" };
    return juce::StringArray {};
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

std::optional<PluginHostManager::HostedPlugin> PluginHostManager::makeHostedPlugin(const juce::String& name) const
{
    auto proc = createProcessor(name);
    if (proc == nullptr) return std::nullopt;
    proc->prepareToPlay(processingSampleRate, processingBlockSize);

    HostedPlugin hp;
    hp.pluginName = name;
    hp.preparedSampleRate = processingSampleRate;
    hp.preparedBlockSize = processingBlockSize;
    hp.processor = std::move(proc);
    return hp;
}

void PluginHostManager::runPluginOnBuffer(HostedPlugin& hp, juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) const
{
    const int numSamples = buffer.getNumSamples();
    const int numCh = juce::jmax(2, hp.processor->getTotalNumInputChannels(),
                                    hp.processor->getTotalNumOutputChannels());

    juce::AudioBuffer<float> plugBuf(numCh, numSamples);
    plugBuf.clear();
    for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), plugBuf.getNumChannels()); ++ch)
        plugBuf.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    hp.processor->processBlock(plugBuf, midi);

    for (int ch = 0; ch < juce::jmin(2, plugBuf.getNumChannels()); ++ch)
        buffer.copyFrom(ch, 0, plugBuf, juce::jmin(ch, plugBuf.getNumChannels() - 1), 0, numSamples);
}

bool PluginHostManager::loadTrackPlugin(const juce::String& name, int trackIndex, int slotIndex)
{
    auto hp = makeHostedPlugin(name);
    if (! hp) return false;
    hostedTrackPlugins[{ trackIndex, slotIndex }] = std::move(*hp);
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

bool PluginHostManager::loadTrackInstrumentPlugin(const juce::String& name, int trackIndex)
{
    auto hp = makeHostedPlugin(name);
    if (! hp) return false;
    hostedTrackInstrumentPlugins[trackIndex] = std::move(*hp);
    return true;
}

bool PluginHostManager::renderTrackInstrument(int trackIndex,
                                              juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midi,
                                              double)
{
    auto* hp = getHostedTrackInstrumentPlugin(trackIndex);
    if (! hp || ! hp->processor || hp->bypassed) return false;

    buffer.clear();
    runPluginOnBuffer(*hp, buffer, midi);
    return true;
}

void PluginHostManager::removeTrack(int trackIndex)
{
    std::map<SlotKey, HostedPlugin> updated;
    for (auto& entry : hostedTrackPlugins)
    {
        if (entry.first.trackIndex == trackIndex)
        {
            if (entry.second.processor) entry.second.processor->releaseResources();
            continue;
        }
        auto k = entry.first;
        if (k.trackIndex > trackIndex) --k.trackIndex;
        updated.emplace(k, std::move(entry.second));
    }
    hostedTrackPlugins = std::move(updated);

    std::map<int, HostedPlugin> updatedInst;
    for (auto& entry : hostedTrackInstrumentPlugins)
    {
        int idx = entry.first;
        if (idx == trackIndex)
        {
            if (entry.second.processor) entry.second.processor->releaseResources();
            continue;
        }
        updatedInst.emplace(idx > trackIndex ? idx - 1 : idx, std::move(entry.second));
    }
    hostedTrackInstrumentPlugins = std::move(updatedInst);
}

bool PluginHostManager::loadMasterPlugin(const juce::String& name, int slotIndex)
{
    auto hp = makeHostedPlugin(name);
    if (! hp) return false;
    hostedMasterPlugins[slotIndex] = std::move(*hp);
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

void PluginHostManager::processTrackEffects(int trackIndex, juce::AudioBuffer<float>& buffer)
{
    juce::MidiBuffer emptyMidi;
    for (int slot = 0; slot < 5; ++slot)
    {
        auto* hp = getHostedTrackPlugin(trackIndex, slot);
        if (hp && hp->processor && ! hp->bypassed)
            runPluginOnBuffer(*hp, buffer, emptyMidi);
    }
}

void PluginHostManager::processMasterEffects(juce::AudioBuffer<float>& buffer)
{
    juce::MidiBuffer emptyMidi;
    for (int slot = 0; slot < 5; ++slot)
    {
        auto* hp = getHostedMasterPlugin(slot);
        if (hp && hp->processor && ! hp->bypassed)
            runPluginOnBuffer(*hp, buffer, emptyMidi);
    }
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

std::unique_ptr<juce::AudioProcessor> PluginHostManager::createProcessor(const juce::String& name) const
{
    if (name == "LowpassHighpassFilter") return std::make_unique<LowpassHighpassFilterAudioProcessor>();
    if (name == "Compressor")            return std::make_unique<CompressorAudioProcessor>();
    if (name == "SevenBandEQ")           return std::make_unique<SevenBandEQAudioProcessor>();
    if (name == "GainPanFilter")         return std::make_unique<GainPanAudioProcessor>();

    scanExternalPlugins();

    for (auto& ep : externalPlugins)
    {
        if (ep.menuName == name)
        {
            juce::String err;
            return std::unique_ptr<juce::AudioProcessor>(
                formatManager.createPluginInstance(ep.description, processingSampleRate, processingBlockSize, err));
        }
    }

    return nullptr;
}

juce::File PluginHostManager::getExternalPluginsDir() const
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

juce::String PluginHostManager::makeMenuNameForPlugin(const juce::PluginDescription& desc)
{
    auto vendor = desc.manufacturerName.trim();
    return vendor.isNotEmpty() ? desc.name + " (" + vendor + ")" : desc.name;
}

void PluginHostManager::scanExternalPlugins() const
{
    if (externalPluginsScanned) return;
    externalPluginsScanned = true;
    externalPlugins.clear();

    auto dir = getExternalPluginsDir();
    if (! dir.isDirectory()) return;

    juce::Array<juce::File> vst3Files;
    dir.findChildFiles(vst3Files, juce::File::findFiles, true, "*.vst3");

    for (auto& file : vst3Files)
        for (auto* fmt : formatManager.getFormats())
        {
            if (! fmt || ! fmt->fileMightContainThisPluginType(file.getFullPathName())) continue;

            const bool isInInstPluginFolder = file.getFullPathName().contains("InstPlugin");

            juce::OwnedArray<juce::PluginDescription> descs;
            fmt->findAllTypesForFile(descs, file.getFullPathName());
            for (auto* d : descs)
                if (d) externalPlugins.push_back({ makeMenuNameForPlugin(*d),
                                                   isInInstPluginFolder ? PluginRole::instrument : PluginRole::effect,
                                                   *d });
        }
}

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
