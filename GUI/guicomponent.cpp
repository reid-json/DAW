#include "guicomponent.h"

#include <map>

#include <juce_core/juce_core.h>
#include <jive_core/algorithms/jive_Find.h>
#include "timelinecomponent.h"

namespace
{
    std::vector<PianoRoll::Note> toPianoRollNotes (const TrackPatternState& pattern)
    {
        std::vector<PianoRoll::Note> notes;
        notes.reserve (pattern.notes.size());

        for (const auto& note : pattern.notes)
            notes.push_back ({ note.id, note.startBeat, note.lengthBeats, note.midiNote, note.label });

        return notes;
    }

    std::vector<TrackPatternNote> toTrackPatternNotes (const std::vector<PianoRoll::Note>& notes)
    {
        std::vector<TrackPatternNote> patternNotes;
        patternNotes.reserve (notes.size());

        for (const auto& note : notes)
            patternNotes.push_back ({ note.id, note.startBeat, note.lengthBeats, note.midiNote, note.label });

        return patternNotes;
    }

    bool samePatternNotes (const std::vector<TrackPatternNote>& lhs, const std::vector<PianoRoll::Note>& rhs)
    {
        if (lhs.size() != rhs.size())
            return false;

        for (size_t i = 0; i < lhs.size(); ++i)
        {
            if (lhs[i].id != rhs[i].id
                || lhs[i].startBeat != rhs[i].startBeat
                || lhs[i].lengthBeats != rhs[i].lengthBeats
                || lhs[i].midiNote != rhs[i].midiNote
                || lhs[i].label != rhs[i].label)
                return false;
        }

        return true;
    }

    juce::File findResourcesDir()
    {
        auto exeFile = juce::File::getSpecialLocation (juce::File::currentExecutableFile);

        auto resourcesDir = exeFile.getSiblingFile ("Resources");
        if (! resourcesDir.exists())
            resourcesDir = exeFile.getParentDirectory();
        if (! resourcesDir.getChildFile ("Resources").exists())
            resourcesDir = resourcesDir.getParentDirectory();
        if (! resourcesDir.getChildFile ("Resources").exists())
            resourcesDir = resourcesDir.getParentDirectory();

        return resourcesDir.getChildFile ("Resources");
    }

    juce::var parseJsonFile (const juce::File& file)
    {
        if (! file.existsAsFile())
            return {};

        return juce::JSON::parse (file.loadFileAsString());
    }

    juce::DynamicObject* asObject (const juce::var& v)
    {
        return v.isObject() ? v.getDynamicObject() : nullptr;
    }

    juce::StringArray getClasses (const juce::ValueTree& node)
    {
        juce::StringArray classes;
        classes.addTokens (node.getProperty ("class").toString(), " ", "\"");
        classes.removeEmptyStrings();
        classes.trim();
        return classes;
    }

    void mergeObjectIntoStyle (juce::DynamicObject& target, const juce::var& sourceVar)
    {
        if (auto* source = asObject (sourceVar))
        {
            for (const auto& property : source->getProperties())
                target.setProperty (property.name, property.value);
        }
    }

    juce::var parseStyleString (const juce::String& styleText)
    {
        if (styleText.trim().isEmpty())
            return juce::var (new juce::DynamicObject());

        auto parsed = juce::JSON::parse (styleText);
        if (! parsed.isObject())
            return juce::var (new juce::DynamicObject());

        return parsed;
    }

    juce::String makeStyleStringForClasses (const juce::String& classNames, const juce::var& stylesheet)
    {
        auto mergedStyle = juce::var (new juce::DynamicObject());
        auto* mergedObject = mergedStyle.getDynamicObject();

        juce::StringArray classes;
        classes.addTokens (classNames, " ", "\"");
        classes.removeEmptyStrings();
        classes.trim();

        if (auto* sheet = asObject (stylesheet))
        {
            for (const auto& className : classes)
                mergeObjectIntoStyle (*mergedObject, sheet->getProperty ("." + className));
        }

        return juce::JSON::toString (mergedStyle, false);
    }

    void applyStylesRecursively (juce::ValueTree node, const juce::var& stylesheet)
    {
        auto* sheet = asObject (stylesheet);
        if (sheet == nullptr)
            return;

        auto mergedStyle = juce::var (new juce::DynamicObject());
        auto* mergedObject = mergedStyle.getDynamicObject();

        for (const auto& className : getClasses (node))
            mergeObjectIntoStyle (*mergedObject, sheet->getProperty ("." + className));

        const auto id = node.getProperty ("id").toString();
        if (id.isNotEmpty())
            mergeObjectIntoStyle (*mergedObject, sheet->getProperty ("#" + id));

        mergeObjectIntoStyle (*mergedObject,
                              parseStyleString (node.getProperty ("style").toString()));

        node.setProperty ("style", juce::JSON::toString (mergedStyle, false), nullptr);

        for (int i = 0; i < node.getNumChildren(); ++i)
            applyStylesRecursively (node.getChild (i), stylesheet);
    }

