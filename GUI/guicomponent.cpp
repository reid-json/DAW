#include "guicomponent.h"

#include <juce_core/juce_core.h>
#include <jive_core/algorithms/jive_Find.h>
#include "timelinecomponent.h"

namespace
{
    juce::File findResourcesDir()
    {
        auto dir = juce::File::getSpecialLocation (juce::File::currentExecutableFile).getParentDirectory();
        for (int i = 0; i < 4; ++i)
        {
            if (dir.getChildFile ("Resources").isDirectory())
                return dir.getChildFile ("Resources");
            dir = dir.getParentDirectory();
        }
        return dir.getChildFile ("Resources");
    }

    juce::var parseJsonFile (const juce::File& file)
    {
        return file.existsAsFile() ? juce::JSON::parse (file.loadFileAsString()) : juce::var {};
    }

    juce::DynamicObject* asObj (const juce::var& v)
    {
        return v.isObject() ? v.getDynamicObject() : nullptr;
    }

    void mergeProps (juce::DynamicObject& target, const juce::var& src)
    {
        if (auto* obj = asObj (src))
            for (auto& p : obj->getProperties())
                target.setProperty (p.name, p.value);
    }

    juce::var parseStyleString (const juce::String& text)
    {
        if (text.trim().isEmpty()) return juce::var (new juce::DynamicObject());
        auto parsed = juce::JSON::parse (text);
        return parsed.isObject() ? parsed : juce::var (new juce::DynamicObject());
    }

    juce::StringArray splitClasses (const juce::String& str)
    {
        juce::StringArray classes;
        classes.addTokens (str, " ", "\"");
        classes.removeEmptyStrings();
        classes.trim();
        return classes;
    }

    juce::String buildStyleForClasses (const juce::String& classNames, const juce::var& stylesheet)
    {
        auto merged = juce::var (new juce::DynamicObject());
        if (auto* sheet = asObj (stylesheet))
            for (auto& cls : splitClasses (classNames))
                mergeProps (*merged.getDynamicObject(), sheet->getProperty ("." + cls));
        return juce::JSON::toString (merged, false);
    }

    void applyStyles (juce::ValueTree node, const juce::var& stylesheet)
    {
        auto* sheet = asObj (stylesheet);
        if (sheet == nullptr) return;

        auto merged = juce::var (new juce::DynamicObject());
        auto* obj = merged.getDynamicObject();

        for (auto& cls : splitClasses (node.getProperty ("class").toString()))
            mergeProps (*obj, sheet->getProperty ("." + cls));

        auto id = node.getProperty ("id").toString();
        if (id.isNotEmpty())
            mergeProps (*obj, sheet->getProperty ("#" + id));

        mergeProps (*obj, parseStyleString (node.getProperty ("style").toString()));
        node.setProperty ("style", juce::JSON::toString (merged, false), nullptr);

        for (int i = 0; i < node.getNumChildren(); ++i)
            applyStyles (node.getChild (i), stylesheet);
    }

    using SpriteAssetMap = std::map<juce::String, juce::var>;

    juce::String normaliseKey (juce::String text)
    {
        text = text.toLowerCase().trim();
        juce::String result;
        for (auto c : text)
            if (juce::CharacterFunctions::isLetterOrDigit (c))
                result << c;
        return result;
    }

    SpriteAssetMap loadSprites (const juce::File& dir)
    {
        SpriteAssetMap sprites;
        if (! dir.isDirectory()) return sprites;

        for (auto& file : dir.findChildFiles (juce::File::findFiles, false, "*"))
        {
            auto key = normaliseKey (file.getFileNameWithoutExtension());
            if (key.isEmpty()) continue;

            auto ext = file.getFileExtension().toLowerCase();
            if (ext == ".svg")
            {
                auto svg = file.loadFileAsString();
                if (svg.isNotEmpty()) sprites[key] = svg;
            }
            else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg")
            {
                if (auto img = juce::ImageFileFormat::loadFrom (file); img.isValid())
                    sprites[key] = juce::VariantConverter<juce::Image>::toVar (img);
            }
        }
        return sprites;
    }

