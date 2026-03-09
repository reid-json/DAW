#include "guicomponent.h"

#include <juce_core/juce_core.h>
#include <jive_core/algorithms/jive_Find.h>

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

        const auto text = file.loadFileAsString();
        return juce::JSON::parse (text);
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

    const auto stylesheet = parseJsonFile (jsonStyleFile);
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

    refreshFromState();
    startTimerHz (60);

    setSize (1100, 720);
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
}

jive::GuiItem* GUIComponent::findGuiItemById (jive::GuiItem& node, const juce::Identifier& id)
{
    const auto nodeId = node.state.getProperty ("id").toString();
    if (nodeId == id.toString())
        return &node;

    for (auto* child : node.getChildren())
    {
        if (child != nullptr)
        {
            if (auto* found = findGuiItemById (*child, id))
                return found;
        }
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

    if (root != nullptr)
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
        DBG ("Play pressed");
        refreshFromState();
    });

    bindButton ("stopBtn", [this]
    {
        state.stop();
        DBG ("Stop pressed");
        refreshFromState();
    });

    bindButton ("pauseBtn", [this]
    {
        state.pause();
        DBG ("Pause pressed");
        refreshFromState();
    });

    bindButton ("restartBtn", [this]
    {
        state.restart();
        DBG ("Restart pressed");
        refreshFromState();
    });

    bindButton ("newTrackBtn", [this]
    {
        state.addTrack();
        DBG ("New Track pressed, trackCount = " << state.trackCount);
        rebuildTrackList();
        refreshFromState();
    });

    bindButton ("fileBtn", [] { DBG ("File pressed"); });
    bindButton ("undoBtn", [] { DBG ("Undo pressed"); });
    bindButton ("redoBtn", [] { DBG ("Redo pressed"); });

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
}

void GUIComponent::refreshFromState()
{
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

    setTextById ("playBtnLabel",
                 state.transportState == DAWState::TransportState::playing ? "Playing" : "Play");

    setTextById ("stopBtnLabel",
                 state.transportState == DAWState::TransportState::stopped ? "Stopped" : "Stop");

    setTextById ("pauseBtnLabel",
                 state.transportState == DAWState::TransportState::paused ? "Paused" : "Pause");

    setTextById ("newTrackBtnLabel",
                 "New Track (" + juce::String (state.trackCount) + ")");

    setTextById ("playheadLabel",
                 "Playhead: " + juce::String (state.playhead, 2));

    setTextById ("dbLabel",
                 juce::String (state.volumeDb, 1) + " dB");

    setValueById ("timelineSlider", state.volumeDb);

    auto sidebarNode = jive::findElementWithID (uiTree, juce::Identifier ("sidebarContent"));
    if (sidebarNode.isValid())
        sidebarNode.setProperty ("width", state.sidebarWidth, nullptr);

    if (root != nullptr)
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

    for (int i = 0; i < state.trackCount; ++i)
    {
        auto laneRow = juce::ValueTree ("Component");
        laneRow.setProperty ("class", "laneRow", nullptr);
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

        auto label = juce::ValueTree ("Text");
        label.setProperty ("class", "label", nullptr);
        label.setProperty ("text", "Track " + juce::String (i + 1), nullptr);

        laneRow.appendChild (laneAccent, nullptr);
        laneRow.appendChild (label, nullptr);
        trackListNode.appendChild (laneRow, nullptr);
    }
}

void GUIComponent::timerCallback()
{
    state.tick (1.0 / 60.0);
    refreshFromState();
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