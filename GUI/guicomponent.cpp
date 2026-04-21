#include "guicomponent.h"

#include <juce_core/juce_core.h>
#include <jive_core/algorithms/jive_Find.h>
#include "timelinecomponent.h"
#include "sharedpopupmenulookandfeel.h"

namespace
{
    const char* themeSettingsKey = "activeTheme";

    struct ThemePresetDefinition
    {
        juce::String headerSpiceKey;
        juce::String bodySpiceKey;
        float accentHue = 0.0f;
    };

    juce::String getHeaderTooltipText (const juce::String& buttonId, const DAWState& state)
    {
        if (buttonId == "restartBtn")
            return "Restart playback from the beginning";

        if (buttonId == "playBtn")
            return "Start playback";

        if (buttonId == "pauseBtn")
            return state.transportState == DAWState::TransportState::paused
                ? "Resume playback"
                : "Pause playback";

        if (buttonId == "recordBtn")
            return state.isRecording
                ? "Stop recording"
                : "Start recording on the armed track";

        if (buttonId == "pianoRollBtn")
            return "Open the piano roll editor";

        if (buttonId == "settingsBtn")
            return "Open audio and app settings";

        return {};
    }

    juce::File getResourcesDir()
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

    juce::File getThemeSettingsFile()
    {
        auto settingsDir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                               .getChildFile ("DAW");
        settingsDir.createDirectory();
        return settingsDir.getChildFile ("theme.settings");
    }

    juce::String themePresetToString (GUIComponent::ThemePreset preset)
    {
        switch (preset)
        {
            case GUIComponent::ThemePreset::blue:   return "blue";
            case GUIComponent::ThemePreset::green:  return "green";
            case GUIComponent::ThemePreset::purple: return "purple";
            case GUIComponent::ThemePreset::orange:
            default:                                return "orange";
        }
    }

    GUIComponent::ThemePreset themePresetFromString (juce::String value)
    {
        value = value.trim().toLowerCase();
        if (value == "blue")
            return GUIComponent::ThemePreset::blue;
        if (value == "green")
            return GUIComponent::ThemePreset::green;
        if (value == "purple")
            return GUIComponent::ThemePreset::purple;
        return GUIComponent::ThemePreset::orange;
    }

    GUIComponent::ThemePreset loadSavedThemePreset()
    {
        juce::PropertiesFile settings (getThemeSettingsFile(), juce::PropertiesFile::Options {});
        return themePresetFromString (settings.getValue (themeSettingsKey, "orange"));
    }

    void saveThemePreset (GUIComponent::ThemePreset preset)
    {
        juce::PropertiesFile settings (getThemeSettingsFile(), juce::PropertiesFile::Options {});
        settings.setValue (themeSettingsKey, themePresetToString (preset));
        settings.saveIfNeeded();
    }

    juce::var parseJsonFile (const juce::File& file)
    {
        return file.existsAsFile() ? juce::JSON::parse (file.loadFileAsString()) : juce::var {};
    }

    juce::DynamicObject* asObj (const juce::var& v)
    {
        return v.isObject() ? v.getDynamicObject() : nullptr;
    }

    juce::var cloneVar (const juce::var& value)
    {
        return juce::JSON::parse (juce::JSON::toString (value, false));
    }

    bool isAccentColour (juce::Colour colour)
    {
        float h = 0.0f, s = 0.0f, v = 0.0f;
        colour.getHSB (h, s, v);
        return s > 0.18f && v > 0.15f && ((h >= 0.02f && h <= 0.16f) || h >= 0.96f);
    }

    void shiftAccentHue (juce::var& node, float targetHue, const juce::String& path = {})
    {
        if (path.startsWith ("theme.ui.button-record")
         || path.startsWith ("theme.tracklist.arm")
         || path.startsWith ("theme.arrangement.clip.delete-background"))
            return;

        if (node.isString())
        {
            auto colour = ThemeData::parseColour (node);
            if (colour && isAccentColour (*colour))
            {
                float h = 0.0f, s = 0.0f, v = 0.0f;
                colour->getHSB (h, s, v);
                auto shifted = juce::Colour::fromHSV (targetHue, s, v, colour->getFloatAlpha());
                if (node.toString().startsWithIgnoreCase ("rgba"))
                    node = "rgba(" + juce::String ((int) shifted.getRed()) + ","
                                   + juce::String ((int) shifted.getGreen()) + ","
                                   + juce::String ((int) shifted.getBlue()) + ","
                                   + juce::String (shifted.getFloatAlpha(), 3) + ")";
                else
                    node = "#" + shifted.toDisplayString (colour->getFloatAlpha() < 0.999f);
            }
            return;
        }

        if (auto* object = asObj (node))
            for (auto& property : object->getProperties())
            {
                auto value = property.value;
                shiftAccentHue (value, targetHue,
                                       path.isEmpty() ? property.name.toString()
                                                      : path + "." + property.name.toString());
                object->setProperty (property.name, value);
            }
    }

