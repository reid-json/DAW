#include "guicomponent.h"

#include <juce_core/juce_core.h>
#include <jive_core/algorithms/jive_Find.h>
#include "timelinecomponent.h"

namespace
{
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
        component->onMasterPluginLoadRequested = [this] (int slotIndex, const juce::String& pluginName)
        {
            return pluginHostManager.loadMasterPlugin(pluginName, slotIndex);
        };
        component->onMasterPluginBypassRequested = [this] (int slotIndex, bool shouldBeBypassed)
        {
            pluginHostManager.setMasterPluginBypassed(slotIndex, shouldBeBypassed);
        };
        component->onMasterPluginRemoveRequested = [this] (int slotIndex)
        {
            pluginHostManager.removeMasterPlugin(slotIndex);
        };
        component->onMasterPluginEditorRequested = [this] (int slotIndex)
        {
            pluginHostManager.showMasterPluginEditor(slotIndex);
        };
        component->onMasterPluginSlotRemoveRequested = [this] (int slotIndex)
        {
            pluginHostManager.removeMasterPluginSlot(slotIndex);
        };
        mixerMasterComponent = component.get();
        return component;
    });

    viewInterpreter.getComponentFactory().set ("TrackListView", [this]
    {
        auto component = std::make_unique<TrackListComponent> (state);
        component->onRemoveTrackRequested = [this] (int trackIndex)
        {
            pluginHostManager.removeTrack(trackIndex);
            state.removeTrackAt (trackIndex);
            refreshFromState();
        };
        component->getAvailableTrackPlugins = [this]
        {
            return pluginHostManager.getAvailableTrackPlugins();
        };
        component->onTrackPluginLoadRequested = [this] (int trackIndex, int slotIndex, const juce::String& pluginName)
        {
            return pluginHostManager.loadTrackPlugin(pluginName, trackIndex, slotIndex);
        };
        component->onTrackPluginBypassRequested = [this] (int trackIndex, int slotIndex, bool shouldBeBypassed)
        {
            pluginHostManager.setTrackPluginBypassed(trackIndex, slotIndex, shouldBeBypassed);
        };
        component->onTrackPluginRemoveRequested = [this] (int trackIndex, int slotIndex)
        {
            pluginHostManager.removeTrackPlugin(trackIndex, slotIndex);
        };
        component->onTrackPluginEditorRequested = [this] (int trackIndex, int slotIndex)
        {
            pluginHostManager.showTrackPluginEditor(trackIndex, slotIndex);
        };
        component->onTrackPluginSlotRemoveRequested = [this] (int trackIndex, int slotIndex)
        {
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
        recentClipsComponent = component.get();
        return component;
    });
}

GUIComponent::GUIComponent(juce::AudioDeviceManager& sharedDeviceManager)
    : deviceManager(sharedDeviceManager)
{
    setLookAndFeel (&lookAndFeel);

    const auto resourcesDir = findResourcesDir();

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
    auto headerBounds = fullArea.removeFromTop (86);
    auto bodyBounds = fullArea;
    const int clipSidebarWidth = state.clipSidebarCollapsed ? 44 : state.clipSidebarWidth;
    const int workspaceWidth = juce::jmax (0, bodyBounds.getWidth() - state.sidebarWidth - clipSidebarWidth);

    headerComp->setBounds (headerBounds);
    bodyComp->setBounds (bodyBounds);

    auto bodyLocalBounds = bodyComp->getLocalBounds();
    sidebarComp->setBounds (bodyLocalBounds.removeFromLeft (state.sidebarWidth));
    workspaceComp->setBounds (bodyLocalBounds.removeFromLeft (workspaceWidth));
    clipSidebarComp->setBounds (bodyLocalBounds.removeFromLeft (clipSidebarWidth));

    if (auto* headerTopRowItem = findGuiItemById (*root, juce::Identifier ("headerTopRow")))
    {
        if (auto headerTopRowComp = headerTopRowItem->getComponent())
            headerTopRowComp->setBounds (headerComp->getLocalBounds().reduced (10).removeFromTop (28));
    }

    if (auto* headerMainRowItem = findGuiItemById (*root, juce::Identifier ("headerMainRow")))
    {
        if (auto headerMainRowComp = headerMainRowItem->getComponent())
        {
            auto headerInner = headerComp->getLocalBounds().reduced (10);
            headerInner.removeFromTop (30);
            headerMainRowComp->setBounds (headerInner.removeFromTop (40));
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
                    button->onClick = std::move (fn);
            }
        }
    };

    bindButton ("playBtn", [this]
    {
        if (onPlayRequested)
            onPlayRequested();
    });

    bindButton ("stopBtn", [this]
    {
        if (onStopRequested)
            onStopRequested();
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
            pianoRollWindow = std::make_unique<PianoRollWindow>();

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

    setTextById ("stopBtnLabel",
                 "Stop");

    setTextById ("pauseBtnLabel",
                 state.transportState == DAWState::TransportState::paused ? "Resume" : "Pause");

    setTextById ("recordBtnLabel",
                 state.isRecording ? "Recording" : "Record");

    setClassById ("recordBtn",
                  state.isRecording ? "btn btn-record-active" : "btn btn-record");
    
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
