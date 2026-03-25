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

        if (auto* resizerItem = findGuiItemById (*root, juce::Identifier ("sidebarResizer")))
        {
            if (auto resizerComp = resizerItem->getComponent())
            {
                resizerComp->setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
                resizerComp->setInterceptsMouseClicks (true, false);

                resizerListener = std::make_unique<SidebarResizerListener> (*this);
                resizerComp->addMouseListener (resizerListener.get(), false);
            }
        }
    }

    rebuildTrackList();
    refreshFromState();

    setSize (1280, 720);
    resized();
}

GUIComponent::~GUIComponent()
{
    if (root == nullptr || resizerListener == nullptr)
        return;

    if (auto* resizerItem = findGuiItemById (*root, juce::Identifier ("sidebarResizer")))
    {
        if (auto resizerComp = resizerItem->getComponent())
            resizerComp->removeMouseListener (resizerListener.get());
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

    setTextById ("newTrackBtnLabel",
                 "New Track (" + juce::String (state.trackCount) + ")");

    setTextById ("toggleClipSidebarBtnLabel",
             state.clipSidebarCollapsed ? ">" : "<");

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
    auto trackListNode = jive::findElementWithID (uiTree, juce::Identifier ("trackList"));
    if (! trackListNode.isValid())
        return;

    while (trackListNode.getNumChildren() > 0)
        trackListNode.removeChild (0, nullptr);

    const auto laneRowStyle = makeStyleStringForClasses ("laneRow", stylesheet);
    const auto laneAccentStyle = makeStyleStringForClasses ("laneAccent", stylesheet);
    const auto labelStyle = makeStyleStringForClasses ("label", stylesheet);
    const auto removeButtonStyle = makeStyleStringForClasses ("btn btn-danger", stylesheet);

    for (int i = 0; i < state.trackCount; ++i)
    {
        auto laneRow = juce::ValueTree ("Component");
        laneRow.setProperty ("id", "trackRow" + juce::String (i + 1), nullptr);
        laneRow.setProperty ("class", "laneRow", nullptr);
        laneRow.setProperty ("style", laneRowStyle, nullptr);
        laneRow.setProperty ("height", 54, nullptr);
        laneRow.setProperty ("padding", 8, nullptr);
        laneRow.setProperty ("display", "flex", nullptr);
        laneRow.setProperty ("flex-direction", "row", nullptr);
        laneRow.setProperty ("align-items", "center", nullptr);
        laneRow.setProperty ("column-gap", 8, nullptr);

        auto laneAccent = juce::ValueTree ("Component");
        laneAccent.setProperty ("width", 10, nullptr);
        laneAccent.setProperty ("height", 24, nullptr);
        laneAccent.setProperty ("class", "laneAccent", nullptr);
        laneAccent.setProperty ("style", laneAccentStyle, nullptr);

        auto label = juce::ValueTree ("Text");
        label.setProperty ("id", "trackLabel" + juce::String (i + 1), nullptr);
        label.setProperty ("class", "label", nullptr);
        label.setProperty ("style", labelStyle, nullptr);
        label.setProperty ("text", "Track " + juce::String (i + 1), nullptr);
        label.setProperty ("flex-grow", 1, nullptr);

        auto removeButton = juce::ValueTree ("Button");
        removeButton.setProperty ("id", "removeTrackBtn" + juce::String (i + 1), nullptr);
        removeButton.setProperty ("class", "btn btn-danger", nullptr);
        removeButton.setProperty ("style", removeButtonStyle, nullptr);
        removeButton.setProperty ("width", 32, nullptr);
        removeButton.setProperty ("height", 28, nullptr);

        auto removeText = juce::ValueTree ("Text");
        removeText.setProperty ("id", "removeTrackBtnLabel" + juce::String (i + 1), nullptr);
        removeText.setProperty ("text", "-", nullptr);

        removeButton.appendChild (removeText, nullptr);

        laneRow.appendChild (laneAccent, nullptr);
        laneRow.appendChild (label, nullptr);
        laneRow.appendChild (removeButton, nullptr);
        trackListNode.appendChild (laneRow, nullptr);
    }

    root->callLayoutChildrenWithRecursionLock();
    bindDynamicTrackButtons();
}

void GUIComponent::refreshExternalState(bool shouldRefreshControls, bool shouldRebuildTrackList)
{
    if (shouldRebuildTrackList)
        rebuildTrackList();

    if (shouldRefreshControls)
        refreshFromState();

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
}

void GUIComponent::SidebarResizerListener::mouseDown (const juce::MouseEvent& e)
{
    owner.draggingSidebar = true;
    owner.dragStartScreenX = e.getScreenX();

    auto sidebarNode = jive::findElementWithID (owner.uiTree, juce::Identifier ("sidebarContent"));
    owner.sidebarStartWidth = GUIComponent::getIntProperty (sidebarNode, juce::Identifier ("width"), 240);
}

void GUIComponent::SidebarResizerListener::mouseDrag (const juce::MouseEvent& e)
{
    if (! owner.draggingSidebar)
        return;

    const int delta = e.getScreenX() - owner.dragStartScreenX;
    owner.setSidebarWidth (owner.sidebarStartWidth + delta);
}

void GUIComponent::SidebarResizerListener::mouseUp (const juce::MouseEvent&)
{
    owner.draggingSidebar = false;
}