    ThemePresetDefinition getThemePresetDefinition (GUIComponent::ThemePreset preset)
    {
        switch (preset)
        {
            case GUIComponent::ThemePreset::blue:
                return { "headerspiceblue", "bodyspiceblue", juce::Colour (0xff2f6fe6).getHue() };
            case GUIComponent::ThemePreset::purple:
                return { "headerspicepurple", "bodyspicepurple", juce::Colour (0xff8b5cf6).getHue() };
            case GUIComponent::ThemePreset::green:
                return { "headerspicegreen", "bodyspicegreen", juce::Colour (0xff22c55e).getHue() };
            case GUIComponent::ThemePreset::orange:
            default:
                return { "headerspice", "bodyspice", juce::Colour (0xffe68000).getHue() };
        }
    }

    juce::var resolveThemeReference (const juce::var& value, const juce::var& stylesheet)
    {
        if (! value.isString())
            return value;

        const auto text = value.toString().trim();
        if (! text.startsWith ("$theme."))
            return value;

        juce::var current = stylesheet;
        juce::StringArray path;
        path.addTokens (text.fromFirstOccurrenceOf ("$", false, false), ".", "\"");
        path.removeEmptyStrings();

        for (const auto& segment : path)
        {
            auto* object = asObj (current);
            if (object == nullptr)
                return value;

            current = object->getProperty (segment);
        }

        return current.isVoid() ? value : current;
    }

    void mergeProps (juce::DynamicObject& target, const juce::var& src, const juce::var& stylesheet)
    {
        if (auto* obj = asObj (src))
            for (auto& p : obj->getProperties())
                target.setProperty (p.name, resolveThemeReference (p.value, stylesheet));
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
                mergeProps (*merged.getDynamicObject(), sheet->getProperty ("." + cls), stylesheet);
        return juce::JSON::toString (merged, false);
    }

