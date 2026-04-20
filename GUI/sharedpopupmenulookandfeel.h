#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "theme.h"

class SharedPopupMenuLookAndFeel : public juce::LookAndFeel_V4
{
public:
    explicit SharedPopupMenuLookAndFeel (ThemeData& themeIn) : theme (themeIn) {}

    void drawPopupMenuBackground (juce::Graphics& g, int width, int height) override
    {
        g.fillAll (theme.colour ("ui.header.background", juce::Colour (0xff18181b)));
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.drawRect (0, 0, width, height, 1);
    }

    void drawPopupMenuItem (juce::Graphics& g,
                            const juce::Rectangle<int>& area,
                            bool isSeparator,
                            bool isActive,
                            bool isHighlighted,
                            bool isTicked,
                            bool,
                            const juce::String& text,
                            const juce::String&,
                            const juce::Drawable*,
                            const juce::Colour*) override
    {
        if (isSeparator)
        {
            g.setColour (juce::Colours::white.withAlpha (0.14f));
            g.fillRect (area.reduced (10, area.getHeight() / 2).withHeight (1));
            return;
        }

        auto itemArea = area.reduced (4, 2).toFloat();
        if (isHighlighted && isActive)
        {
            g.setColour (juce::Colours::white.withAlpha (0.98f));
            g.fillRoundedRectangle (itemArea, 7.0f);
            g.setColour (theme.colour ("ui.button.background", juce::Colour (0xffe68000)));
            g.fillRoundedRectangle (itemArea.reduced (2.0f), 5.0f);
        }

        g.setColour (isActive ? juce::Colours::white : juce::Colours::white.withAlpha (0.45f));
        g.setFont (juce::Font (13.0f, juce::Font::bold));
        g.drawText (text, area.reduced (14, 0), juce::Justification::centredLeft, true);

        if (isTicked)
        {
            auto tickArea = area.withTrimmedLeft (juce::jmax (0, area.getWidth() - 18));
            g.drawText ("*", tickArea, juce::Justification::centred, false);
        }
    }

private:
    ThemeData& theme;
};