    using SpriteAssetMap = std::map<juce::String, juce::var>;

    juce::String normaliseSpriteKey (juce::String text)
    {
        text = text.toLowerCase().trim();
        juce::String result;

        for (const auto character : text)
        {
            if (juce::CharacterFunctions::isLetterOrDigit (character))
                result << character;
        }

        return result;
    }

    SpriteAssetMap loadSpriteAssets (const juce::File& spritesDirectory)
    {
        SpriteAssetMap sprites;

        if (! spritesDirectory.isDirectory())
            return sprites;

        for (const auto& spriteFile : spritesDirectory.findChildFiles (juce::File::findFiles, false, "*"))
        {
            const auto key = normaliseSpriteKey (spriteFile.getFileNameWithoutExtension());
            if (key.isEmpty())
                continue;

            const auto extension = spriteFile.getFileExtension().toLowerCase();

            if (extension == ".svg")
            {
                const auto svgSource = spriteFile.loadFileAsString();
                if (svgSource.isNotEmpty())
                    sprites[key] = svgSource;

                continue;
            }

            if (extension == ".png" || extension == ".jpg" || extension == ".jpeg")
            {
                if (auto image = juce::ImageFileFormat::loadFrom (spriteFile); image.isValid())
                    sprites[key] = juce::VariantConverter<juce::Image>::toVar (image);
            }
        }

        return sprites;
    }

    juce::StringArray buildSpriteLookupKeys (const juce::ValueTree& button)
    {
        juce::StringArray keys;
        keys.addIfNotAlreadyThere (normaliseSpriteKey (button.getProperty ("id").toString()));

        auto buttonId = button.getProperty ("id").toString();
        if (buttonId.endsWithIgnoreCase ("Btn"))
            keys.addIfNotAlreadyThere (normaliseSpriteKey (buttonId.dropLastCharacters (3)));

        for (int i = 0; i < button.getNumChildren(); ++i)
        {
            const auto child = button.getChild (i);
            if (child.getType().toString().compareIgnoreCase ("Text") != 0)
                continue;

            const auto label = child.getProperty ("text").toString();
            keys.addIfNotAlreadyThere (normaliseSpriteKey (label));
        }

        return keys;
    }

    const juce::var* findMatchingSprite (const juce::StringArray& keys, const SpriteAssetMap& sprites)
    {
        for (const auto& key : keys)
        {
            if (const auto it = sprites.find (key); it != sprites.end())
                return &it->second;
        }

        for (const auto& key : keys)
        {
            for (const auto& [spriteKey, spriteValue] : sprites)
            {
                if (spriteKey.startsWith (key) || key.startsWith (spriteKey))
                    return &spriteValue;
            }
        }

        return nullptr;
    }

    void replaceButtonLabelsWithSprites (juce::ValueTree node, const SpriteAssetMap& sprites)
    {
        if (! node.isValid())
            return;

        if (node.getType().toString().compareIgnoreCase ("Button") == 0)
        {
            juce::var matchedSprite;
            if (const auto* sprite = findMatchingSprite (buildSpriteLookupKeys (node), sprites))
                matchedSprite = *sprite;

            if (! matchedSprite.isVoid())
            {
                juce::String label;

                for (int i = node.getNumChildren(); --i >= 0;)
                {
                    const auto child = node.getChild (i);
                    if (child.getType().toString().compareIgnoreCase ("Text") != 0)
                        continue;

                    if (label.isEmpty())
                        label = child.getProperty ("text").toString();

                    node.removeChild (i, nullptr);
                }

                if (label.isNotEmpty())
                {
                    if (node.getProperty ("title").toString().isEmpty())
                        node.setProperty ("title", label, nullptr);

                    if (node.getProperty ("tooltip").toString().isEmpty())
                        node.setProperty ("tooltip", label, nullptr);
                }

                auto classNames = node.getProperty ("class").toString().trim();
                if (! classNames.containsWholeWord ("btn-icon"))
                {
                    if (classNames.isNotEmpty())
                        classNames << " ";

                    classNames << "btn-icon";
                    node.setProperty ("class", classNames, nullptr);
                }

                node.setProperty ("width", 48, nullptr);
                node.setProperty ("height", 48, nullptr);
                node.setProperty ("padding", 0, nullptr);
                node.setProperty ("justify-content", "center", nullptr);
                node.setProperty ("align-items", "center", nullptr);

                juce::ValueTree spriteNode ("Image");
                spriteNode.setProperty ("id", node.getProperty ("id").toString() + "Sprite", nullptr);
                spriteNode.setProperty ("source", matchedSprite, nullptr);
                spriteNode.setProperty ("width", 40, nullptr);
                spriteNode.setProperty ("height", 40, nullptr);
                spriteNode.setProperty ("placement", "centred", nullptr);
                spriteNode.setProperty ("intercepts-mouse-clicks", false, nullptr);
                node.appendChild (spriteNode, nullptr);
            }
        }

        for (int i = 0; i < node.getNumChildren(); ++i)
            replaceButtonLabelsWithSprites (node.getChild (i), sprites);
    }