    void applyStyles (juce::ValueTree node, const juce::var& stylesheet)
    {
        auto* sheet = asObj (stylesheet);
        if (sheet == nullptr) return;

        auto merged = juce::var (new juce::DynamicObject());
        auto* obj = merged.getDynamicObject();

        for (auto& cls : splitClasses (node.getProperty ("class").toString()))
            mergeProps (*obj, sheet->getProperty ("." + cls), stylesheet);

        auto id = node.getProperty ("id").toString();
        if (id.isNotEmpty())
            mergeProps (*obj, sheet->getProperty ("#" + id), stylesheet);

        mergeProps (*obj, parseStyleString (node.getProperty ("style").toString()), stylesheet);
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
                auto img = juce::ImageFileFormat::loadFrom (file);
                if (img.isValid())
                    sprites[key] = juce::VariantConverter<juce::Image>::toVar (img);
            }
        }
        return sprites;
    }

    const juce::var* findSprite (const juce::StringArray& keys, const SpriteAssetMap& sprites)
    {
        for (auto& key : keys)
        {
            auto it = sprites.find (key);
            if (it != sprites.end())
                return &it->second;
        }
        for (auto& key : keys)
            for (auto& entry : sprites)
                if (entry.first.startsWith (key) || key.startsWith (entry.first))
                    return &entry.second;
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

class GUIComponent::HeaderButtonOverlay : public juce::Component,
                                          public juce::SettableTooltipClient
{
public:
    HeaderButtonOverlay (ThemeData& themeIn, juce::String buttonIdIn)
        : theme (themeIn), buttonId (std::move (buttonIdIn))
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void setTargetButton (juce::Button* button)
    {
        targetButton = button;
        if (targetButton != nullptr)
            setEnabled (targetButton->isEnabled());
    }

    void setText (juce::String newText)
    {
        text = std::move (newText);
        repaint();
    }

    void setTooltipText (juce::String newTooltip)
    {
        setTooltip (std::move (newTooltip));
    }

    void setSprite (const juce::Image& newSprite)
    {
        sprite = newSprite;
        repaint();
    }

    void setRecordActive (bool shouldBeActive)
    {
        recordActive = shouldBeActive;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        if (bounds.isEmpty())
            return;

        const auto baseFill = getBaseFillColour();
        auto fill = baseFill;
        if (pressed)
            fill = fill.darker (0.34f);
        else if (hovered)
            fill = fill.brighter (0.24f);

        const bool isFileButton = buttonId == "fileBtn";
        const float ringThickness = isFileButton ? 2.5f : 3.0f;
        const float corner = isFileButton ? 10.0f : bounds.getHeight() * 0.5f;

        g.setColour (juce::Colours::white.withAlpha (0.98f));
        g.fillRoundedRectangle (bounds.reduced (0.5f), corner);

        auto innerBounds = bounds.reduced (ringThickness);
        g.setColour (fill);
        g.fillRoundedRectangle (innerBounds, juce::jmax (0.0f, corner - ringThickness));

        if (hovered && ! pressed)
        {
            g.setColour (juce::Colours::white.withAlpha (0.14f));
            g.fillRoundedRectangle (innerBounds.reduced (1.0f),
                                    juce::jmax (0.0f, corner - ringThickness - 1.0f));
        }

        if (sprite.isValid() && buttonId != "fileBtn")
        {
            if (pressed || hovered)
            {
                g.saveState();
                auto clipPath = juce::Path();
                clipPath.addRoundedRectangle (innerBounds, juce::jmax (0.0f, corner - ringThickness));
                g.reduceClipRegion (clipPath);
                g.setColour (baseFill);
                g.fillRoundedRectangle (innerBounds, juce::jmax (0.0f, corner - ringThickness));
                g.restoreState();

                if (hovered && ! pressed)
                {
                    g.setColour (juce::Colours::white.withAlpha (0.14f));
                    g.fillRoundedRectangle (innerBounds.reduced (1.0f),
                                            juce::jmax (0.0f, corner - ringThickness - 1.0f));
                }
            }

            const auto isSettingsButton = buttonId == "settingsBtn";
            const float insetScale = isSettingsButton ? 0.08f : 0.12f;
            const auto spriteBounds = innerBounds.reduced (innerBounds.getWidth() * insetScale,
                                                           innerBounds.getHeight() * insetScale);
            g.drawImageWithin (sprite,
                               juce::roundToInt (spriteBounds.getX()),
                               juce::roundToInt (spriteBounds.getY()),
                               juce::roundToInt (spriteBounds.getWidth()),
                               juce::roundToInt (spriteBounds.getHeight()),
                               juce::RectanglePlacement::centred,
                               false);
            return;
        }

        g.setColour (theme.colour ("ui.button.foreground", juce::Colours::white));
        g.setFont (juce::Font (13.0f, juce::Font::bold));
        g.drawText (text, getLocalBounds(), juce::Justification::centred, false);
    }

    void mouseEnter (const juce::MouseEvent&) override
    {
        hovered = true;
        repaint();
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        hovered = false;
        pressed = false;
        repaint();
    }

    void mouseDown (const juce::MouseEvent&) override
    {
        pressed = true;
        repaint();
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        const bool shouldTrigger = pressed && getLocalBounds().contains (e.getPosition());
        pressed = false;
        hovered = getLocalBounds().contains (e.getPosition());
        repaint();

        if (shouldTrigger && targetButton != nullptr)
            targetButton->triggerClick();
    }

private:
    juce::Colour getBaseFillColour() const
    {
        if (buttonId == "recordBtn")
        {
            const auto token = recordActive ? "ui.button-record-active.background"
                                            : "ui.button-record.background";
            const auto fallback = recordActive ? juce::Colour (0xffff4d4f)
                                               : juce::Colour (0xffb42318);
            return theme.colour (token, fallback);
        }

        return theme.colour ("ui.button.background", juce::Colour (0xffe68000));
    }

    ThemeData& theme;
    juce::String buttonId;
    juce::String text;
    juce::Image sprite;
    juce::Button* targetButton = nullptr;
    bool hovered = false;
    bool pressed = false;
    bool recordActive = false;
};

void GUIComponent::registerCustomComponentTypes()
{
    viewInterpreter.getComponentFactory().set ("TrackListView", [this]
    {
        auto component = std::make_unique<TrackListComponent> (state, themeData);
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
        auto component = std::make_unique<TimelineComponent> (state, themeData);
        timelineComponent = component.get();
        return component;
    });

    viewInterpreter.getComponentFactory().set ("ArrangementView", [this]
    {
        auto component = std::make_unique<ArrangementComponent> (state, themeData);

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
        component->onAssetRenameRequested = [this] (int assetId, const juce::String& newName)
        {
            if (onAssetRenameRequested)
                onAssetRenameRequested (assetId, newName);
        };
        component->onPatternEditRequested = [this] (int assetId)
        {
            if (onPatternEditRequested)
                onPatternEditRequested (assetId);
        };

        arrangementComponent = component.get();
        return component;
    });

    viewInterpreter.getComponentFactory().set ("RecentClipsView", [this]
    {
        auto component = std::make_unique<RecentClipsComponent> (state, themeData);
        component->onAssetRenameRequested = [this] (int assetId, const juce::String& newName)
        {
            if (onAssetRenameRequested)
                onAssetRenameRequested (assetId, newName);
        };
        component->onPatternEditRequested = [this] (int assetId)
        {
            if (onPatternEditRequested)
                onPatternEditRequested (assetId);
        };
        recentClipsComponent = component.get();
        return component;
    });

    viewInterpreter.getComponentFactory().set ("TempoControlView", [this]
    {
        auto component = std::make_unique<TempoControlComponent> (state, themeData);
        tempoControlComponent = component.get();
        return component;
    });

}

GUIComponent::GUIComponent(juce::AudioDeviceManager& sharedDeviceManager)
    : deviceManager(sharedDeviceManager)
{
    setLookAndFeel (&lookAndFeel);

    const auto resourcesDir = getResourcesDir();
    const auto spriteDir = resourcesDir.getChildFile ("ui")
                                       .getChildFile ("sprites");

    const auto xmlFile = resourcesDir.getChildFile ("ui")
                                     .getChildFile ("views")
                                     .getChildFile ("main_view.xml");

    const auto jsonStyleFile = resourcesDir.getChildFile ("ui")
                                           .getChildFile ("styles")
                                           .getChildFile ("styles.json");

    auto xml = juce::XmlDocument (xmlFile).getDocumentElement();
    uiTree = juce::ValueTree::fromXml (*xml);

    spriteAssets = loadSprites (spriteDir);
    replaceLabelsWithSprites (uiTree, spriteAssets);

    auto headerSpiceNode = jive::findElementWithID (uiTree, juce::Identifier ("headerSpice"));
    if (headerSpiceNode.isValid())
    {
        if (auto* spice = findSprite (juce::StringArray { "headerspice" }, spriteAssets))
            headerSpiceNode.setProperty ("source", *spice, nullptr);
    }

    auto headerLogoNode = jive::findElementWithID (uiTree, juce::Identifier ("headerLogo"));
    if (headerLogoNode.isValid())
    {
        if (auto* logo = findSprite (juce::StringArray { "chromalogotransparent", "chromalogo" }, spriteAssets))
            headerLogoNode.setProperty ("source", *logo, nullptr);
    }

    auto bodySpiceSidebarNode = jive::findElementWithID (uiTree, juce::Identifier ("bodySpiceSidebar"));
    if (bodySpiceSidebarNode.isValid())
    {
        if (auto* spice = findSprite (juce::StringArray { "bodyspice" }, spriteAssets))
            bodySpiceSidebarNode.setProperty ("source", *spice, nullptr);
    }

    baseStylesheet = parseJsonFile (jsonStyleFile);
    stylesheet = cloneVar (baseStylesheet);
    themeData.loadFromStylesheet (stylesheet);
    fileMenuLookAndFeel = std::make_unique<SharedPopupMenuLookAndFeel> (themeData);
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
        createHeaderButtonOverlays();
    }

    currentTheme = loadSavedThemePreset();
    applyThemePreset (currentTheme);
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

    auto rootNode = jive::findElementWithID (uiTree, juce::Identifier ("root"));
    if (rootNode.isValid())
    {
        rootNode.setProperty ("width",  getWidth(),  nullptr);
        rootNode.setProperty ("height", getHeight(), nullptr);
    }

    if (auto comp = root->getComponent())
        comp->setBounds (getLocalBounds());

    root->callLayoutChildrenWithRecursionLock();
    layoutBody();
    updateHeaderButtonOverlayBounds();
}

