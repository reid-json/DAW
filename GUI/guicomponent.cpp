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
            rebuildClipList();
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
                slider->setSliderStyle (juce::Slider::LinearHorizontal);
                slider->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
                slider->setRange (-60.0, 6.0, 0.1);
                slider->setValue (state.masterVolumeDb, juce::dontSendNotification);

                slider->onValueChange = [this, slider]
                {
                    state.masterVolumeDb = (float) slider->getValue();
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

    if (auto* masterItem = findGuiItemById (*root, juce::Identifier ("masterVolumeSlider")))
    {
        if (auto comp = masterItem->getComponent())
        {
            if (auto* slider = dynamic_cast<juce::Slider*> (comp.get()))
            {
                slider->setSliderStyle (juce::Slider::LinearHorizontal);
                slider->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
                slider->setRange (-60.0, 6.0, 0.1);
                slider->setValue (state.masterVolumeDb, juce::dontSendNotification);

                slider->setColour (juce::Slider::trackColourId,
                                   juce::Colour::fromRGBA (70, 78, 98, 255));
                slider->setColour (juce::Slider::backgroundColourId,
                                   juce::Colour::fromRGBA (28, 34, 48, 255));
                slider->setColour (juce::Slider::thumbColourId,
                                   juce::Colour::fromRGB (70, 160, 255));

                slider->onValueChange = [this, slider]
                {
                    state.masterVolumeDb = (float) slider->getValue();
                    refreshFromState();
                };
            }
        }
    }

    for (int i = 0; i < state.getNumTracks(); ++i)
    {
        const auto index = i + 1;

        if (auto* removeItem = findGuiItemById (*root, juce::Identifier ("removeTrackBtn" + juce::String (index))))
        {
            if (auto comp = removeItem->getComponent())
            {
                if (auto* button = dynamic_cast<juce::Button*> (comp.get()))
                {
                    button->onClick = [this, i]
                    {
                        state.removeTrackAt (i);

                        juce::MessageManager::callAsync ([this]
                        {
                            rebuildTrackList();
                            rebuildClipList();
                            refreshFromState();
                        });
                    };
                }
            }
        }

        if (auto* fxItem = findGuiItemById (*root, juce::Identifier ("fxTrackBtn" + juce::String (index))))
        {
            if (auto comp = fxItem->getComponent())
            {
                if (auto* button = dynamic_cast<juce::Button*> (comp.get()))
                {
                    button->onClick = [this, i]
                    {
                        state.selectedTrackIndex = i;

                        juce::MessageManager::callAsync ([this]
                        {
                            rebuildTrackList();
                            rebuildClipList();
                            refreshFromState();
                        });
                    };
                }
            }
        }

        if (auto* rowItem = findGuiItemById (*root, juce::Identifier ("trackRow" + juce::String (index))))
        {
            if (auto comp = rowItem->getComponent())
            {
                comp->setInterceptsMouseClicks (true, true);
                comp->addMouseListener (this, true);
            }
        }

        if (auto* armItem = findGuiItemById (*root, juce::Identifier ("armTrackBtn" + juce::String (index))))
        {
            if (auto comp = armItem->getComponent())
            {
                if (auto* button = dynamic_cast<juce::Button*> (comp.get()))
                {
                    button->onClick = [this, i]
                    {
                        state.getTrack (i).armed = ! state.getTrack (i).armed;

                        juce::MessageManager::callAsync ([this]
                        {
                            rebuildTrackList();
                            refreshFromState();
                        });
                    };
                }
            }
        }

        if (auto* muteItem = findGuiItemById (*root, juce::Identifier ("muteTrackBtn" + juce::String (index))))
        {
            if (auto comp = muteItem->getComponent())
            {
                if (auto* button = dynamic_cast<juce::Button*> (comp.get()))
                {
                    button->onClick = [this, i]
                    {
                        state.getTrack (i).muted = ! state.getTrack (i).muted;

                        juce::MessageManager::callAsync ([this]
                        {
                            rebuildTrackList();
                            refreshFromState();
                        });
                    };
                }
            }
        }

        if (auto* panItem = findGuiItemById (*root, juce::Identifier ("panSlider" + juce::String (index))))
        {
            if (auto comp = panItem->getComponent())
            {
                if (auto* slider = dynamic_cast<juce::Slider*> (comp.get()))
                {
                    slider->setSliderStyle (juce::Slider::LinearHorizontal);
                    slider->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
                    slider->setRange (-1.0, 1.0, 0.01);
                    slider->setValue (state.getTrack (i).pan, juce::dontSendNotification);

                    slider->setColour (juce::Slider::trackColourId,
                                       juce::Colour::fromRGBA (70, 78, 98, 255));
                    slider->setColour (juce::Slider::backgroundColourId,
                                       juce::Colour::fromRGBA (28, 34, 48, 255));
                    slider->setColour (juce::Slider::thumbColourId,
                                       juce::Colour::fromRGB (70, 160, 255));

                    slider->onValueChange = [this, i, slider]
                    {
                        state.getTrack (i).pan = (float) slider->getValue();
                        refreshFromState();
                    };
                }
            }
        }

        if (auto* volumeItem = findGuiItemById (*root, juce::Identifier ("volumeSlider" + juce::String (index))))
        {
            if (auto comp = volumeItem->getComponent())
            {
                if (auto* slider = dynamic_cast<juce::Slider*> (comp.get()))
                {
                    slider->setSliderStyle (juce::Slider::LinearHorizontal);
                    slider->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
                    slider->setRange (-60.0, 6.0, 0.1);
                    slider->setValue (state.getTrack (i).volumeDb, juce::dontSendNotification);

                    slider->setColour (juce::Slider::trackColourId,
                                       juce::Colour::fromRGBA (70, 78, 98, 255));
                    slider->setColour (juce::Slider::backgroundColourId,
                                       juce::Colour::fromRGBA (28, 34, 48, 255));
                    slider->setColour (juce::Slider::thumbColourId,
                                       juce::Colour::fromRGB (70, 160, 255));

                    slider->onValueChange = [this, i, slider]
                    {
                        state.getTrack (i).volumeDb = (float) slider->getValue();
                        refreshFromState();
                    };
                }
            }
        }
    }
}