    void allowClicksToPassThroughChildren (juce::Component& component)
    {
        for (int i = 0; i < component.getNumChildComponents(); ++i)
        {
            if (auto* child = component.getChildComponent (i))
            {
                child->setInterceptsMouseClicks (false, false);
                allowClicksToPassThroughChildren (*child);
            }
        }
    }
}

void GUIComponent::registerCustomComponentTypes()
{
    viewInterpreter.getComponentFactory().set ("MixerMasterView", [this]
    {
        auto component = std::make_unique<MixerMasterComponent>(state);
        component->getAvailableMasterPlugins = [this]
        {
            return pluginHostManager.getAvailableMasterPlugins();
        };
        component->getAvailableMasterOutputs = [this]
        {
            return getAvailableMasterOutputs();
        };
        component->getMasterOutputDeviceName = [this]
        {
            return getMasterOutputDeviceName();
        };
        component->getAvailableTrackPlugins = [this]
        {
            return pluginHostManager.getAvailableTrackPlugins();
        };
        component->getAvailableTrackInstrumentPlugins = [this]
        {
            return pluginHostManager.getAvailableTrackInstrumentPlugins();
        };
        component->getAvailableTrackInputs = [this]
        {
            return getAvailableTrackInputs();
        };
        component->getAvailableTrackOutputs = [this]
        {
            return getAvailableTrackOutputs();
        };
        component->getTrackInputDeviceName = [this]
        {
            return getTrackInputDeviceName();
        };
        component->getTrackOutputDeviceName = [this]
        {
            return getTrackOutputDeviceName();
        };
        component->onMasterPluginLoadRequested = [this] (int slotIndex, const juce::String& pluginName)
        {
            juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
            if (auto* device = deviceManager.getCurrentAudioDevice())
                pluginHostManager.setProcessingConfig(device->getCurrentSampleRate(),
                                                      device->getCurrentBufferSizeSamples());
            return pluginHostManager.loadMasterPlugin(pluginName, slotIndex);
        };
        component->onMasterPluginBypassRequested = [this] (int slotIndex, bool shouldBeBypassed)
        {
            juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
            pluginHostManager.setMasterPluginBypassed(slotIndex, shouldBeBypassed);
        };
        component->onMasterPluginRemoveRequested = [this] (int slotIndex)
        {
            juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
            pluginHostManager.removeMasterPlugin(slotIndex);
        };
        component->onMasterPluginEditorRequested = [this] (int slotIndex)
        {
            pluginHostManager.showMasterPluginEditor(slotIndex);
        };
        component->onMasterPluginSlotRemoveRequested = [this] (int slotIndex)
        {
            juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
            pluginHostManager.removeMasterPluginSlot(slotIndex);
        };
        component->onTrackPluginLoadRequested = [this] (int trackIndex, int slotIndex, const juce::String& pluginName)
        {
            juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
            if (auto* device = deviceManager.getCurrentAudioDevice())
                pluginHostManager.setProcessingConfig(device->getCurrentSampleRate(),
                                                      device->getCurrentBufferSizeSamples());
            return pluginHostManager.loadTrackPlugin(pluginName, trackIndex, slotIndex);
        };
        component->onTrackPluginBypassRequested = [this] (int trackIndex, int slotIndex, bool shouldBeBypassed)
        {
            juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
            pluginHostManager.setTrackPluginBypassed(trackIndex, slotIndex, shouldBeBypassed);
        };
        component->onTrackPluginRemoveRequested = [this] (int trackIndex, int slotIndex)
        {
            juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
            pluginHostManager.removeTrackPlugin(trackIndex, slotIndex);
        };
        component->onTrackPluginEditorRequested = [this] (int trackIndex, int slotIndex)
        {
            pluginHostManager.showTrackPluginEditor(trackIndex, slotIndex);
        };
        component->onTrackPluginSlotRemoveRequested = [this] (int trackIndex, int slotIndex)
        {
            juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
            pluginHostManager.removeTrackPluginSlot(trackIndex, slotIndex);
        };
        component->onTrackInstrumentPluginLoadRequested = [this] (int trackIndex, const juce::String& pluginName)
        {
            juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
            if (auto* device = deviceManager.getCurrentAudioDevice())
                pluginHostManager.setProcessingConfig(device->getCurrentSampleRate(),
                                                      device->getCurrentBufferSizeSamples());
            return pluginHostManager.loadTrackInstrumentPlugin(pluginName, trackIndex);
        };
        component->onTrackInstrumentPluginBypassRequested = [this] (int trackIndex, bool shouldBeBypassed)
        {
            juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
            pluginHostManager.setTrackInstrumentPluginBypassed(trackIndex, shouldBeBypassed);
        };
        component->onTrackInstrumentPluginRemoveRequested = [this] (int trackIndex)
        {
            juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
            pluginHostManager.removeTrackInstrumentPlugin(trackIndex);
        };
        component->onTrackInstrumentPluginEditorRequested = [this] (int trackIndex)
        {
            pluginHostManager.showTrackInstrumentPluginEditor(trackIndex);
        };
        component->onTrackContentTypeChanged = [this] (int)
        {
            refreshFromState();
        };
        mixerMasterComponent = component.get();
        return component;
    });

    viewInterpreter.getComponentFactory().set ("TrackListView", [this]
    {
        auto component = std::make_unique<TrackListComponent> (state);
        component->onTrackSelected = [this] (int)
        {
            syncPianoRollWithSelectedTrack();
            if (arrangementComponent != nullptr)
                arrangementComponent->repaint();
        };
        component->onMixerFocusChanged = [this]
        {
            if (mixerMasterComponent != nullptr)
                mixerMasterComponent->repaint();
            if (arrangementComponent != nullptr)
                arrangementComponent->repaint();
        };
        component->onRemoveTrackRequested = [this] (int trackIndex)
        {
            juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
            pluginHostManager.removeTrack(trackIndex);
            state.removeTrackAt (trackIndex);
            refreshFromState();
        };
        component->getAvailableTrackPlugins = [this]
        {
            return pluginHostManager.getAvailableTrackPlugins();
        };
        component->getAvailableTrackInputs = [this]
        {
            return getAvailableTrackInputs();
        };
        component->getAvailableTrackOutputs = [this]
        {
            return getAvailableTrackOutputs();
        };
        component->getTrackInputDeviceName = [this]
        {
            return getTrackInputDeviceName();
        };
        component->getTrackOutputDeviceName = [this]
        {
            return getTrackOutputDeviceName();
        };
        component->onTrackPluginLoadRequested = [this] (int trackIndex, int slotIndex, const juce::String& pluginName)
        {
            juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
            return pluginHostManager.loadTrackPlugin(pluginName, trackIndex, slotIndex);
        };
        component->onTrackPluginBypassRequested = [this] (int trackIndex, int slotIndex, bool shouldBeBypassed)
        {
            juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
            pluginHostManager.setTrackPluginBypassed(trackIndex, slotIndex, shouldBeBypassed);
        };
        component->onTrackPluginRemoveRequested = [this] (int trackIndex, int slotIndex)
        {
            juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
            pluginHostManager.removeTrackPlugin(trackIndex, slotIndex);
        };
        component->onTrackPluginEditorRequested = [this] (int trackIndex, int slotIndex)
        {
            pluginHostManager.showTrackPluginEditor(trackIndex, slotIndex);
        };
        component->onTrackPluginSlotRemoveRequested = [this] (int trackIndex, int slotIndex)
        {
            juce::ScopedLock audioLock(deviceManager.getAudioCallbackLock());
            pluginHostManager.removeTrackPluginSlot(trackIndex, slotIndex);
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

    viewInterpreter.getComponentFactory().set ("TempoControlView", [this]
    {
        auto component = std::make_unique<TempoControlComponent> (state);
        tempoControlComponent = component.get();
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
    {
        juce::XmlDocument doc (xmlFile);

        if (auto xmlElement = doc.getDocumentElement())
        {
            uiTree = juce::ValueTree::fromXml (*xmlElement);
        }
        else
        {
            uiTree = juce::ValueTree ("Component");
            uiTree.setProperty ("id", "root", nullptr);

            auto label = juce::ValueTree ("Label");
            label.setProperty ("text", "Failed to parse XML", nullptr);
            uiTree.appendChild (label, nullptr);
        }
    }
    else
    {
        uiTree = juce::ValueTree ("Component");
        uiTree.setProperty ("id", "root", nullptr);

        auto label = juce::ValueTree ("Label");
        label.setProperty ("text", "Missing XML: " + xmlFile.getFullPathName(), nullptr);
        uiTree.appendChild (label, nullptr);
    }

    spriteAssets = loadSpriteAssets (spriteDir);
    replaceButtonLabelsWithSprites (uiTree, spriteAssets);

    stylesheet = parseJsonFile (jsonStyleFile);
    if (stylesheet.isObject())
        applyStylesRecursively (uiTree, stylesheet);

    registerCustomComponentTypes();
    root = viewInterpreter.interpret (uiTree);

    if (root != nullptr)
    {
        if (auto comp = root->getComponent())
            addAndMakeVisible (comp.get());

        viewInterpreter.listenTo (*root);
        installCallbacks();

        if (auto comp = root->getComponent())
        {
            workspaceResizeListener = std::make_unique<WorkspaceResizeListener> (*this);
            comp->addMouseListener (workspaceResizeListener.get(), true);
        }
    }

    rebuildTrackList();
    refreshFromState();

    setSize (1440, 700);
    resized();
}

GUIComponent::~GUIComponent()
{
    setLookAndFeel (nullptr);

    if (root == nullptr)
        return;

    if (workspaceResizeListener != nullptr)
    {
        if (auto comp = root->getComponent())
        {
            comp->removeMouseListener (workspaceResizeListener.get());
        }
    }
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
    const auto nodeId = node.state.getProperty ("id").toString();
    if (nodeId == id.toString())
        return &node;

    for (auto* child : node.getChildren())
    {
        if (child != nullptr)
            if (auto* found = findGuiItemById (*child, id))
                return found;
    }

    return nullptr;
}

int GUIComponent::getIntProperty (const juce::ValueTree& v,
                                  const juce::Identifier& key,
                                  int fallback)
{
    if (! v.isValid() || ! v.hasProperty (key))
        return fallback;

    const auto value = v.getProperty (key);

    if (value.isInt() || value.isInt64())
        return static_cast<int> (value);

    return value.toString().trim().getIntValue();
}

bool GUIComponent::isNearRightEdge (const juce::MouseEvent& e, juce::Component& target, int gripWidth)
{
    auto relative = e.getEventRelativeTo (&target);
    return relative.x >= 0
        && relative.y >= 0
        && relative.x < target.getWidth()
        && relative.y < target.getHeight()
        && relative.x >= target.getWidth() - gripWidth;
}

bool GUIComponent::isNearLeftEdge (const juce::MouseEvent& e, juce::Component& target, int gripWidth)
{
    auto relative = e.getEventRelativeTo (&target);
    return relative.x >= 0
        && relative.y >= 0
        && relative.x < target.getWidth()
        && relative.y < target.getHeight()
        && relative.x <= gripWidth;
}

void GUIComponent::applyManualBodyLayout()
{
    if (root == nullptr)
        return;

    auto* headerItem = findGuiItemById (*root, juce::Identifier ("headerContent"));
    auto* bodyItem = findGuiItemById (*root, juce::Identifier ("bodyContent"));
    auto* sidebarItem = findGuiItemById (*root, juce::Identifier ("sidebarContent"));
    auto* workspaceItem = findGuiItemById (*root, juce::Identifier ("workspaceContent"));
    auto* clipSidebarItem = findGuiItemById (*root, juce::Identifier ("clipSidebarContent"));
    if (headerItem == nullptr || bodyItem == nullptr || sidebarItem == nullptr || workspaceItem == nullptr || clipSidebarItem == nullptr)
        return;

    auto headerComp = headerItem->getComponent();
    auto bodyComp = bodyItem->getComponent();
    auto sidebarComp = sidebarItem->getComponent();
    auto workspaceComp = workspaceItem->getComponent();
    auto clipSidebarComp = clipSidebarItem->getComponent();
    if (headerComp == nullptr || bodyComp == nullptr || sidebarComp == nullptr || workspaceComp == nullptr || clipSidebarComp == nullptr)
        return;

    auto fullArea = getLocalBounds();
    auto headerBounds = fullArea.removeFromTop (98);
    auto bodyBounds = fullArea;
    const int clipSidebarWidth = state.clipSidebarCollapsed ? 44 : state.clipSidebarWidth;
    const int workspaceWidth = juce::jmax (0, bodyBounds.getWidth() - state.sidebarWidth - clipSidebarWidth);

    headerComp->setBounds (headerBounds);
    bodyComp->setBounds (bodyBounds);

    auto bodyLocalBounds = bodyComp->getLocalBounds();
    sidebarComp->setBounds (bodyLocalBounds.removeFromLeft (state.sidebarWidth));
    workspaceComp->setBounds (bodyLocalBounds.removeFromLeft (workspaceWidth));
    clipSidebarComp->setBounds (bodyLocalBounds.removeFromLeft (clipSidebarWidth));

    if (auto* headerMainRowItem = findGuiItemById (*root, juce::Identifier ("headerMainRow")))
    {
        if (auto headerMainRowComp = headerMainRowItem->getComponent())
        {
            auto headerInner = headerComp->getLocalBounds().reduced (10);
            headerMainRowComp->setBounds (headerInner.removeFromTop (52));
        }
    }

    auto sidebarArea = sidebarComp->getLocalBounds().reduced (10);

    if (auto* masterItem = findGuiItemById (*root, juce::Identifier ("masterSection")))
        if (auto masterComp = masterItem->getComponent())
        {
            masterComp->setBounds (sidebarArea.removeFromTop (298));
            sidebarArea.removeFromTop (12);
        }

    auto footerArea = sidebarArea.removeFromBottom (78);

    juce::Component::SafePointer<juce::Component> mixerPanelComp;
    if (auto* mixerPanelItem = findGuiItemById (*root, juce::Identifier ("mixerPanel")))
        mixerPanelComp = mixerPanelItem->getComponent().get();

    if (mixerPanelComp != nullptr)
        mixerPanelComp->setBounds (sidebarArea);

    if (auto* newTrackItem = findGuiItemById (*root, juce::Identifier ("newTrackBtn")))
        if (auto newTrackComp = newTrackItem->getComponent())
            newTrackComp->setBounds (footerArea.withSizeKeepingCentre (170, 44));

    if (auto* trackListItem = findGuiItemById (*root, juce::Identifier ("trackList")))
        if (auto trackListComp = trackListItem->getComponent())
            if (mixerPanelComp != nullptr)
                trackListComp->setBounds (mixerPanelComp->getLocalBounds().reduced (6).withTrimmedRight (10));

    auto workspaceArea = workspaceComp->getLocalBounds();
    if (auto* timelinePanelItem = findGuiItemById (*root, juce::Identifier ("timelinePanel")))
    {
        if (auto timelinePanelComp = timelinePanelItem->getComponent())
            timelinePanelComp->setBounds (workspaceArea.removeFromTop (90));
    }

    workspaceArea.removeFromTop (12);

    if (auto* arrangementPanelItem = findGuiItemById (*root, juce::Identifier ("arrangementPanel")))
        if (auto arrangementPanelComp = arrangementPanelItem->getComponent())
            arrangementPanelComp->setBounds (workspaceArea);

    auto clipArea = clipSidebarComp->getLocalBounds().reduced (10);
    if (auto* headerNode = findGuiItemById (*root, juce::Identifier ("clipBrowserHeader")))
    {
        if (auto clipHeaderComp = headerNode->getComponent())
        {
            clipHeaderComp->setBounds (clipArea.removeFromTop (52));
            clipArea.removeFromTop (4);
        }
    }

    if (auto* clipListItem = findGuiItemById (*root, juce::Identifier ("clipList")))
        if (auto clipListComp = clipListItem->getComponent())
            clipListComp->setBounds (clipArea);
}

void GUIComponent::setSidebarWidth (int newWidth)
{
    auto sidebarNode = jive::findElementWithID (uiTree, juce::Identifier ("sidebarContent"));
    if (! sidebarNode.isValid())
        return;

    const int minW = getIntProperty (sidebarNode, juce::Identifier ("min-width"), 180);
    const int maxW = getIntProperty (sidebarNode, juce::Identifier ("max-width"), 420);

    newWidth = juce::jlimit (minW, maxW, newWidth);
    sidebarNode.setProperty ("width", newWidth, nullptr);
    state.sidebarWidth = newWidth;

    root->callLayoutChildrenWithRecursionLock();
    resized();
}

void GUIComponent::setClipSidebarWidth (int newWidth)
{
    auto clipSidebarNode = jive::findElementWithID (uiTree, juce::Identifier ("clipSidebarContent"));
    if (! clipSidebarNode.isValid())
        return;

    const int minW = getIntProperty (clipSidebarNode, juce::Identifier ("min-width"), 160);
    const int maxW = getIntProperty (clipSidebarNode, juce::Identifier ("max-width"), 320);
    const int collapsedWidth = 44;

    if (newWidth <= collapsedWidth)
    {
        state.clipSidebarCollapsed = true;
    }
    else
    {
        state.clipSidebarCollapsed = false;
        state.clipSidebarWidth = juce::jlimit (minW, maxW, newWidth);
    }

    refreshFromState();
}

void GUIComponent::installCallbacks()
{
    auto bindButton = [this] (const juce::String& id, std::function<void()> fn)
    {
        if (root == nullptr)
            return;

        if (auto* item = findGuiItemById (*root, juce::Identifier (id)))
        {
            if (auto comp = item->getComponent())
            {
                if (auto* button = dynamic_cast<juce::Button*> (comp.get()))
                {
                    allowClicksToPassThroughChildren (*button);
                    button->onClick = std::move (fn);
                }
            }
        }
    };

    bindButton ("playBtn", [this]
    {
        if (onPlayRequested)
            onPlayRequested();
    });

    bindButton ("pauseBtn", [this]
    {
        if (onPauseRequested)
            onPauseRequested();
    });

    bindButton ("restartBtn", [this]
    {
        if (onRestartRequested)
            onRestartRequested();
    });

    bindButton ("recordBtn", [this]
    {
        if (state.isRecording)
        {
            if (onStopRequested)
                onStopRequested();

            return;
        }

        if (onRecordToggleRequested)
            onRecordToggleRequested();
    });

    bindButton ("monitorBtn", [this]
    {
        if (onMonitoringToggleRequested)
        {
            onMonitoringToggleRequested();
            return;
        }

        state.toggleAudioMonitoring();
        refreshFromState();
    });

    bindButton ("settingsBtn", [this]
    {
        openSettingsWindow();
    });

    bindButton ("newTrackBtn", [this]
    {
        state.addTrack();

        juce::MessageManager::callAsync ([this]
        {
            rebuildTrackList();
            refreshFromState();
        });
    });

    bindButton ("toggleClipSidebarBtn", [this]
    {
        state.toggleClipSidebar();
        refreshFromState();
    });

    bindButton ("pianoRollBtn", [this]
    {
        if (pianoRollWindow == nullptr)
        {
            pianoRollWindow = std::make_unique<PianoRollWindow>();
            pianoRollWindow->setNotesChangedCallback ([this] (const std::vector<PianoRoll::Note>& notes)
            {
                auto& pattern = state.getSelectedTrackPatternState();

                if (! samePatternNotes (pattern.notes, notes))
                    pattern.notes = toTrackPatternNotes (notes);
            });
        }

        syncPianoRollWithSelectedTrack();
        pianoRollWindow->setVisible (true);
        pianoRollWindow->toFront (true);
    });

    bindButton ("fileBtn", [this]
    {
        if (onImportAudioRequested)
            onImportAudioRequested();
    });
    bindDynamicTrackButtons();
}

void GUIComponent::bindDynamicTrackButtons()
{
    if (root == nullptr)
        return;

    for (int i = 0; i < state.trackCount; ++i)
    {
        const auto buttonId = "removeTrackBtn" + juce::String (i + 1);

        if (auto* item = findGuiItemById (*root, juce::Identifier (buttonId)))
        {
            if (auto comp = item->getComponent())
            {
                if (auto* button = dynamic_cast<juce::Button*> (comp.get()))
                {
                    button->onClick = [this, i]
                    {
                        state.removeTrackAt (i);

                        juce::MessageManager::callAsync ([this]
                        {
                            rebuildTrackList();
                            refreshFromState();
                        });
                    };
                }
            }
        }
    }
}

void GUIComponent::openSettingsWindow()
{
    if (settingsWindow == nullptr)
        settingsWindow = std::make_unique<SettingsWindow> (deviceManager);

    settingsWindow->setVisible (true);
    settingsWindow->toFront (true);
}

void GUIComponent::syncPianoRollWithSelectedTrack()
{
    if (pianoRollWindow == nullptr)
        return;

    const auto selectedTrackIndex = juce::jlimit (0, state.trackCount - 1, state.selectedTrackIndex);
    const auto& pattern = state.getTrackPatternState (selectedTrackIndex);

    pianoRollWindow->setTrackContext (state.getTrackName (selectedTrackIndex),
                                      state.getTrackPatternName (selectedTrackIndex));
    pianoRollWindow->setNotes (toPianoRollNotes (pattern));
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

juce::StringArray GUIComponent::getAvailableTrackOutputs() const
{
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup(setup);
    juce::StringArray outputs;

    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        const auto outputNames = device->getOutputChannelNames();
        for (int i = 0; i < outputNames.size(); ++i)
            if (setup.outputChannels[i])
                outputs.add(outputNames[i]);

        if (outputs.isEmpty())
            outputs = outputNames;
    }

    outputs.removeDuplicates(false);
    return outputs;
}

juce::String GUIComponent::getTrackInputDeviceName() const
{
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup(setup);
    return setup.inputDeviceName.isNotEmpty() ? setup.inputDeviceName : "Inputs";
}

juce::String GUIComponent::getTrackOutputDeviceName() const
{
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup(setup);
    return setup.outputDeviceName.isNotEmpty() ? setup.outputDeviceName : "Outputs";
}

juce::StringArray GUIComponent::getAvailableMasterOutputs() const
{
    return getAvailableTrackOutputs();
}

juce::String GUIComponent::getMasterOutputDeviceName() const
{
    return getTrackOutputDeviceName();
}

void GUIComponent::refreshFromState()
{
    if (! uiTree.isValid())
        return;

    if (root == nullptr)
        return;

    syncPianoRollWithSelectedTrack();

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
            node.setProperty ("style", makeStyleStringForClasses (className, stylesheet), nullptr);
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

    if (auto* recordSprite = findMatchingSprite (juce::StringArray{ state.isRecording ? "stop" : "record", "recordbtn" }, spriteAssets))
        setPropertyById ("recordBtnSprite", juce::Identifier ("source"), *recordSprite);
    
    setTextById ("monitorBtnLabel",
             state.audioMonitoringEnabled ? "Monitor On" : "Monitor Off");

    setClassById ("monitorBtn",
              state.audioMonitoringEnabled ? "btn btn-monitor-on"
                                           : "btn btn-monitor-off");

    setTextById ("newTrackBtnLabel",
                 "Add Track +");

    auto sidebarNode = jive::findElementWithID (uiTree, juce::Identifier ("sidebarContent"));
    if (sidebarNode.isValid())
        sidebarNode.setProperty ("width", state.sidebarWidth, nullptr);

    auto clipSidebarNode = jive::findElementWithID (uiTree, juce::Identifier ("clipSidebarContent"));
    if (clipSidebarNode.isValid())
    {
        const int collapsedWidth = 44;
        clipSidebarNode.setProperty ("width", state.clipSidebarCollapsed ? collapsedWidth : state.clipSidebarWidth, nullptr);
        clipSidebarNode.setProperty ("min-width", state.clipSidebarCollapsed ? collapsedWidth : 160, nullptr);
        clipSidebarNode.setProperty ("max-width", state.clipSidebarCollapsed ? collapsedWidth : 320, nullptr);
    }

    setPropertyById ("clipBrowserHeader", juce::Identifier ("display"),
                     state.clipSidebarCollapsed ? "none" : "flex");

    setPropertyById ("clipList", juce::Identifier ("display"),
                     state.clipSidebarCollapsed ? "none" : "flex");

    setPropertyById ("clipSidebarSpacer", juce::Identifier ("display"),
                     state.clipSidebarCollapsed ? "none" : "flex");

    root->callLayoutChildrenWithRecursionLock();
    repaint();
}

void GUIComponent::rebuildTrackList()
{
    if (trackListComponent != nullptr)
        trackListComponent->repaint();
}

void GUIComponent::refreshExternalState(bool shouldRefreshControls, bool shouldRebuildTrackList)
{
    if (shouldRebuildTrackList)
        rebuildTrackList();

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

void GUIComponent::WorkspaceResizeListener::mouseDown (const juce::MouseEvent& e)
{
    auto* sidebarItem = GUIComponent::findGuiItemById (*owner.root, juce::Identifier ("sidebarContent"));
    auto* workspaceItem = GUIComponent::findGuiItemById (*owner.root, juce::Identifier ("workspaceContent"));
    if (sidebarItem == nullptr || workspaceItem == nullptr)
        return;

    auto sidebarComp = sidebarItem->getComponent();
    auto workspaceComp = workspaceItem->getComponent();
    if (sidebarComp == nullptr || workspaceComp == nullptr)
        return;

    const bool nearSidebarRight = GUIComponent::isNearRightEdge (e, *sidebarComp, GUIComponent::sidebarEdgeGripWidth);
    const bool nearWorkspaceLeft = GUIComponent::isNearLeftEdge (e, *workspaceComp, GUIComponent::sidebarEdgeGripWidth);

    if (nearSidebarRight || nearWorkspaceLeft)
    {
        owner.activeWorkspaceResizeEdge = GUIComponent::WorkspaceResizeEdge::left;
        auto sidebarNode = jive::findElementWithID (owner.uiTree, juce::Identifier ("sidebarContent"));
        owner.sidebarStartWidth = GUIComponent::getIntProperty (sidebarNode, juce::Identifier ("width"), 340);
    }
    else if (GUIComponent::isNearRightEdge (e, *workspaceComp, GUIComponent::sidebarEdgeGripWidth))
    {
        owner.activeWorkspaceResizeEdge = GUIComponent::WorkspaceResizeEdge::none;
        return;
    }
    else
    {
        owner.activeWorkspaceResizeEdge = GUIComponent::WorkspaceResizeEdge::none;
        return;
    }

    owner.draggingSidebar = owner.activeWorkspaceResizeEdge == GUIComponent::WorkspaceResizeEdge::left;
    owner.draggingClipSidebar = false;
    owner.dragStartScreenX = e.getScreenX();
}

void GUIComponent::WorkspaceResizeListener::mouseDrag (const juce::MouseEvent& e)
{
    const int delta = e.getScreenX() - owner.dragStartScreenX;
    if (owner.activeWorkspaceResizeEdge == GUIComponent::WorkspaceResizeEdge::left)
    {
        owner.setSidebarWidth (owner.sidebarStartWidth + delta);
    }
}

void GUIComponent::WorkspaceResizeListener::mouseUp (const juce::MouseEvent&)
{
    owner.draggingSidebar = false;
    owner.draggingClipSidebar = false;
    owner.activeWorkspaceResizeEdge = GUIComponent::WorkspaceResizeEdge::none;
}

void GUIComponent::WorkspaceResizeListener::mouseMove (const juce::MouseEvent& e)
{
    auto* sidebarItem = GUIComponent::findGuiItemById (*owner.root, juce::Identifier ("sidebarContent"));
    auto* workspaceItem = GUIComponent::findGuiItemById (*owner.root, juce::Identifier ("workspaceContent"));
    if (sidebarItem == nullptr || workspaceItem == nullptr)
        return;

    auto sidebarComp = sidebarItem->getComponent();
    auto workspaceComp = workspaceItem->getComponent();
    if (sidebarComp == nullptr || workspaceComp == nullptr)
        return;

    const bool nearDivider = GUIComponent::isNearRightEdge (e, *sidebarComp, GUIComponent::sidebarEdgeGripWidth)
                          || GUIComponent::isNearLeftEdge (e, *workspaceComp, GUIComponent::sidebarEdgeGripWidth);
    const auto cursor = nearDivider ? juce::MouseCursor::LeftRightResizeCursor
                                    : juce::MouseCursor::NormalCursor;

    if (auto rootComp = owner.root->getComponent())
        rootComp->setMouseCursor (cursor);

    sidebarComp->setMouseCursor (cursor);
    workspaceComp->setMouseCursor (cursor);
    e.eventComponent->setMouseCursor (cursor);
}

void GUIComponent::WorkspaceResizeListener::mouseExit (const juce::MouseEvent& e)
{
    if (auto rootComp = owner.root->getComponent())
        rootComp->setMouseCursor (juce::MouseCursor::NormalCursor);

    if (auto* workspaceItem = GUIComponent::findGuiItemById (*owner.root, juce::Identifier ("workspaceContent")))
        if (auto workspaceComp = workspaceItem->getComponent())
            workspaceComp->setMouseCursor (juce::MouseCursor::NormalCursor);
}