void GUIComponent::paintOverChildren(juce::Graphics& g)
{
    g.setColour(juce::Colours::white.withAlpha(0.75f));
    g.fillRect(0, 98, getWidth(), 1);
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

juce::Component* GUIComponent::getCompById (const juce::String& id)
{
    if (root == nullptr) return nullptr;
    if (auto* item = findGuiItemById (*root, juce::Identifier (id)))
        if (auto comp = item->getComponent())
            return comp.get();
    return nullptr;
}

juce::Button* GUIComponent::getBtnById (const juce::String& id)
{
    return dynamic_cast<juce::Button*> (getCompById (id));
}

void GUIComponent::layoutBody()
{
    if (root == nullptr) return;

    auto setBoundsById = [this] (const juce::String& id, juce::Rectangle<int> bounds)
    {
        if (auto* comp = getCompById (id))
            comp->setBounds (bounds);
    };

    auto* headerComp    = getCompById ("headerContent");
    auto* bodyComp      = getCompById ("bodyContent");
    auto* workspaceComp = getCompById ("workspaceContent");
    auto* sidebarComp   = getCompById ("clipSidebarContent");
    if (! headerComp || ! bodyComp || ! workspaceComp || ! sidebarComp) return;

    constexpr int stripW = 210, masterH = 100;

    auto area = getLocalBounds();
    headerComp->setBounds (area.removeFromTop (98));
    bodyComp->setBounds (area);

    setBoundsById ("headerSpice", headerComp->getLocalBounds());
    auto headerInnerBounds = headerComp->getLocalBounds().reduced (10, 0);
    setBoundsById ("headerMainRow", headerInnerBounds.removeFromTop (90));

    if (auto* transportComp = getCompById ("transportGroup"))
    {
        auto bounds = transportComp->getBounds();
        bounds.setX (headerComp->getX() + (headerComp->getWidth() - bounds.getWidth()) / 2);
        transportComp->setBounds (bounds);
    }

    if (auto* transportComp = getCompById ("transportGroup"))
        if (auto* settingsComp = getCompById ("settingsBtn"))
            if (auto* tempoComp = getCompById ("tempoControlView"))
            {
                constexpr int gap = 16;
                constexpr int sideInset = 28;
                const int tempoX = transportComp->getRight() + gap + sideInset;
                const int maxTempoRight = settingsComp->getX() - gap - sideInset;
                const int availableWidth = juce::jmax (120, maxTempoRight - tempoX);
                const int tempoHeight = 38;

                auto tempoBounds = juce::Rectangle<int> (tempoX,
                                                         transportComp->getY() + ((transportComp->getHeight() - tempoHeight) / 2),
                                                         availableWidth,
                                                         tempoHeight);
                tempoComp->setBounds (tempoBounds);
            }

    int sideW = state.clipSidebarCollapsed ? 44 : state.clipSidebarWidth;
    auto bodyArea = bodyComp->getLocalBounds();
    sidebarComp->setBounds (bodyArea.removeFromRight (sideW));
    workspaceComp->setBounds (bodyArea);

    auto wsArea = workspaceComp->getLocalBounds();
    setBoundsById ("trackList",        wsArea.removeFromLeft (stripW));
    setBoundsById ("timelinePanel",    wsArea.removeFromTop (masterH));
    setBoundsById ("arrangementPanel", wsArea);

    setBoundsById ("bodySpiceSidebar",
                   juce::Rectangle<int> (0,
                                         juce::roundToInt (sidebarComp->getHeight() * 0.28f),
                                         sidebarComp->getWidth(),
                                         sidebarComp->getHeight()));

    auto clipArea = sidebarComp->getLocalBounds().reduced (10);
    setBoundsById ("clipBrowserTitle", clipArea.removeFromTop (30));
    clipArea.removeFromTop (4);
    setBoundsById ("clipList", clipArea);

    updateHeaderButtonOverlayBounds();
}

void GUIComponent::createHeaderButtonOverlays()
{
    for (const auto& id : { juce::String ("fileBtn"),
                            juce::String ("themesBtn"),
                            juce::String ("restartBtn"),
                            juce::String ("playBtn"),
                            juce::String ("pauseBtn"),
                            juce::String ("recordBtn"),
                            juce::String ("pianoRollBtn"),
                            juce::String ("settingsBtn") })
    {
        auto* button = getBtnById (id);
        if (button == nullptr)
            continue;

        auto overlay = std::make_unique<HeaderButtonOverlay> (themeData, id);
        overlay->setTargetButton (button);

        auto node = jive::findElementWithID (uiTree, juce::Identifier (id));
        if (node.isValid() && id != "fileBtn" && id != "themesBtn")
            if (const auto* sprite = findSprite (buildSpriteKeys (node), spriteAssets))
                if (! sprite->isVoid())
                    overlay->setSprite (juce::VariantConverter<juce::Image>::fromVar (*sprite));

        if (node.isValid() && (id == "fileBtn" || id == "themesBtn"))
        {
            for (int i = 0; i < node.getNumChildren(); ++i)
            {
                auto child = node.getChild (i);
                if (child.getType().toString().compareIgnoreCase ("Text") == 0)
                {
                    overlay->setText (child.getProperty ("text").toString());
                    break;
                }
            }
        }

        if (id == "themesBtn")
            overlay->setText ("Themes");

        addAndMakeVisible (*overlay);
        overlay->toFront (false);
        headerButtonOverlays[id] = std::move (overlay);

        button->setAlpha (0.0f);
        button->setInterceptsMouseClicks (false, false);
        enableClickThrough (*button);
    }

    refreshHeaderButtonTooltips();
}

void GUIComponent::updateHeaderButtonOverlayBounds()
{
    for (auto& entry : headerButtonOverlays)
    {
        auto* button = getBtnById (entry.first);
        if (button == nullptr || entry.second == nullptr)
            continue;

        entry.second->setBounds (getLocalArea (button->getParentComponent(), button->getBounds()));
        entry.second->toFront (false);
    }
}

void GUIComponent::refreshHeaderButtonTooltips()
{
    for (const auto& id : { juce::String ("restartBtn"),
                            juce::String ("playBtn"),
                            juce::String ("pauseBtn"),
                            juce::String ("recordBtn"),
                            juce::String ("pianoRollBtn"),
                            juce::String ("settingsBtn") })
    {
        const auto tooltip = getHeaderTooltipText (id, state);

        if (auto* button = getBtnById (id))
            button->setTooltip (tooltip);

        auto it = headerButtonOverlays.find (id);
        if (it != headerButtonOverlays.end() && it->second != nullptr)
            it->second->setTooltipText (tooltip);
    }
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
        openPianoRollForPattern (getSelectedTrackPatternNotes(), getSelectedTrackPatternAssetId());
    });

    bindBtn ("fileBtn", [this]
    {
        juce::PopupMenu menu;
        menu.setLookAndFeel (fileMenuLookAndFeel.get());
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

    bindBtn ("themesBtn", [this]
    {
        juce::PopupMenu menu;
        menu.setLookAndFeel (fileMenuLookAndFeel.get());
        menu.addItem (1, "Orange", true, currentTheme == ThemePreset::orange);
        menu.addItem (2, "Blue", true, currentTheme == ThemePreset::blue);
        menu.addItem (3, "Green", true, currentTheme == ThemePreset::green);
        menu.addItem (4, "Purple", true, currentTheme == ThemePreset::purple);

        menu.showMenuAsync (juce::PopupMenu::Options(), [this] (int result)
        {
            if (result == 1)
                applyThemePreset (ThemePreset::orange);
            else if (result == 2)
                applyThemePreset (ThemePreset::blue);
            else if (result == 3)
                applyThemePreset (ThemePreset::green);
            else if (result == 4)
                applyThemePreset (ThemePreset::purple);
        });
    });
}

void GUIComponent::applyThemePreset (ThemePreset preset)
{
    currentTheme = preset;
    saveThemePreset (preset);

    stylesheet = cloneVar (baseStylesheet);
    if (preset != ThemePreset::orange)
        shiftAccentHue (stylesheet, getThemePresetDefinition (preset).accentHue);

    themeData.loadFromStylesheet (stylesheet);

    juce::Colour primaryAccent (0xffe68000);
    juce::Colour deepAccent (0xffc34723);
    if (preset == ThemePreset::blue)   { primaryAccent = juce::Colour (0xff4c88ff); deepAccent = juce::Colour (0xff4258b8); }
    if (preset == ThemePreset::green)  { primaryAccent = juce::Colour (0xff22c55e); deepAccent = juce::Colour (0xff1f8a46); }
    if (preset == ThemePreset::purple) { primaryAccent = juce::Colour (0xff8b5cf6); deepAccent = juce::Colour (0xff5b3aa8); }

    const char* primaryTokens[] = {
        "ui.button.background", "ui.button-primary.background", "ui.button-mixer-add.background",
        "ui.button-monitor-off.background", "ui.button-monitor-on.background", "ui.button-danger.background",
        "ui.lane-accent.background", "arrangement.clip.fill", "arrangement.scrollbar.thumb",
        "tracklist.remove-button.background", "tracklist.pill-off", "tracklist.mute", "tracklist.fx",
        "tracklist.input", "tracklist.routing.background", "tracklist.fader.fill-active",
        "tracklist.pan.ring-active", "tracklist.pan.ring-inactive", "tracklist.add-track.background",
        "recent-clips.clip-item.background", "recent-clips.pattern-item.background", "recent-clips.scrollbar.thumb",
        "tempo.slider.track", "tempo.value.background", "tracklist.inline-editor.outline"
    };
    for (const auto* token : primaryTokens)
        themeData.setColour (token, primaryAccent);

    themeData.setColour ("tracklist.panel", deepAccent);
    themeData.setColour ("tracklist.master-panel", deepAccent);
    themeData.setColour ("tracklist.fader.fill-inactive", primaryAccent.withAlpha (0.32f));
    themeData.setColour ("arrangement.scrollbar.track", primaryAccent.withAlpha (0.22f));
    themeData.setColour ("recent-clips.scrollbar.track", primaryAccent.withAlpha (0.22f));

    const auto presetDef = getThemePresetDefinition (preset);
    const auto getSpriteImage = [this] (const juce::String& key) -> juce::Image
    {
        const auto* sprite = findSprite (juce::StringArray { key }, spriteAssets);
        if (sprite != nullptr && ! sprite->isVoid())
            return juce::VariantConverter<juce::Image>::fromVar (*sprite);
        return {};
    };

    auto headerSpiceNode = jive::findElementWithID (uiTree, juce::Identifier ("headerSpice"));
    if (headerSpiceNode.isValid())
    {
        if (const auto* spice = findSprite (juce::StringArray { presetDef.headerSpiceKey }, spriteAssets))
            headerSpiceNode.setProperty ("source", *spice, nullptr);
    }

    auto bodySpiceNode = jive::findElementWithID (uiTree, juce::Identifier ("bodySpiceSidebar"));
    if (bodySpiceNode.isValid())
    {
        if (const auto* spice = findSprite (juce::StringArray { presetDef.bodySpiceKey }, spriteAssets))
            bodySpiceNode.setProperty ("source", *spice, nullptr);
    }

    const auto bodySpiceImage = getSpriteImage (presetDef.bodySpiceKey);
    const auto headerSpiceImage = getSpriteImage (presetDef.headerSpiceKey);

    if (trackListComponent != nullptr)   trackListComponent->setBodySpiceImage (bodySpiceImage);
    if (arrangementComponent != nullptr) arrangementComponent->setBodySpiceImage (bodySpiceImage);
    if (settingsWindow != nullptr)       settingsWindow->setAccentColour (primaryAccent);
    if (pianoRollWindow != nullptr)      pianoRollWindow->content->setThemeAssets (headerSpiceImage, bodySpiceImage, primaryAccent);
    pluginHostManager.setBuiltInPluginTheme (primaryAccent, bodySpiceImage);

    root->callLayoutChildrenWithRecursionLock();
    layoutBody();
    refreshFromState();
    repaintDynamicViews();
    repaint();
}

void GUIComponent::openSettingsWindow()
{
    if (settingsWindow == nullptr)
        settingsWindow = std::make_unique<SettingsWindow> (deviceManager);

    settingsWindow->setAccentColour (themeData.colour ("ui.button.background", juce::Colour (0xffe68000)));

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
    }

    inputs.removeDuplicates(false);
    return inputs;
}