    const juce::var* findSprite (const juce::StringArray& keys, const SpriteAssetMap& sprites)
    {
        // Exact match first
        for (auto& key : keys)
            if (auto it = sprites.find (key); it != sprites.end())
                return &it->second;
        // Prefix match fallback
        for (auto& key : keys)
            for (auto& [sk, sv] : sprites)
                if (sk.startsWith (key) || key.startsWith (sk))
                    return &sv;
        return nullptr;
    }

    juce::StringArray buildSpriteKeys (const juce::ValueTree& btn)
    {
        juce::StringArray keys;
        auto id = btn.getProperty ("id").toString();
        keys.addIfNotAlreadyThere (normaliseKey (id));
        if (id.endsWithIgnoreCase ("Btn"))
            keys.addIfNotAlreadyThere (normaliseKey (id.dropLastCharacters (3)));

        for (int i = 0; i < btn.getNumChildren(); ++i)
        {
            auto child = btn.getChild (i);
            if (child.getType().toString().compareIgnoreCase ("Text") == 0)
                keys.addIfNotAlreadyThere (normaliseKey (child.getProperty ("text").toString()));
        }
        return keys;
    }

    void replaceLabelsWithSprites (juce::ValueTree node, const SpriteAssetMap& sprites)
    {
        if (! node.isValid()) return;

        if (node.getType().toString().compareIgnoreCase ("Button") == 0)
        {
            auto* sprite = findSprite (buildSpriteKeys (node), sprites);
            if (sprite != nullptr && ! sprite->isVoid())
            {
                // Remove text children, capture label
                juce::String label;
                for (int i = node.getNumChildren(); --i >= 0;)
                {
                    auto child = node.getChild (i);
                    if (child.getType().toString().compareIgnoreCase ("Text") != 0) continue;
                    if (label.isEmpty()) label = child.getProperty ("text").toString();
                    node.removeChild (i, nullptr);
                }

                if (label.isNotEmpty())
                {
                    if (node.getProperty ("title").toString().isEmpty())
                        node.setProperty ("title", label, nullptr);
                    if (node.getProperty ("tooltip").toString().isEmpty())
                        node.setProperty ("tooltip", label, nullptr);
                }

                // Add btn-icon class
                auto cls = node.getProperty ("class").toString().trim();
                if (! cls.containsWholeWord ("btn-icon"))
                    node.setProperty ("class", (cls.isEmpty() ? "" : cls + " ") + "btn-icon", nullptr);

                node.setProperty ("width", 48, nullptr);
                node.setProperty ("height", 48, nullptr);
                node.setProperty ("padding", 0, nullptr);
                node.setProperty ("justify-content", "center", nullptr);
                node.setProperty ("align-items", "center", nullptr);

                juce::ValueTree img ("Image");
                img.setProperty ("id", node.getProperty ("id").toString() + "Sprite", nullptr);
                img.setProperty ("source", *sprite, nullptr);
                img.setProperty ("width", 40, nullptr);
                img.setProperty ("height", 40, nullptr);
                img.setProperty ("placement", "centred", nullptr);
                img.setProperty ("intercepts-mouse-clicks", false, nullptr);
                node.appendChild (img, nullptr);
            }
        }

        for (int i = 0; i < node.getNumChildren(); ++i)
            replaceLabelsWithSprites (node.getChild (i), sprites);
    }

    void enableClickThrough (juce::Component& comp)
    {
        for (int i = 0; i < comp.getNumChildComponents(); ++i)
            if (auto* child = comp.getChildComponent (i))
            {
                child->setInterceptsMouseClicks (false, false);
                enableClickThrough (*child);
            }
    }
}

