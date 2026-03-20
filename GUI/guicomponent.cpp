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

GUIComponent::GUIComponent()
{
    deviceManager.initialise (0, 2, nullptr, true);

    const auto resourcesDir = findResourcesDir();

    const auto xmlFile = resourcesDir.getChildFile ("ui")
                                     .getChildFile ("views")
                                     .getChildFile ("main_view.xml");

    const auto jsonStyleFile = resourcesDir.getChildFile ("ui")
                                           .getChildFile ("styles")
                                           .getChildFile ("styles.json");

    DBG ("=== UI LOAD DEBUG ===");
    DBG ("Resources Dir: " << resourcesDir.getFullPathName());
    DBG ("XML path: " << xmlFile.getFullPathName());
    DBG ("Style path: " << jsonStyleFile.getFullPathName());

    if (xmlFile.existsAsFile())
    {
        juce::XmlDocument doc (xmlFile);

        if (auto xmlElement = doc.getDocumentElement())
        {
            uiTree = juce::ValueTree::fromXml (*xmlElement);
        }
        else
        {
            DBG ("XML parse failed: " << doc.getLastParseError());

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
    {
        applyStylesRecursively (uiTree, stylesheet);
        DBG ("Applied external stylesheet.");
    }
    else
    {
        DBG ("Failed to load external stylesheet. Inline styles only.");
    }

    root = viewInterpreter.interpret (uiTree);

    if (root != nullptr)
    {
        if (auto comp = root->getComponent())
            addAndMakeVisible (comp.get());

        viewInterpreter.listenTo (*root);
        installCallbacks();

        timelineComponent = std::make_unique<TimelineComponent> (state);
        addAndMakeVisible (*timelineComponent);

        arrangementComponent = std::make_unique<ArrangementComponent> (state);
        addAndMakeVisible (*arrangementComponent);

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
    rebuildClipList();
    refreshFromState();
    startTimerHz (60);

    setSize (1280, 720);
    resized();
}

GUIComponent::~GUIComponent()
{
    stopTimer();

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

    if (timelineComponent != nullptr)
    {
        if (auto* timelineItem = findGuiItemById (*root, juce::Identifier ("timelinePanel")))
        {
            if (auto timelinePanel = timelineItem->getComponent())
            {
                auto screenBounds = timelinePanel->getScreenBounds();
                auto localBounds = getLocalArea (nullptr, screenBounds);
                timelineComponent->setBounds (localBounds.reduced (8));
            }
        }
    }

    if (arrangementComponent != nullptr)
    {
        if (auto* arrangementItem = findGuiItemById (*root, juce::Identifier ("arrangementPanel")))
        {
            if (auto arrangementPanel = arrangementItem->getComponent())
            {
                auto screenBounds = arrangementPanel->getScreenBounds();
                auto localBounds = getLocalArea (nullptr, screenBounds);
                arrangementComponent->setBounds (localBounds.reduced (16));
            }
        }
    }
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
        state.play();
        refreshFromState();
    });

    bindButton ("stopBtn", [this]
    {
        state.stop();

        juce::MessageManager::callAsync ([this]
        {
            rebuildClipList();
            refreshFromState();
        });
    });

    bindButton ("pauseBtn", [this]
    {
        state.pause();
        refreshFromState();
    });

    bindButton ("restartBtn", [this]
    {
        state.restart();
        refreshFromState();
    });

    bindButton ("recordBtn", [this]
    {
        const auto clipCountBefore = state.recordedClips.size();
        state.toggleRecord();

        juce::MessageManager::callAsync ([this, clipCountBefore]
        {
            if (state.recordedClips.size() != clipCountBefore)
                rebuildClipList();

            refreshFromState();
        });
    });

    bindButton ("monitorBtn", [this]
    {
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

    bindButton ("fileBtn", [] {});
    bindButton ("undoBtn", [] {});
    bindButton ("redoBtn", [] {});

    if (root == nullptr)
        return;

    if (auto* sliderItem = findGuiItemById (*root, juce::Identifier ("timelineSlider")))
    {
        if (auto comp = sliderItem->getComponent())
        {
            if (auto* slider = dynamic_cast<juce::Slider*> (comp.get()))
            {
                slider->setRange (-60.0, 6.0, 0.1);
                slider->setValue (state.volumeDb, juce::dontSendNotification);

                slider->onValueChange = [this, slider]
                {
                    state.volumeDb = slider->getValue();
                    refreshFromState();
                };
            }
        }
    }

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

    auto setValueById = [this] (const juce::String& id, double value)
    {
        auto node = jive::findElementWithID (uiTree, juce::Identifier (id));
        if (node.isValid())
            node.setProperty ("value", value, nullptr);
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
                 state.transportState == DAWState::TransportState::stopped ? "Stopped" : "Stop");

    setTextById ("pauseBtnLabel",
                 state.transportState == DAWState::TransportState::paused ? "Paused" : "Pause");

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
                 "New Track (" + juce::String (state.trackCount) + ")");

    setTextById ("dbLabel",
                 juce::String (state.volumeDb, 1) + " dB");

    setValueById ("timelineSlider", state.volumeDb);

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

void GUIComponent::rebuildClipList()
{
    auto clipListNode = jive::findElementWithID (uiTree, juce::Identifier ("clipList"));
    if (! clipListNode.isValid())
        return;

    while (clipListNode.getNumChildren() > 0)
        clipListNode.removeChild (0, nullptr);

    const auto clipItemStyle = makeStyleStringForClasses ("clipItem", stylesheet);
    const auto clipNameStyle = makeStyleStringForClasses ("label", stylesheet);
    const auto secondaryStyle = makeStyleStringForClasses ("secondary-label", stylesheet);

    if (state.recordedClips.empty())
    {
        auto emptyText = juce::ValueTree ("Text");
        emptyText.setProperty ("id", "emptyClipListLabel", nullptr);
        emptyText.setProperty ("class", "secondary-label", nullptr);
        emptyText.setProperty ("style", secondaryStyle, nullptr);
        emptyText.setProperty ("text", "No recorded clips yet", nullptr);
        clipListNode.appendChild (emptyText, nullptr);

        root->callLayoutChildrenWithRecursionLock();
        return;
    }

    for (int i = 0; i < (int) state.recordedClips.size(); ++i)
    {
        auto clipItem = juce::ValueTree ("Component");
        clipItem.setProperty ("id", "clipItem" + juce::String (i + 1), nullptr);
        clipItem.setProperty ("class", "clipItem", nullptr);
        clipItem.setProperty ("style", clipItemStyle, nullptr);
        clipItem.setProperty ("display", "flex", nullptr);
        clipItem.setProperty ("flex-direction", "column", nullptr);
        clipItem.setProperty ("padding", 8, nullptr);
        clipItem.setProperty ("row-gap", 3, nullptr);
        clipItem.setProperty ("min-height", 52, nullptr);

        auto nameText = juce::ValueTree ("Text");
        nameText.setProperty ("id", "clipName" + juce::String (i + 1), nullptr);
        nameText.setProperty ("class", "label", nullptr);
        nameText.setProperty ("style", clipNameStyle, nullptr);
        nameText.setProperty ("text", state.recordedClips[(size_t) i], nullptr);

        auto metaText = juce::ValueTree ("Text");
        metaText.setProperty ("id", "clipMeta" + juce::String (i + 1), nullptr);
        metaText.setProperty ("class", "secondary-label", nullptr);
        metaText.setProperty ("style", secondaryStyle, nullptr);
        metaText.setProperty ("text", "Recorded audio clip", nullptr);

        clipItem.appendChild (nameText, nullptr);
        clipItem.appendChild (metaText, nullptr);
        clipListNode.appendChild (clipItem, nullptr);
    }

    root->callLayoutChildrenWithRecursionLock();
}

void GUIComponent::timerCallback()
{
    state.tick (1.0 / 60.0);
    refreshFromState();

    if (timelineComponent != nullptr)
        timelineComponent->repaint();

    if (arrangementComponent != nullptr)
        arrangementComponent->repaint();
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