void GUIComponent::openPianoRollForPattern(std::vector<PianoRoll::Note> notes, int assetId)
{
    ensurePianoRollWindow();
    currentPianoRollAssetId = assetId;
    pianoRollWindow->content->setNotes(std::move(notes));
    pianoRollWindow->setVisible(true);
    pianoRollWindow->toFront(true);
}

std::vector<PianoRoll::Note> GUIComponent::getSelectedTrackPatternNotes() const
{
    if (state.isMasterMixerFocused())
        return {};

    const auto trackIndex = juce::jlimit (0, state.trackCount - 1, state.selectedTrackIndex);
    const auto& mixerState = state.getTrackMixerState (trackIndex);
    if (mixerState.contentType != TrackMixerState::ContentType::pattern)
        return {};

    const auto& patternState = state.getTrackPatternState (trackIndex);
    std::vector<PianoRoll::Note> notes;
    notes.reserve (patternState.notes.size());

    for (const auto& note : patternState.notes)
        notes.push_back ({ note.id, note.startBeat, note.lengthBeats, note.midiNote, note.label });

    return notes;
}

int GUIComponent::getSelectedTrackPatternAssetId() const
{
    if (state.isMasterMixerFocused())
        return -1;

    const auto trackIndex = juce::jlimit (0, state.trackCount - 1, state.selectedTrackIndex);
    const auto& mixerState = state.getTrackMixerState (trackIndex);
    if (mixerState.contentType != TrackMixerState::ContentType::pattern)
        return -1;

    return state.getTrackPatternState(trackIndex).assetId;
}