void GUIComponent::registerCustomComponentTypes()
{
    viewInterpreter.getComponentFactory().set ("TrackListView", [this]
    {
        auto component = std::make_unique<TrackListComponent> (state);
        component->onTrackSelected = [this] (int)
        {
            if (arrangementComponent != nullptr)
                arrangementComponent->repaint();
        };
        component->onMixerFocusChanged = [this]
        {
            if (arrangementComponent != nullptr)
                arrangementComponent->repaint();
        };
        component->onAddTrackRequested = [this]
        {
            state.addTrack();
            refreshFromState();
        };
        component->onRemoveTrackRequested = [this] (int trackIndex)
        {
            juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
            pluginHostManager.removeTrack(trackIndex);
            state.removeTrackAt (trackIndex);
            refreshFromState();
        };
        component->onGetAvailablePlugins = [this] (bool isMaster)
        {
            return isMaster ? pluginHostManager.getAvailableMasterPlugins()
                            : pluginHostManager.getAvailableTrackPlugins();
        };
        component->onGetPluginName = [this] (bool isMaster, int trackIndex, int slotIndex) -> juce::String
        {
            return isMaster ? pluginHostManager.getMasterPluginName(slotIndex)
                            : pluginHostManager.getTrackPluginName(trackIndex, slotIndex);
        };
        component->onLoadPlugin = [this] (bool isMaster, int trackIndex, int slotIndex, const juce::String& name)
        {
            juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
            if (isMaster)
            {
                pluginHostManager.loadMasterPlugin(name, slotIndex);
                state.loadMasterPluginIntoFxSlot(slotIndex, name);
            }
            else
            {
                pluginHostManager.loadTrackPlugin(name, trackIndex, slotIndex);
                state.loadTrackPluginIntoFxSlot(trackIndex, slotIndex, name);
            }
        };
        component->onRemovePlugin = [this] (bool isMaster, int trackIndex, int slotIndex)
        {
            juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
            if (isMaster)
            {
                pluginHostManager.removeMasterPlugin(slotIndex);
                state.clearMasterFxSlot(slotIndex);
            }
            else
            {
                pluginHostManager.removeTrackPlugin(trackIndex, slotIndex);
                state.clearTrackFxSlot(trackIndex, slotIndex);
            }
        };
        component->onShowPluginEditor = [this] (bool isMaster, int trackIndex, int slotIndex)
        {
            if (isMaster)
                pluginHostManager.showMasterPluginEditor(slotIndex);
            else
                pluginHostManager.showTrackPluginEditor(trackIndex, slotIndex);
        };
        component->onGetAvailableInputs = [this]
        {
            return getAvailableTrackInputs();
        };
        trackListComponent = component.get();
        return component;
    });

    viewInterpreter.getComponentFactory().set ("TimelineView", [this]
    {
        auto component = std::make_unique<TimelineComponent> (state);
        timelineComponent = component.get();
        return component;
    });

    viewInterpreter.getComponentFactory().set ("ArrangementView", [this]
    {
        auto component = std::make_unique<ArrangementComponent> (state);

        component->onRecentClipDropped = [this] (int assetId, int trackIndex, double startSeconds)
        {
            if (onRecentClipDropped)
                onRecentClipDropped (assetId, trackIndex, startSeconds);
        };

        component->onTimelineClipMoved = [this] (int placementId, int trackIndex, double startSeconds)
        {
            if (onTimelineClipMoved)
                onTimelineClipMoved (placementId, trackIndex, startSeconds);
        };

        component->onTimelineClipDeleteRequested = [this] (int placementId)
        {
            if (onTimelineClipDeleteRequested)
            {
                onTimelineClipDeleteRequested (placementId);
                return;
            }

            auto it = std::remove_if (state.timelineClips.begin(), state.timelineClips.end(),
                                      [placementId] (const TimelineClipItem& clip) { return clip.placementId == placementId; });
            state.timelineClips.erase (it, state.timelineClips.end());
            repaintDynamicViews();
        };

        arrangementComponent = component.get();
        return component;
    });

    viewInterpreter.getComponentFactory().set ("RecentClipsView", [this]
    {
        auto component = std::make_unique<RecentClipsComponent> (state);
        component->onAssetRenameRequested = [this] (int assetId, const juce::String& newName)
        {
            if (onAssetRenameRequested)
                onAssetRenameRequested (assetId, newName);
        };
        recentClipsComponent = component.get();
        return component;
    });

}

