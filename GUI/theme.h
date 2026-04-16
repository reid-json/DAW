#pragma once

#include <map>

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>

class ThemeData
{
public:
    void loadFromStylesheet (const juce::var& stylesheet);
    juce::Colour colour (const juce::String& token, juce::Colour fallback = juce::Colours::transparentBlack) const;

private:
    std::map<juce::String, juce::Colour> colours;
};