void GUIComponent::ensurePianoRollWindow()
{
    if (pianoRollWindow != nullptr)
        return;

    pianoRollWindow = std::make_unique<PianoRollWindow>();
    auto* pr = pianoRollWindow->content;
    pr->setOnSavePattern ([this] (const std::vector<PianoRoll::Note>& notes)
    {
        if (onSavePatternRequested)
            onSavePatternRequested (currentPianoRollAssetId, notes);
    });
    pr->setOnGetAvailableInstruments ([this]
    {
        return pluginHostManager.getAvailableTrackInstrumentPlugins();
    });
    pr->setOnInstrumentChanged ([this] (const juce::String& name)
    {
        if (onPianoRollInstrumentChangeRequested)
            onPianoRollInstrumentChangeRequested (name);
    });

    auto currentInst = pluginHostManager.getPianoRollInstrumentName();
    if (currentInst.isNotEmpty())
        pr->setInstrumentName (currentInst);

    const auto presetDef = getThemePresetDefinition (currentTheme);
    if (const auto* headerSpice = findSprite (juce::StringArray { presetDef.headerSpiceKey }, spriteAssets))
        if (const auto* bodySpice = findSprite (juce::StringArray { presetDef.bodySpiceKey }, spriteAssets))
            if (! headerSpice->isVoid() && ! bodySpice->isVoid())
                pr->setThemeAssets (juce::VariantConverter<juce::Image>::fromVar (*headerSpice),
                                    juce::VariantConverter<juce::Image>::fromVar (*bodySpice),
                                    themeData.colour ("ui.button.background", juce::Colour (0xffe68000)));
}