GUIComponent::GUIComponent(juce::AudioDeviceManager& sharedDeviceManager)
    : deviceManager(sharedDeviceManager)
{
    setLookAndFeel (&lookAndFeel);

    const auto resourcesDir = findResourcesDir();
    const auto spriteDir = resourcesDir.getChildFile ("ui")
                                       .getChildFile ("sprites");

    const auto xmlFile = resourcesDir.getChildFile ("ui")
                                     .getChildFile ("views")
                                     .getChildFile ("main_view.xml");

    const auto jsonStyleFile = resourcesDir.getChildFile ("ui")
                                           .getChildFile ("styles")
                                           .getChildFile ("styles.json");

    if (xmlFile.existsAsFile())
        if (auto xml = juce::XmlDocument (xmlFile).getDocumentElement())
            uiTree = juce::ValueTree::fromXml (*xml);

    if (! uiTree.isValid())
    {
        uiTree = juce::ValueTree ("Component");
        uiTree.setProperty ("id", "root", nullptr);
    }

    spriteAssets = loadSprites (spriteDir);
    replaceLabelsWithSprites (uiTree, spriteAssets);

    stylesheet = parseJsonFile (jsonStyleFile);
    if (stylesheet.isObject())
        applyStyles (uiTree, stylesheet);

    registerCustomComponentTypes();
    root = viewInterpreter.interpret (uiTree);

    if (root != nullptr)
    {
        if (auto comp = root->getComponent())
            addAndMakeVisible (comp.get());

        viewInterpreter.listenTo (*root);
        installCallbacks();
    }

    refreshFromState();

    setSize (1440, 700);
    resized();
}

GUIComponent::~GUIComponent()
{
    setLookAndFeel (nullptr);
}

void GUIComponent::resized()
{
    if (root == nullptr)
        return;

    if (auto rootNode = jive::findElementWithID (uiTree, juce::Identifier ("root")); rootNode.isValid())
    {
        rootNode.setProperty ("width",  getWidth(),  nullptr);
        rootNode.setProperty ("height", getHeight(), nullptr);
    }

    if (auto comp = root->getComponent())
        comp->setBounds (getLocalBounds());

    root->callLayoutChildrenWithRecursionLock();
    applyManualBodyLayout();
}

jive::GuiItem* GUIComponent::findGuiItemById (jive::GuiItem& node, const juce::Identifier& id)
{
    if (node.state.getProperty ("id").toString() == id.toString())
        return &node;

    for (auto* child : node.getChildren())
        if (child != nullptr)
            if (auto* found = findGuiItemById (*child, id))
                return found;

    return nullptr;
}

void GUIComponent::applyManualBodyLayout()
{
    if (root == nullptr) return;

    // Helper to set bounds on a named element
    auto setBoundsById = [this] (const juce::String& id, juce::Rectangle<int> bounds) -> bool
    {
        if (auto* item = findGuiItemById (*root, juce::Identifier (id)))
            if (auto comp = item->getComponent())
            {
                comp->setBounds (bounds);
                return true;
            }
        return false;
    };

    auto getComp = [this] (const juce::String& id) -> juce::Component*
    {
        if (auto* item = findGuiItemById (*root, juce::Identifier (id)))
            if (auto comp = item->getComponent())
                return comp.get();
        return nullptr;
    };

    auto* headerComp    = getComp ("headerContent");
    auto* bodyComp      = getComp ("bodyContent");
    auto* workspaceComp = getComp ("workspaceContent");
    auto* sidebarComp   = getComp ("clipSidebarContent");
    if (! headerComp || ! bodyComp || ! workspaceComp || ! sidebarComp) return;

    constexpr int stripW = 210, masterH = 100;

    auto area = getLocalBounds();
    headerComp->setBounds (area.removeFromTop (98));
    bodyComp->setBounds (area);

    setBoundsById ("headerMainRow", headerComp->getLocalBounds().reduced (10).removeFromTop (52));

    int sideW = state.clipSidebarCollapsed ? 44 : state.clipSidebarWidth;
    auto bodyArea = bodyComp->getLocalBounds();
    sidebarComp->setBounds (bodyArea.removeFromRight (sideW));
    workspaceComp->setBounds (bodyArea);

    auto wsArea = workspaceComp->getLocalBounds();
    setBoundsById ("trackList",        wsArea.removeFromLeft (stripW));
    setBoundsById ("timelinePanel",    wsArea.removeFromTop (masterH));
    setBoundsById ("arrangementPanel", wsArea);

    auto clipArea = sidebarComp->getLocalBounds().reduced (10);
    setBoundsById ("clipBrowserTitle", clipArea.removeFromTop (24));
    clipArea.removeFromTop (4);
    setBoundsById ("clipList", clipArea);
}