void GUIComponent::mouseUp (const juce::MouseEvent& e)
{
    if (root == nullptr)
        return;

    for (int i = 0; i < state.getNumTracks(); ++i)
    {
        const auto id = "trackRow" + juce::String (i + 1);

        if (auto* item = findGuiItemById (*root, juce::Identifier (id)))
        {
            if (auto comp = item->getComponent())
            {
                auto* clicked = e.originalComponent != nullptr ? e.originalComponent
                                                               : e.eventComponent;

                if (clicked == comp.get() || comp->isParentOf (clicked))
                {
                    state.selectedTrackIndex = i;

                    juce::MessageManager::callAsync ([this]
                    {
                        rebuildTrackList();
                        rebuildClipList();
                        refreshFromState();
                    });

                    return;
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
    if (! uiTree.isValid() || root == nullptr)
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
                 "New Track (" + juce::String (state.getNumTracks()) + ")");

    setTextById ("dbLabel",
                 juce::String (state.masterVolumeDb, 1) + " dB");

    setValueById ("timelineSlider", state.masterVolumeDb);

    setTextById ("masterVolumeLabel",
                 "Vol: " + juce::String (state.masterVolumeDb, 1) + " dB");

    setValueById ("masterVolumeSlider", state.masterVolumeDb);

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

    for (int i = 0; i < state.getNumTracks(); ++i)
    {
        const bool isSelected = (i == state.selectedTrackIndex);

        setClassById ("trackRow" + juce::String (i + 1),
                  isSelected ? "laneRow-selected" : "laneRow");
    }

    if (auto comp = root->getComponent())
        comp->repaint();
}

void GUIComponent::rebuildTrackList()
{
    auto trackListNode = jive::findElementWithID (uiTree, juce::Identifier ("trackList"));
    if (! trackListNode.isValid())
        return;

    while (trackListNode.getNumChildren() > 0)
        trackListNode.removeChild (0, nullptr);

    trackListNode.appendChild (createMasterStrip(), nullptr);

    for (int i = 0; i < state.getNumTracks(); ++i)
        trackListNode.appendChild (createMixerStrip (i), nullptr);

    root->callLayoutChildrenWithRecursionLock();
    bindDynamicTrackButtons();
}

juce::ValueTree GUIComponent::createMixerStrip (int trackIndex)
{
    const auto labelStyle        = makeStyleStringForClasses ("label", stylesheet);
    const auto dangerButtonStyle = makeStyleStringForClasses ("btn btn-danger", stylesheet);
    const bool isSelected = (trackIndex == state.selectedTrackIndex);

    auto strip = juce::ValueTree ("Component");
    strip.setProperty ("id", "trackRow" + juce::String (trackIndex + 1), nullptr);
    strip.setProperty ("class", isSelected ? "laneRow-selected" : "laneRow", nullptr);
    strip.setProperty ("style",
                       makeStyleStringForClasses (isSelected ? "laneRow-selected" : "laneRow",
                                                  stylesheet),
                       nullptr);
    strip.setProperty ("display", "flex", nullptr);
    strip.setProperty ("flex-direction", "column", nullptr);
    strip.setProperty ("align-items", "stretch", nullptr);
    strip.setProperty ("row-gap", 10, nullptr);
    strip.setProperty ("padding", 10, nullptr);
    strip.setProperty ("min-height", 210, nullptr);

    auto headerRow = juce::ValueTree ("Component");
    headerRow.setProperty ("display", "flex", nullptr);
    headerRow.setProperty ("flex-direction", "row", nullptr);
    headerRow.setProperty ("align-items", "center", nullptr);
    headerRow.setProperty ("column-gap", 8, nullptr);

    auto label = juce::ValueTree ("Text");
    label.setProperty ("id", "trackLabel" + juce::String (trackIndex + 1), nullptr);
    label.setProperty ("class", "label", nullptr);
    label.setProperty ("style", labelStyle, nullptr);
    label.setProperty ("text", state.getTrack (trackIndex).name, nullptr);
    label.setProperty ("flex-grow", 1, nullptr);

    auto fxButton = juce::ValueTree ("Button");
    fxButton.setProperty ("id", "fxTrackBtn" + juce::String (trackIndex + 1), nullptr);
    fxButton.setProperty ("class", "btn", nullptr);
    fxButton.setProperty ("style", makeStyleStringForClasses ("btn", stylesheet), nullptr);
    fxButton.setProperty ("width", 40, nullptr);
    fxButton.setProperty ("height", 28, nullptr);

    auto fxText = juce::ValueTree ("Text");
    fxText.setProperty ("text", "FX", nullptr);
    fxButton.appendChild (fxText, nullptr);

    auto removeButton = juce::ValueTree ("Button");
    removeButton.setProperty ("id", "removeTrackBtn" + juce::String (trackIndex + 1), nullptr);
    removeButton.setProperty ("class", "btn btn-danger", nullptr);
    removeButton.setProperty ("style", dangerButtonStyle, nullptr);
    removeButton.setProperty ("width", 32, nullptr);
    removeButton.setProperty ("height", 28, nullptr);

    auto removeText = juce::ValueTree ("Text");
    removeText.setProperty ("text", "-", nullptr);
    removeButton.appendChild (removeText, nullptr);

    headerRow.appendChild (label, nullptr);
    headerRow.appendChild (fxButton, nullptr);
    headerRow.appendChild (removeButton, nullptr);

    auto buttonRow = juce::ValueTree ("Component");
    buttonRow.setProperty ("display", "flex", nullptr);
    buttonRow.setProperty ("flex-direction", "row", nullptr);
    buttonRow.setProperty ("align-items", "center", nullptr);
    buttonRow.setProperty ("column-gap", 8, nullptr);

    auto armButton = juce::ValueTree ("Button");
    armButton.setProperty ("id", "armTrackBtn" + juce::String (trackIndex + 1), nullptr);
    armButton.setProperty ("class", state.getTrack (trackIndex).armed ? "btn-arm-active" : "btn-arm", nullptr);
    armButton.setProperty ("style",
                           makeStyleStringForClasses (state.getTrack (trackIndex).armed ? "btn-arm-active" : "btn-arm",
                                                      stylesheet),
                           nullptr);
    armButton.setProperty ("width", 44, nullptr);
    armButton.setProperty ("height", 28, nullptr);

    auto armText = juce::ValueTree ("Text");
    armText.setProperty ("text", "Arm", nullptr);
    armButton.appendChild (armText, nullptr);

    auto muteButton = juce::ValueTree ("Button");
    muteButton.setProperty ("id", "muteTrackBtn" + juce::String (trackIndex + 1), nullptr);
    muteButton.setProperty ("class", state.getTrack (trackIndex).muted ? "btn-mute-active" : "btn-mute", nullptr);
    muteButton.setProperty ("style",
                            makeStyleStringForClasses (state.getTrack (trackIndex).muted ? "btn-mute-active" : "btn-mute",
                                                       stylesheet),
                            nullptr);
    muteButton.setProperty ("width", 52, nullptr);
    muteButton.setProperty ("height", 28, nullptr);

    auto muteText = juce::ValueTree ("Text");
    muteText.setProperty ("text", "Mute", nullptr);
    muteButton.appendChild (muteText, nullptr);

    buttonRow.appendChild (armButton, nullptr);
    buttonRow.appendChild (muteButton, nullptr);

    auto panLabel = juce::ValueTree ("Text");
    panLabel.setProperty ("id", "panLabel" + juce::String (trackIndex + 1), nullptr);
    panLabel.setProperty ("class", "label", nullptr);
    panLabel.setProperty ("style", labelStyle, nullptr);
    panLabel.setProperty ("text", "Pan: " + juce::String (state.getTrack (trackIndex).pan, 2), nullptr);

    auto panSlider = juce::ValueTree ("Slider");
    panSlider.setProperty ("id", "panSlider" + juce::String (trackIndex + 1), nullptr);
    panSlider.setProperty ("min", -1.0, nullptr);
    panSlider.setProperty ("max", 1.0, nullptr);
    panSlider.setProperty ("value", state.getTrack (trackIndex).pan, nullptr);
    panSlider.setProperty ("height", 32, nullptr);

    auto volumeLabel = juce::ValueTree ("Text");
    volumeLabel.setProperty ("id", "volumeLabel" + juce::String (trackIndex + 1), nullptr);
    volumeLabel.setProperty ("class", "label", nullptr);
    volumeLabel.setProperty ("style", labelStyle, nullptr);
    volumeLabel.setProperty ("text", "Vol: " + juce::String (state.getTrack (trackIndex).volumeDb, 1) + " dB", nullptr);

    auto volumeSlider = juce::ValueTree ("Slider");
    volumeSlider.setProperty ("id", "volumeSlider" + juce::String (trackIndex + 1), nullptr);
    volumeSlider.setProperty ("min", -60.0, nullptr);
    volumeSlider.setProperty ("max", 6.0, nullptr);
    volumeSlider.setProperty ("value", state.getTrack (trackIndex).volumeDb, nullptr);
    volumeSlider.setProperty ("height", 32, nullptr);

    auto meter = juce::ValueTree ("Component");
    meter.setProperty ("id", "trackMeter" + juce::String (trackIndex + 1), nullptr);
    meter.setProperty ("class", "meter", nullptr);
    meter.setProperty ("height", 8, nullptr);
    meter.setProperty ("style",
                       "{\"background\":\"rgba(255,255,255,0.1)\",\"border-radius\":4}",
                       nullptr);

    strip.appendChild (headerRow, nullptr);
    strip.appendChild (buttonRow, nullptr);
    strip.appendChild (panLabel, nullptr);
    strip.appendChild (panSlider, nullptr);
    strip.appendChild (volumeLabel, nullptr);
    strip.appendChild (volumeSlider, nullptr);
    strip.appendChild (meter, nullptr);

    return strip;
}

juce::ValueTree GUIComponent::createMasterStrip()
{
    const auto stripStyle = makeStyleStringForClasses ("laneRow-selected", stylesheet);
    const auto labelStyle = makeStyleStringForClasses ("label", stylesheet);

    auto strip = juce::ValueTree ("Component");
    strip.setProperty ("id", "masterRow", nullptr);
    strip.setProperty ("class", "laneRow-selected", nullptr);
    strip.setProperty ("style", stripStyle, nullptr);
    strip.setProperty ("display", "flex", nullptr);
    strip.setProperty ("flex-direction", "column", nullptr);
    strip.setProperty ("align-items", "stretch", nullptr);
    strip.setProperty ("row-gap", 10, nullptr);
    strip.setProperty ("padding", 10, nullptr);
    strip.setProperty ("min-height", 170, nullptr);

    auto label = juce::ValueTree ("Text");
    label.setProperty ("id", "masterLabel", nullptr);
    label.setProperty ("class", "label", nullptr);
    label.setProperty ("style", labelStyle, nullptr);
    label.setProperty ("text", "Master", nullptr);

    auto volumeLabel = juce::ValueTree ("Text");
    volumeLabel.setProperty ("id", "masterVolumeLabel", nullptr);
    volumeLabel.setProperty ("class", "label", nullptr);
    volumeLabel.setProperty ("style", labelStyle, nullptr);
    volumeLabel.setProperty ("text", "Vol: " + juce::String (state.masterVolumeDb, 1) + " dB", nullptr);

    auto volumeSlider = juce::ValueTree ("Slider");
    volumeSlider.setProperty ("id", "masterVolumeSlider", nullptr);
    volumeSlider.setProperty ("min", -60.0, nullptr);
    volumeSlider.setProperty ("max", 6.0, nullptr);
    volumeSlider.setProperty ("value", state.masterVolumeDb, nullptr);
    volumeSlider.setProperty ("height", 32, nullptr);

    auto meter = juce::ValueTree ("Component");
    meter.setProperty ("id", "masterMeter", nullptr);
    meter.setProperty ("class", "meter", nullptr);
    meter.setProperty ("height", 8, nullptr);
    meter.setProperty ("width", 0, nullptr);
    meter.setProperty ("style",
                   "{\"background\":\"#3a7afe\",\"border-radius\":4}",
                   nullptr);

    strip.appendChild (label, nullptr);
    strip.appendChild (volumeLabel, nullptr);
    strip.appendChild (volumeSlider, nullptr);
    strip.appendChild (meter, nullptr);

    return strip;
}

void GUIComponent::rebuildClipList()
{
    auto clipListNode = jive::findElementWithID (uiTree, juce::Identifier ("clipList"));
    if (! clipListNode.isValid())
        return;

    while (clipListNode.getNumChildren() > 0)
        clipListNode.removeChild (0, nullptr);

    const auto clipItemStyle = makeStyleStringForClasses ("clipItem", stylesheet);
    const auto labelStyle = makeStyleStringForClasses ("label", stylesheet);
    const auto secondaryStyle = makeStyleStringForClasses ("secondary-label", stylesheet);
    const auto buttonStyle = makeStyleStringForClasses ("btn", stylesheet);

    if (state.getNumTracks() > 0)
    {
        const int selectedIndex = juce::jlimit (0, state.getNumTracks() - 1, state.selectedTrackIndex);
        const auto& selectedTrack = state.getTrack (selectedIndex);

        auto trackLabel = juce::ValueTree ("Text");
        trackLabel.setProperty ("id", "pluginTrackLabel", nullptr);
        trackLabel.setProperty ("class", "subtle-label", nullptr);
        trackLabel.setProperty ("style", secondaryStyle, nullptr);
        trackLabel.setProperty ("text", selectedTrack.name, nullptr);
        clipListNode.appendChild (trackLabel, nullptr);

        auto pluginHeader = juce::ValueTree ("Text");
        pluginHeader.setProperty ("id", "pluginPanelHeader", nullptr);
        pluginHeader.setProperty ("class", "section-header", nullptr);
        pluginHeader.setProperty ("style", labelStyle, nullptr);
        pluginHeader.setProperty ("text", "Plugins", nullptr);
        clipListNode.appendChild (pluginHeader, nullptr);

        if (selectedTrack.plugins.isEmpty())
        {
            auto emptyPluginText = juce::ValueTree ("Text");
            emptyPluginText.setProperty ("id", "emptyPluginListLabel", nullptr);
            emptyPluginText.setProperty ("class", "secondary-label", nullptr);
            emptyPluginText.setProperty ("style", secondaryStyle, nullptr);
            emptyPluginText.setProperty ("text", "No plugins on this track", nullptr);
            clipListNode.appendChild (emptyPluginText, nullptr);
        }
        else
        {
            for (int i = 0; i < selectedTrack.plugins.size(); ++i)
            {
                auto pluginItem = juce::ValueTree ("Component");
                pluginItem.setProperty ("id", "pluginItem" + juce::String (i + 1), nullptr);
                pluginItem.setProperty ("class", "clipItem", nullptr);
                pluginItem.setProperty ("style", clipItemStyle, nullptr);
                pluginItem.setProperty ("display", "flex", nullptr);
                pluginItem.setProperty ("flex-direction", "column", nullptr);
                pluginItem.setProperty ("padding", 8, nullptr);
                pluginItem.setProperty ("row-gap", 3, nullptr);
                pluginItem.setProperty ("min-height", 52, nullptr);

                auto nameText = juce::ValueTree ("Text");
                nameText.setProperty ("id", "pluginName" + juce::String (i + 1), nullptr);
                nameText.setProperty ("class", "label", nullptr);
                nameText.setProperty ("style", labelStyle, nullptr);
                nameText.setProperty ("text", selectedTrack.plugins[i], nullptr);

                auto metaText = juce::ValueTree ("Text");
                metaText.setProperty ("id", "pluginMeta" + juce::String (i + 1), nullptr);
                metaText.setProperty ("class", "secondary-label", nullptr);
                metaText.setProperty ("style", secondaryStyle, nullptr);
                metaText.setProperty ("text", "Track effect slot " + juce::String (i + 1), nullptr);

                pluginItem.appendChild (nameText, nullptr);
                pluginItem.appendChild (metaText, nullptr);
                clipListNode.appendChild (pluginItem, nullptr);
            }
        }

        auto addPluginButton = juce::ValueTree ("Button");
        addPluginButton.setProperty ("id", "addPluginBtn", nullptr);
        addPluginButton.setProperty ("class", "btn", nullptr);
        addPluginButton.setProperty ("style", buttonStyle, nullptr);
        addPluginButton.setProperty ("height", 30, nullptr);

        auto addPluginText = juce::ValueTree ("Text");
        addPluginText.setProperty ("id", "addPluginBtnLabel", nullptr);
        addPluginText.setProperty ("text", "Add Plugin", nullptr);
        addPluginButton.appendChild (addPluginText, nullptr);

        clipListNode.appendChild (addPluginButton, nullptr);
    }

    auto spacer = juce::ValueTree ("Component");
    spacer.setProperty ("height", 16, nullptr);
    clipListNode.appendChild (spacer, nullptr);

    auto clipsHeader = juce::ValueTree ("Text");
    clipsHeader.setProperty ("id", "clipsSubHeader", nullptr);
    clipsHeader.setProperty ("class", "section-header", nullptr);
    clipsHeader.setProperty ("style", labelStyle, nullptr);
    clipsHeader.setProperty ("text", "Recent Clips", nullptr);
    clipListNode.appendChild (clipsHeader, nullptr);

    if (state.recordedClips.empty())
    {
        auto emptyClipText = juce::ValueTree ("Text");
        emptyClipText.setProperty ("id", "emptyClipListLabel", nullptr);
        emptyClipText.setProperty ("class", "secondary-label", nullptr);
        emptyClipText.setProperty ("style", secondaryStyle, nullptr);
        emptyClipText.setProperty ("text", "No recorded clips yet", nullptr);
        clipListNode.appendChild (emptyClipText, nullptr);
    }
    else
    {
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
            nameText.setProperty ("style", labelStyle, nullptr);
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
    }

    root->callLayoutChildrenWithRecursionLock();

    if (auto* addPluginItem = findGuiItemById (*root, juce::Identifier ("addPluginBtn")))
    {
        if (auto comp = addPluginItem->getComponent())
        {
            if (auto* button = dynamic_cast<juce::Button*> (comp.get()))
            {
                button->onClick = [this]
                {
                    if (state.getNumTracks() <= 0)
                        return;

                    auto& plugins = state.getTrack (state.selectedTrackIndex).plugins;

                    if (plugins.size() == 0)
                        plugins.add ("EQ");
                    else if (plugins.size() == 1)
                        plugins.add ("Compressor");
                    else if (plugins.size() == 2)
                        plugins.add ("Reverb");
                    else if (plugins.size() == 3)
                        plugins.add ("Delay");
                    else
                        plugins.add ("Plugin " + juce::String (plugins.size() + 1));

                    juce::MessageManager::callAsync ([this]
                    {
                        rebuildClipList();
                        refreshFromState();
                    });
                };
            }
        }
    }
}

void GUIComponent::timerCallback()
{
    state.tick (1.0 / 60.0);

    // Fake signal (later replaced with real audio levels)
    state.masterLevel = 0.3f + 0.2f * std::sin (juce::Time::getMillisecondCounterHiRes() * 0.002);

    auto meterNode = jive::findElementWithID (uiTree, juce::Identifier ("masterMeter"));
    if (meterNode.isValid())
    {
        meterNode.setProperty ("width", int (state.masterLevel * 200.0f), nullptr);
    }

    if (root != nullptr)
        root->callLayoutChildrenWithRecursionLock();

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