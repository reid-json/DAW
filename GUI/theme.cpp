#include "theme.h"
#include <juce_graphics/juce_graphics.h>

juce::Image ThemeData::loadSpriteImage (const juce::String& fileName)
{
    auto dir = juce::File::getSpecialLocation (juce::File::currentExecutableFile).getParentDirectory();
    for (int i = 0; i < 4; ++i)
    {
        auto file = dir.getChildFile ("Resources").getChildFile ("ui")
                       .getChildFile ("sprites").getChildFile (fileName);
        if (file.existsAsFile())
            return juce::ImageFileFormat::loadFrom (file);
        dir = dir.getParentDirectory();
    }
    return {};
}

std::optional<juce::Colour> ThemeData::parseColour (const juce::var& value)
{
    if (! value.isString()) return std::nullopt;
    auto text = value.toString().trim();
    if (text.isEmpty()) return std::nullopt;

    if (text.startsWithChar ('#'))
    {
        auto hex = text.substring (1);
        if (hex.length() != 6 && hex.length() != 8) return std::nullopt;
        auto v = static_cast<juce::uint32> (hex.getHexValue32());
        if (hex.length() == 6)
            return juce::Colour ((juce::uint8)(v >> 16), (juce::uint8)(v >> 8), (juce::uint8) v);
        return juce::Colour ((juce::uint8)(v >> 16), (juce::uint8)(v >> 8), (juce::uint8) v, (juce::uint8)(v >> 24));
    }

    if (text.startsWithIgnoreCase ("rgb"))
    {
        const bool hasAlpha = text.startsWithIgnoreCase ("rgba");
        juce::StringArray parts;
        parts.addTokens (text.fromFirstOccurrenceOf ("(", false, false)
                             .upToLastOccurrenceOf (")", false, false), ",", "\"");
        parts.trim();
        if (parts.size() != (hasAlpha ? 4 : 3)) return std::nullopt;
        juce::Colour c ((juce::uint8) parts[0].getIntValue(),
                        (juce::uint8) parts[1].getIntValue(),
                        (juce::uint8) parts[2].getIntValue());
        if (! hasAlpha) return c;
        auto a = parts[3].containsChar ('.') ? parts[3].getFloatValue()
                                             : parts[3].getFloatValue() / 255.0f;
        return c.withAlpha (a);
    }

    if (text.equalsIgnoreCase ("white")) return juce::Colours::white;
    if (text.equalsIgnoreCase ("black")) return juce::Colours::black;
    if (text.equalsIgnoreCase ("transparent")) return juce::Colours::transparentBlack;
    return std::nullopt;
}

namespace
{
    void flatten (const juce::var& node, const juce::String& prefix,
                  std::map<juce::String, juce::Colour>& output)
    {
        if (auto c = ThemeData::parseColour (node))
        {
            if (prefix.isNotEmpty()) output[prefix] = *c;
            return;
        }

        if (auto* obj = node.isObject() ? node.getDynamicObject() : nullptr)
            for (auto& p : obj->getProperties())
                flatten (p.value, prefix.isEmpty() ? p.name.toString()
                                                   : prefix + "." + p.name.toString(), output);
    }
}

void ThemeData::loadFromStylesheet (const juce::var& stylesheet)
{
    colours.clear();
    auto* root = stylesheet.isObject() ? stylesheet.getDynamicObject() : nullptr;
    if (root == nullptr) return;
    flatten (root->getProperty ("theme"), {}, colours);
}

juce::Colour ThemeData::colour (const juce::String& token, juce::Colour fallback) const
{
    auto it = colours.find (token);
    return it != colours.end() ? it->second : fallback;
}

void ThemeData::setColour (const juce::String& token, juce::Colour colourValue)
{
    colours[token] = colourValue;
}