void GUIComponent::installCallbacks()
{
    auto bindBtn = [this] (const juce::String& id, std::function<void()> fn)
    {
        if (root == nullptr) return;
        if (auto* item = findGuiItemById (*root, juce::Identifier (id)))
            if (auto comp = item->getComponent())
                if (auto* btn = dynamic_cast<juce::Button*> (comp.get()))
                {
                    enableClickThrough (*btn);
                    btn->onClick = std::move (fn);
                }
    };

    bindBtn ("playBtn",    [this] { if (onPlayRequested)    onPlayRequested(); });
    bindBtn ("pauseBtn",   [this] { if (onPauseRequested)   onPauseRequested(); });
    bindBtn ("restartBtn", [this] { if (onRestartRequested)  onRestartRequested(); });

    bindBtn ("recordBtn", [this]
    {
        if (state.isRecording) { if (onStopRequested) onStopRequested(); return; }
        if (onRecordToggleRequested) onRecordToggleRequested();
    });

    bindBtn ("monitorBtn", [this]
    {
        if (onMonitoringToggleRequested) { onMonitoringToggleRequested(); return; }
        state.toggleAudioMonitoring();
        refreshFromState();
    });

    bindBtn ("settingsBtn", [this] { openSettingsWindow(); });

    bindBtn ("toggleClipSidebarBtn", [this] { state.toggleClipSidebar(); refreshFromState(); });

    bindBtn ("pianoRollBtn", [this]
    {
        if (pianoRollWindow == nullptr)
        {
            pianoRollWindow = std::make_unique<PianoRollWindow>();
            pianoRollWindow->setOnSavePattern ([this] (const std::vector<PianoRoll::Note>& notes)
            {
                if (onSavePatternRequested)
                    onSavePatternRequested (notes);
            });
            pianoRollWindow->setOnGetAvailableInstruments ([this]
            {
                return pluginHostManager.getAvailableTrackInstrumentPlugins();
            });
            pianoRollWindow->setOnInstrumentChanged ([this] (const juce::String& name)
            {
                if (onPianoRollInstrumentChangeRequested)
                    onPianoRollInstrumentChangeRequested (name);
            });

            auto currentInst = pluginHostManager.getPianoRollInstrumentName();
            if (currentInst.isNotEmpty())
                pianoRollWindow->setInstrumentName (currentInst);
        }
        pianoRollWindow->setVisible (true);
        pianoRollWindow->toFront (true);
    });

    bindBtn ("fileBtn", [this]
    {
        juce::PopupMenu menu;
        menu.addItem (1, "Import Audio File...");
        menu.addItem (2, "Open Project...");
        menu.addSeparator();
        menu.addItem (3, "Save Project");
        menu.addSeparator();
        menu.addItem (4, "Export as WAV...");

        menu.showMenuAsync (juce::PopupMenu::Options(), [this] (int result)
        {
            if (result == 1 && onImportAudioRequested)    onImportAudioRequested();
            else if (result == 2 && onOpenProjectRequested)    onOpenProjectRequested();
            else if (result == 3 && onSaveProjectRequested)    onSaveProjectRequested();
            else if (result == 4 && onExportWavRequested)      onExportWavRequested();
        });
    });
}

void GUIComponent::openSettingsWindow()
{
    if (settingsWindow == nullptr)
        settingsWindow = std::make_unique<SettingsWindow> (deviceManager);

    settingsWindow->setVisible (true);
    settingsWindow->toFront (true);
}

juce::StringArray GUIComponent::getAvailableTrackInputs() const
{
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup(setup);
    juce::StringArray inputs;

    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        const auto inputNames = device->getInputChannelNames();
        for (int i = 0; i < inputNames.size(); ++i)
            if (setup.inputChannels[i])
                inputs.add(inputNames[i]);

        if (inputs.isEmpty())
            inputs = inputNames;
    }

    inputs.removeDuplicates(false);
    return inputs;
}

