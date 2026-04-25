#include "pluginhostmanager.h"
#include "../Plugin/LowHighFilter/PluginProcessor.h"
#include "../Plugin/Compressor/PluginProcessor.h"
#include "../Plugin/SevenBandEQ/PluginProcessor.h"
#include "../Plugin/GainPanFilter/PluginProcessor.h"
#include "../Plugin/BuiltInPluginTheme.h"
#include <algorithm>

namespace { constexpr const char* externalPluginsPath = "ExternalPlugins"; }

PluginHostManager::PluginHostManager()  { formatManager.addDefaultFormats(); }
PluginHostManager::~PluginHostManager() = default;

PluginHostManager::PluginEditorWindow::PluginEditorWindow(const juce::String& title,
                                                          std::unique_ptr<juce::AudioProcessorEditor> ed,
                                                          bool useBuiltInTheme)
    : juce::DocumentWindow(title, juce::Colours::black, juce::DocumentWindow::closeButton),
      builtInThemeEnabled(useBuiltInTheme),
      ownedEditor(std::move(ed))
{
    setUsingNativeTitleBar(true);
    setResizable(true, true);
    setContentOwned(ownedEditor.release(), true);
    centreWithSize(getWidth(), getHeight());
    refreshTheme();
}

void PluginHostManager::PluginEditorWindow::closeButtonPressed() { setVisible(false); }

void PluginHostManager::PluginEditorWindow::refreshTheme()
{
    if (! builtInThemeEnabled)
        return;

    setBackgroundColour (BuiltInPluginTheme::getPalette().background);

    if (auto* content = getContentComponent())
    {
        content->sendLookAndFeelChange();
        content->repaint();
    }

    repaint();
}

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

    proc->enableAllBuses();

    if (proc->getBusCount(false) > 0 && proc->getMainBusNumOutputChannels() == 0)
        proc->setChannelLayoutOfBus(false, 0, juce::AudioChannelSet::stereo());

    if (proc->getBusCount(true) > 0 && proc->getMainBusNumInputChannels() == 0 && ! proc->acceptsMidi())
        proc->setChannelLayoutOfBus(true, 0, juce::AudioChannelSet::stereo());

    proc->setRateAndBufferSizeDetails(processingSampleRate, processingBlockSize);
    proc->prepareToPlay(processingSampleRate, processingBlockSize);

    HostedPlugin hp;
    hp.pluginName = name;
    hp.preparedSampleRate = processingSampleRate;
    hp.preparedBlockSize = processingBlockSize;
    hp.processor = std::move(proc);
    return hp;
}

void PluginHostManager::releaseHostedPlugin(HostedPlugin& hp) const
{
    if (hp.editorWindow != nullptr)
    {
        hp.editorWindow->setVisible(false);
        hp.editorWindow.reset();
    }

    if (hp.processor != nullptr)
        hp.processor->releaseResources();
}

void PluginHostManager::runPluginOnBuffer(HostedPlugin& hp, juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                                          bool mixAllOutputsToStereo) const
{
    const int numSamples = buffer.getNumSamples();
    const int numCh = juce::jmax(2, hp.processor->getTotalNumInputChannels(),
                                    hp.processor->getTotalNumOutputChannels());

    juce::AudioBuffer<float> plugBuf(numCh, numSamples);
    plugBuf.clear();
    for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), plugBuf.getNumChannels()); ++ch)
        plugBuf.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    hp.processor->processBlock(plugBuf, midi);

    if (mixAllOutputsToStereo)
    {
        for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), plugBuf.getNumChannels(), 2); ++ch)
            buffer.copyFrom(ch, 0, plugBuf, ch, 0, numSamples);

        for (int ch = 2; ch < plugBuf.getNumChannels(); ++ch)
        {
            const auto destCh = ch % juce::jmax(1, buffer.getNumChannels());
            buffer.addFrom(destCh, 0, plugBuf, ch, 0, numSamples);
        }

        for (int ch = plugBuf.getNumChannels(); ch < buffer.getNumChannels(); ++ch)
            buffer.clear(ch, 0, numSamples);

        return;
    }

    for (int ch = 0; ch < juce::jmin(2, plugBuf.getNumChannels()); ++ch)
        buffer.copyFrom(ch, 0, plugBuf, juce::jmin(ch, plugBuf.getNumChannels() - 1), 0, numSamples);
}

