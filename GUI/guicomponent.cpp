#include "guicomponent.h"
#include <juce_core/juce_core.h>

GUIComponent::GUIComponent()
{

    auto exeFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    

    auto resourcesDir = exeFile.getSiblingFile("Resources");
    if (!resourcesDir.exists())
        resourcesDir = exeFile.getParentDirectory();
    if (!resourcesDir.getChildFile("Resources").exists())
        resourcesDir = resourcesDir.getParentDirectory();
    if (!resourcesDir.getChildFile("Resources").exists())
        resourcesDir = resourcesDir.getParentDirectory();
    
    resourcesDir = resourcesDir.getChildFile("Resources");

    DBG("Resources Dir: " << resourcesDir.getFullPathName());

    auto xmlFile = resourcesDir.getChildFile("ui").getChildFile("views").getChildFile("main_view.xml");
    juce::ValueTree uiTree;

    if (xmlFile.existsAsFile())
    {
        if (auto xmlElement = juce::XmlDocument::parse(xmlFile))
        {
            uiTree = juce::ValueTree::fromXml(*xmlElement);
        }
        else
        {
            uiTree = juce::ValueTree("Component", {}, { juce::ValueTree("Label", { {"text", "Failed to parse XML"} }) });
        }
    }
    else
    {
        uiTree = juce::ValueTree("Component", {}, { juce::ValueTree("Label", { {"text", "Failed to find XML file: " + xmlFile.getFullPathName()} }) });
    }

    // --- APPLY STYLESHEET ---
    auto jsonStyleFile = resourcesDir.getChildFile("ui").getChildFile("styles").getChildFile("styles.json");

    if (jsonStyleFile.existsAsFile())
    {
        auto jsonTree = jive::parseJSON(jsonStyleFile.loadFileAsString());
        if (!jsonTree.isVoid())
        {
            uiTree.setProperty("style", jsonTree, nullptr);
        }
    }

    // --- CREATE JIVE COMPONENT TREE ---
    root = viewInterpreter.interpret(uiTree);
    
    if (root != nullptr)
    {
        if (auto comp = root->getComponent())
        {
            addAndMakeVisible(comp.get());
        }
        viewInterpreter.listenTo(*root);
    }

    setSize(1000, 700);
}

void GUIComponent::resized()
{
    if (root != nullptr)
    {
        if (auto comp = root->getComponent())
        {
            comp->setBounds(getLocalBounds());
        }
    }
}
