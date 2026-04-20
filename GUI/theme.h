#pragma once

#include <map>
#include <optional>

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>

class ThemeData
{
public:
    void loadFromStylesheet (const juce::var& stylesheet);
    juce::Colour colour (const juce::String& token, juce::Colour fallback = juce::Colours::transparentBlack) const;
    void setColour (const juce::String& token, juce::Colour colourValue);

    static std::optional<juce::Colour> parseColour (const juce::var& value);
    static juce::Image loadSpriteImage (const juce::String& fileName);

private:
    std::map<juce::String, juce::Colour> colours;
};