static juce::MidiBuffer withDrumChannelDuplicate(const juce::MidiBuffer& midi)
{
    static const int drumPadNotes[] = { 35, 36, 38, 40, 41, 42, 45, 46, 48, 49, 51 };
    juce::MidiBuffer result;

    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();

        if (message.isNoteOnOrOff())
        {
            auto mappedMessage = message;
            const auto padIndex = juce::jlimit(0, (int) std::size(drumPadNotes) - 1, std::abs(message.getNoteNumber() - 60) % (int) std::size(drumPadNotes));
            mappedMessage.setNoteNumber(drumPadNotes[padIndex]);
            mappedMessage.setChannel(1);
            result.addEvent(mappedMessage, metadata.samplePosition);
        }
        else
        {
            result.addEvent(message, metadata.samplePosition);
        }
    }

    return result;
}

bool PluginHostManager::loadTrackPlugin(const juce::String& name, int trackIndex, int slotIndex)
{
    auto hp = makeHostedPlugin(name);
    if (! hp) return false;

    auto key = SlotKey { trackIndex, slotIndex };
    if (auto* existing = getHostedTrackPlugin(trackIndex, slotIndex))
        releaseHostedPlugin(*existing);

    hostedTrackPlugins[key] = std::move(*hp);
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
    releaseHostedPlugin(it->second);
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
            hp->pluginName + " - Track " + juce::String(trackIndex + 1), std::move(ed), isBuiltInPluginName (hp->pluginName));
    }
    hp->editorWindow->setVisible(true);
    hp->editorWindow->toFront(true);
    return true;
}

bool PluginHostManager::loadTrackInstrumentPlugin(const juce::String& name, int trackIndex)
{
    auto hp = makeHostedPlugin(name);
    if (! hp) return false;

    if (auto* existing = getHostedTrackInstrumentPlugin(trackIndex))
        releaseHostedPlugin(*existing);

    hostedTrackInstrumentPlugins[trackIndex] = std::move(*hp);
    return true;
}

juce::String PluginHostManager::getTrackInstrumentPluginName(int trackIndex) const
{
    auto* hp = getHostedTrackInstrumentPlugin(trackIndex);
    return hp != nullptr ? hp->pluginName : juce::String();
}

bool PluginHostManager::showTrackInstrumentPluginEditor(int trackIndex)
{
    auto* hp = getHostedTrackInstrumentPlugin(trackIndex);
    if (! hp || ! hp->processor || ! hp->processor->hasEditor()) return false;

    if (! hp->editorWindow)
    {
        auto ed = std::unique_ptr<juce::AudioProcessorEditor>(hp->processor->createEditor());
        if (! ed) return false;
        hp->editorWindow = std::make_unique<PluginEditorWindow>(hp->pluginName + " - Track " + juce::String(trackIndex + 1),
                                                                std::move(ed),
                                                                isBuiltInPluginName (hp->pluginName));
    }

    hp->editorWindow->setVisible(true);
    hp->editorWindow->toFront(true);
    return true;
}

bool PluginHostManager::renderTrackInstrument(int trackIndex,
                                              juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midi,
                                              double)
{
    auto* hp = getHostedTrackInstrumentPlugin(trackIndex);
    if (! hp || ! hp->processor || hp->bypassed) return false;

    juce::MidiBuffer drumMidi;
    auto* midiToRender = &midi;

    if (hp->pluginName.containsIgnoreCase("drum"))
    {
        drumMidi = withDrumChannelDuplicate(midi);
        midiToRender = &drumMidi;
    }

    buffer.clear();
    runPluginOnBuffer(*hp, buffer, *midiToRender, true);

    return true;
}