void GUIComponent::refreshFromState()
{
    if (root == nullptr) return;

    auto setPropertyById = [this] (const juce::String& id, const juce::Identifier& prop, const juce::var& value)
    {
        auto node = jive::findElementWithID (uiTree, juce::Identifier (id));
        if (node.isValid()) node.setProperty (prop, value, nullptr);
    };

    auto setTextById = [&] (const juce::String& id, const juce::String& text)
    {
        setPropertyById (id, "text", text);
    };

    auto setClassById = [&] (const juce::String& id, const juce::String& className)
    {
        setPropertyById (id, "class", className);
        setPropertyById (id, "style", buildStyleForClasses (className, stylesheet));
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
    {
        setPropertyById ("recordBtnSprite", juce::Identifier ("source"), *recordSprite);
        auto it = headerButtonOverlays.find ("recordBtn");
        if (it != headerButtonOverlays.end() && ! recordSprite->isVoid())
            it->second->setSprite (juce::VariantConverter<juce::Image>::fromVar (*recordSprite));
    }

    auto recordBtnOverlay = headerButtonOverlays.find ("recordBtn");
    if (recordBtnOverlay != headerButtonOverlays.end())
        recordBtnOverlay->second->setRecordActive (state.isRecording);

    refreshHeaderButtonTooltips();
    
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

    if (auto* item = findGuiItemById (*root, juce::Identifier ("clipBrowserTitle")))
        if (auto comp = item->getComponent())
            if (auto* label = dynamic_cast<juce::Label*> (comp.get()))
            {
                label->setFont (juce::Font (16.0f, juce::Font::bold));
                label->setColour (juce::Label::textColourId, juce::Colours::white);
                label->setJustificationType (juce::Justification::centredLeft);
            }

    root->callLayoutChildrenWithRecursionLock();

    if (tempoControlComponent != nullptr)
        tempoControlComponent->refreshFromState();

    repaint();
}

void GUIComponent::syncFromEngine(bool shouldRefreshControls, bool /*shouldRebuildTrackList*/)
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