void GUIComponent::refreshFromState()
{
    if (! uiTree.isValid())
        return;

    if (root == nullptr)
        return;

    auto setTextById = [this] (const juce::String& id, const juce::String& text)
    {
        auto node = jive::findElementWithID (uiTree, juce::Identifier (id));
        if (node.isValid())
            node.setProperty ("text", text, nullptr);
    };

    auto setClassById = [this] (const juce::String& id, const juce::String& className)
    {
        auto node = jive::findElementWithID (uiTree, juce::Identifier (id));
        if (node.isValid())
        {
            node.setProperty ("class", className, nullptr);
            node.setProperty ("style", buildStyleForClasses (className, stylesheet), nullptr);
        }
    };

    auto setPropertyById = [this] (const juce::String& id, const juce::Identifier& prop, const juce::var& value)
    {
        auto node = jive::findElementWithID (uiTree, juce::Identifier (id));
        if (node.isValid())
            node.setProperty (prop, value, nullptr);
    };

    setTextById ("playBtnLabel",
                 state.transportState == DAWState::TransportState::playing ? "Playing" : "Play");

    setTextById ("pauseBtnLabel",
                 state.transportState == DAWState::TransportState::paused ? "Resume" : "Pause");

    setTextById ("recordBtnLabel",
                 state.isRecording ? "Stop" : "Record");

    setClassById ("recordBtn",
                  state.isRecording ? "btn btn-record-active" : "btn btn-record");

    if (auto* recordSprite = findSprite (juce::StringArray{ state.isRecording ? "stop" : "record", "recordbtn" }, spriteAssets))
        setPropertyById ("recordBtnSprite", juce::Identifier ("source"), *recordSprite);
    
    setTextById ("monitorBtnLabel",
             state.audioMonitoringEnabled ? "Monitor On" : "Monitor Off");

    setClassById ("monitorBtn",
              state.audioMonitoringEnabled ? "btn btn-monitor-on"
                                           : "btn btn-monitor-off");

    auto clipSidebarNode = jive::findElementWithID (uiTree, juce::Identifier ("clipSidebarContent"));
    if (clipSidebarNode.isValid())
    {
        const int collapsedWidth = 44;
        clipSidebarNode.setProperty ("width", state.clipSidebarCollapsed ? collapsedWidth : state.clipSidebarWidth, nullptr);
        clipSidebarNode.setProperty ("min-width", state.clipSidebarCollapsed ? collapsedWidth : 160, nullptr);
        clipSidebarNode.setProperty ("max-width", state.clipSidebarCollapsed ? collapsedWidth : 320, nullptr);
    }

    setPropertyById ("clipBrowserTitle", juce::Identifier ("display"),
                     state.clipSidebarCollapsed ? "none" : "flex");

    setPropertyById ("clipList", juce::Identifier ("display"),
                     state.clipSidebarCollapsed ? "none" : "flex");

    setPropertyById ("clipSidebarSpacer", juce::Identifier ("display"),
                     state.clipSidebarCollapsed ? "none" : "flex");

    root->callLayoutChildrenWithRecursionLock();
    repaint();
}

void GUIComponent::refreshExternalState(bool shouldRefreshControls, bool /*shouldRebuildTrackList*/)
{
    if (shouldRefreshControls)
        refreshFromState();

    followTimelinePlayhead();
    repaintDynamicViews();
}

void GUIComponent::repaintDynamicViews()
{
    if (timelineComponent != nullptr)
        timelineComponent->repaint();

    if (arrangementComponent != nullptr)
        arrangementComponent->repaint();

    if (recentClipsComponent != nullptr)
        recentClipsComponent->repaint();

    if (trackListComponent != nullptr)
        trackListComponent->repaint();
}

void GUIComponent::followTimelinePlayhead()
{
    if (timelineComponent == nullptr && arrangementComponent == nullptr)
        return;

    if (state.isDraggingHorizontalScrollbar)
        return;

    if (state.transportState != DAWState::TransportState::playing)
        return;

    int viewportWidth = 0;
    if (arrangementComponent != nullptr)
        viewportWidth = arrangementComponent->getWidth() - 10;
    else if (timelineComponent != nullptr)
        viewportWidth = timelineComponent->getWidth();

    if (viewportWidth <= 0)
        return;

    constexpr float pixelsPerSecond = 100.0f;
    constexpr float leftInset = 40.0f;
    constexpr int followPadding = 120;
    const int playheadX = static_cast<int>(std::round(leftInset + state.playhead * pixelsPerSecond));
    const int visibleLeft = state.horizontalScrollOffset;
    const int visibleRight = state.horizontalScrollOffset + viewportWidth;

    if (playheadX > visibleRight - followPadding)
        state.horizontalScrollOffset = juce::jmax(0, playheadX - viewportWidth + followPadding);
    else if (playheadX < visibleLeft + 40)
        state.horizontalScrollOffset = juce::jmax(0, playheadX - 40);
}