bool PluginHostManager::preloadInstrumentPlugin(const juce::String& pluginName)
{
    if (pluginName.isEmpty() || hostedPatternInstrumentPlugins.find(pluginName) != hostedPatternInstrumentPlugins.end())
        return true;

    auto hp = makeHostedPlugin(pluginName);
    if (! hp) return false;

    hostedPatternInstrumentPlugins.emplace(pluginName, std::move(*hp));
    return true;
}

bool PluginHostManager::renderInstrumentPlugin(const juce::String& pluginName,
                                               juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& midi,
                                               double)
{
    if (pluginName.isEmpty())
        return false;

    auto it = hostedPatternInstrumentPlugins.find(pluginName);
    if (it == hostedPatternInstrumentPlugins.end())
        return false;

    auto& hp = it->second;
    if (! hp.processor || hp.bypassed) return false;

    juce::MidiBuffer drumMidi;
    auto* midiToRender = &midi;

    if (pluginName.containsIgnoreCase("drum"))
    {
        drumMidi = withDrumChannelDuplicate(midi);
        midiToRender = &drumMidi;
    }

    buffer.clear();
    runPluginOnBuffer(hp, buffer, *midiToRender, true);

    return true;
}

void PluginHostManager::removeTrack(int trackIndex)
{
    std::map<SlotKey, HostedPlugin> updated;
    for (auto& entry : hostedTrackPlugins)
    {
        if (entry.first.trackIndex == trackIndex)
        {
            releaseHostedPlugin(entry.second);
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
            releaseHostedPlugin(entry.second);
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

    if (auto* existing = getHostedMasterPlugin(slotIndex))
        releaseHostedPlugin(*existing);

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
    releaseHostedPlugin(it->second);
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
        hp->editorWindow = std::make_unique<PluginEditorWindow>(hp->pluginName + " - Master",
                                                                std::move(ed),
                                                                isBuiltInPluginName (hp->pluginName));
    }
    hp->editorWindow->setVisible(true);
    hp->editorWindow->toFront(true);
    return true;
}

void PluginHostManager::setBuiltInPluginTheme (juce::Colour accentColour, juce::Image bodySpiceImage)
{
    BuiltInPluginTheme::setAccentColour (accentColour);
    BuiltInPluginTheme::setBodySpiceImage (std::move (bodySpiceImage));

    const auto refreshHosted = [] (auto& plugins)
    {
        for (auto& entry : plugins)
            if (entry.second.editorWindow != nullptr)
                entry.second.editorWindow->refreshTheme();
    };

    refreshHosted (hostedTrackPlugins);
    refreshHosted (hostedTrackInstrumentPlugins);
    refreshHosted (hostedMasterPlugins);
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
    return getTrackInstrumentPluginName(-1);
}

bool PluginHostManager::showPianoRollInstrumentEditor()
{
    return showTrackInstrumentPluginEditor(-1);
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

bool PluginHostManager::isBuiltInPluginName (const juce::String& pluginName)
{
    const auto names = getBuiltInPluginNames (PluginRole::effect);
    return names.contains (pluginName);
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

    juce::Array<juce::File> pluginFiles;
    dir.findChildFiles(pluginFiles, juce::File::findFiles, true, "*.vst3");

   #if JUCE_PLUGINHOST_VST
    dir.findChildFiles(pluginFiles, juce::File::findFiles, true, "*.dll");
   #endif

    for (auto& file : pluginFiles)
        for (auto* fmt : formatManager.getFormats())
        {
            if (! fmt || ! fmt->fileMightContainThisPluginType(file.getFullPathName())) continue;

            const bool isInInstPluginFolder = file.getFullPathName().containsIgnoreCase("InstPlugin");

            juce::OwnedArray<juce::PluginDescription> descs;
            fmt->findAllTypesForFile(descs, file.getFullPathName());
            for (auto* d : descs)
                if (d)
                {
                    const auto menuName = makeMenuNameForPlugin(*d);
                    externalPlugins.push_back({ menuName,
                                                isInInstPluginFolder ? PluginRole::instrument : PluginRole::effect,
                                                *d });
                }
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
