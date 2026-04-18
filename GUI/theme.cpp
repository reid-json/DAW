#include "theme.h"

namespace
{
    juce::DynamicObject* asObj (const juce::var& value)
    {
        return value.isObject() ? value.getDynamicObject() : nullptr;
    }

    juce::var getPathValue (const juce::var& root, const juce::String& path)
    {
        juce::var current = root;
        juce::StringArray parts;
        parts.addTokens (path, ".", "\"");
        parts.removeEmptyStrings();

        for (const auto& part : parts)
        {
            auto* object = asObj (current);
            if (object == nullptr)
                return {};

            current = object->getProperty (part);
        }

        return current;
    }

    std::optional<juce::Colour> parseHexColour (juce::String text)
    {
        text = text.trim();
        if (! text.startsWithChar ('#'))
            return std::nullopt;

        text = text.substring (1);
        if (text.length() == 6)
        {
            const auto rgb = static_cast<juce::uint32> (text.getHexValue32());
            return juce::Colour ((juce::uint8) ((rgb >> 16) & 0xff),
                                 (juce::uint8) ((rgb >> 8) & 0xff),
                                 (juce::uint8) (rgb & 0xff));
        }

        if (text.length() == 8)
        {
            const auto argb = static_cast<juce::uint32> (text.getHexValue32());
            return juce::Colour ((juce::uint8) ((argb >> 24) & 0xff),
                                 (juce::uint8) ((argb >> 16) & 0xff),
                                 (juce::uint8) ((argb >> 8) & 0xff),
                                 (juce::uint8) (argb & 0xff));
        }

        return std::nullopt;
    }

    std::optional<juce::Colour> parseRgbColour (juce::String text)
    {
        const auto open = text.indexOfChar ('(');
        const auto close = text.lastIndexOfChar (')');
        if (open < 0 || close <= open)
            return std::nullopt;

        const auto fn = text.substring (0, open).trim().toLowerCase();
        const bool hasAlpha = fn == "rgba";
        if (fn != "rgb" && ! hasAlpha)
            return std::nullopt;

        juce::StringArray parts;
        parts.addTokens (text.substring (open + 1, close), ",", "\"");
        parts.removeEmptyStrings();
        parts.trim();

        if (parts.size() != (hasAlpha ? 4 : 3))
            return std::nullopt;

        const auto red = (juce::uint8) juce::jlimit (0, 255, parts[0].getIntValue());
        const auto green = (juce::uint8) juce::jlimit (0, 255, parts[1].getIntValue());
        const auto blue = (juce::uint8) juce::jlimit (0, 255, parts[2].getIntValue());

        if (! hasAlpha)
            return juce::Colour (red, green, blue);

        const auto alphaText = parts[3].trim();
        const float alphaValue = alphaText.containsChar ('.')
            ? juce::jlimit (0.0f, 1.0f, alphaText.getFloatValue())
            : (float) juce::jlimit (0, 255, alphaText.getIntValue()) / 255.0f;

        return juce::Colour (red, green, blue).withAlpha (alphaValue);
    }

    std::optional<juce::Colour> parseColourValue (const juce::var& value)
    {
        if (! value.isString())
            return std::nullopt;

        const auto text = value.toString().trim();
        if (text.isEmpty())
            return std::nullopt;

        if (auto hex = parseHexColour (text))
            return hex;

        if (auto rgb = parseRgbColour (text))
            return rgb;

        if (text.equalsIgnoreCase ("white"))
            return juce::Colours::white;

        if (text.equalsIgnoreCase ("black"))
            return juce::Colours::black;

        if (text.equalsIgnoreCase ("transparent"))
            return juce::Colours::transparentBlack;

        return std::nullopt;
    }

    void flattenThemeColours (const juce::var& node,
                              const juce::String& prefix,
                              std::map<juce::String, juce::Colour>& output)
    {
        if (auto colour = parseColourValue (node))
        {
            if (prefix.isNotEmpty())
                output[prefix] = *colour;
            return;
        }

        auto* object = asObj (node);
        if (object == nullptr)
            return;

        for (auto& property : object->getProperties())
        {
            const auto childKey = prefix.isEmpty() ? property.name.toString()
                                                   : prefix + "." + property.name.toString();
            flattenThemeColours (property.value, childKey, output);
        }
    }
}

void ThemeData::loadFromStylesheet (const juce::var& stylesheet)
{
    colours.clear();
    flattenThemeColours (getPathValue (stylesheet, "theme"), {}, colours);
}

juce::Colour ThemeData::colour (const juce::String& token, juce::Colour fallback) const
{
    if (auto it = colours.find (token); it != colours.end())
        return it->second;

    return fallback;
}

void ThemeData::setColour (const juce::String& token, juce::Colour colourValue)
{
    colours[token] = colourValue;
}
