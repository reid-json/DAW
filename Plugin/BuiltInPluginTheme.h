#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace BuiltInPluginTheme
{
    struct Palette
    {
        juce::Colour background { 0xff18181b };
        juce::Colour panel { 0xff232329 };
        juce::Colour accent { 0xffe68000 };
        juce::Colour text { juce::Colours::white };
        juce::Colour outline { juce::Colours::white.withAlpha (0.75f) };
        juce::Colour mutedText { juce::Colours::white.withAlpha (0.72f) };
    };

    inline Palette& state()
    {
        static Palette palette;
        return palette;
    }

    inline Palette getPalette()
    {
        return state();
    }

    inline juce::Image& bodySpiceImage()
    {
        static juce::Image image;
        return image;
    }

    inline void setAccentColour (juce::Colour newAccent)
    {
        state().accent = newAccent;
    }

    inline void setBodySpiceImage (juce::Image newImage)
    {
        bodySpiceImage() = std::move (newImage);
    }

    class LookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        void drawRotarySlider (juce::Graphics& g,
                               int x,
                               int y,
                               int width,
                               int height,
                               float sliderPos,
                               float rotaryStartAngle,
                               float rotaryEndAngle,
                               juce::Slider&) override
        {
            const auto palette = getPalette();
            auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (8.0f);
            const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
            const auto centre = bounds.getCentre();
            const auto rx = centre.x - radius;
            const auto ry = centre.y - radius;
            const auto rw = radius * 2.0f;
            const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

            g.setColour (palette.panel);
            g.fillEllipse (rx, ry, rw, rw);
            g.setColour (palette.outline.withAlpha (0.35f));
            g.drawEllipse (rx, ry, rw, rw, 1.2f);

            juce::Path valueArc;
            valueArc.addCentredArc (centre.x, centre.y, radius - 3.0f, radius - 3.0f, 0.0f,
                                    rotaryStartAngle, angle, true);
            g.setColour (palette.accent);
            g.strokePath (valueArc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            juce::Path pointer;
            pointer.addRoundedRectangle (-2.0f, -radius + 8.0f, 4.0f, radius * 0.55f, 2.0f);
            g.setColour (palette.text);
            g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
        }

        void drawLinearSlider (juce::Graphics& g,
                               int x,
                               int y,
                               int width,
                               int height,
                               float sliderPos,
                               float minSliderPos,
                               float maxSliderPos,
                               const juce::Slider::SliderStyle style,
                               juce::Slider&) override
        {
            const auto palette = getPalette();

            if (style == juce::Slider::LinearVertical || style == juce::Slider::LinearBarVertical)
            {
                auto track = juce::Rectangle<float> ((float) x + width * 0.5f - 5.0f, (float) y + 8.0f, 10.0f, (float) height - 16.0f);
                g.setColour (palette.panel.brighter (0.12f));
                g.fillRoundedRectangle (track, 5.0f);
                g.setColour (palette.outline.withAlpha (0.45f));
                g.drawRoundedRectangle (track, 5.0f, 1.0f);

                auto filled = track.withY (sliderPos).withBottom (track.getBottom());
                g.setColour (palette.accent);
                g.fillRoundedRectangle (filled, 5.0f);

                auto thumb = juce::Rectangle<float> (track.getX() - 5.0f, sliderPos - 7.0f, track.getWidth() + 10.0f, 14.0f);
                g.setColour (palette.text);
                g.fillRoundedRectangle (thumb, 7.0f);
                g.setColour (palette.outline.withAlpha (0.55f));
                g.drawRoundedRectangle (thumb, 7.0f, 1.0f);
                return;
            }

            auto track = juce::Rectangle<float> ((float) x + 8.0f, (float) y + height * 0.5f - 5.0f, (float) width - 16.0f, 10.0f);
            g.setColour (palette.panel.brighter (0.12f));
            g.fillRoundedRectangle (track, 5.0f);
            g.setColour (palette.outline.withAlpha (0.45f));
            g.drawRoundedRectangle (track, 5.0f, 1.0f);

            auto filled = track.withRight (sliderPos);
            g.setColour (palette.accent);
            g.fillRoundedRectangle (filled, 5.0f);

            auto thumb = juce::Rectangle<float> (sliderPos - 7.0f, track.getCentreY() - 7.0f, 14.0f, 14.0f);
            g.setColour (palette.text);
            g.fillRoundedRectangle (thumb, 7.0f);
            g.setColour (palette.outline.withAlpha (0.55f));
            g.drawRoundedRectangle (thumb, 7.0f, 1.0f);
        }

        void drawTickBox (juce::Graphics& g,
                          juce::Component&,
                          float x,
                          float y,
                          float w,
                          float h,
                          bool ticked,
                          bool,
                          bool,
                          bool) override
        {
            const auto palette = getPalette();
            auto bounds = juce::Rectangle<float> (x, y, w, h).reduced (1.0f);
            g.setColour (palette.outline.withAlpha (0.98f));
            g.fillRoundedRectangle (bounds, 6.0f);
            g.setColour (ticked ? palette.accent : palette.panel);
            g.fillRoundedRectangle (bounds.reduced (1.5f), 4.5f);

            if (ticked)
            {
                juce::Path tick;
                tick.startNewSubPath (bounds.getX() + bounds.getWidth() * 0.24f, bounds.getCentreY());
                tick.lineTo (bounds.getX() + bounds.getWidth() * 0.44f, bounds.getBottom() - bounds.getHeight() * 0.24f);
                tick.lineTo (bounds.getRight() - bounds.getWidth() * 0.20f, bounds.getY() + bounds.getHeight() * 0.24f);
                g.setColour (palette.text);
                g.strokePath (tick, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
        }
    };

    inline LookAndFeel& getLookAndFeel()
    {
        static LookAndFeel lookAndFeel;
        return lookAndFeel;
    }

    inline void styleLabel (juce::Label& label)
    {
        const auto palette = getPalette();
        label.setColour (juce::Label::textColourId, palette.text);
        label.setFont (juce::Font (13.0f, juce::Font::bold));
        label.setJustificationType (juce::Justification::centred);
    }

    inline void styleSlider (juce::Slider& slider)
    {
        const auto palette = getPalette();
        slider.setColour (juce::Slider::backgroundColourId, palette.panel);
        slider.setColour (juce::Slider::trackColourId, palette.accent);
        slider.setColour (juce::Slider::thumbColourId, palette.text);
        slider.setColour (juce::Slider::rotarySliderFillColourId, palette.accent);
        slider.setColour (juce::Slider::rotarySliderOutlineColourId, palette.outline.withAlpha (0.35f));
        slider.setColour (juce::Slider::textBoxTextColourId, palette.text);
        slider.setColour (juce::Slider::textBoxBackgroundColourId, palette.panel);
        slider.setColour (juce::Slider::textBoxOutlineColourId, palette.outline.withAlpha (0.55f));
        slider.setColour (juce::Slider::textBoxHighlightColourId, palette.accent.withAlpha (0.35f));
    }

    inline void styleToggleButton (juce::ToggleButton& button)
    {
        const auto palette = getPalette();
        button.setColour (juce::ToggleButton::textColourId, palette.text);
    }

    inline void applyEditorTheme (juce::AudioProcessorEditor& editor)
    {
        editor.setLookAndFeel (&getLookAndFeel());
        editor.setColour (juce::ResizableWindow::backgroundColourId, getPalette().background);
    }

    inline void clearEditorTheme (juce::AudioProcessorEditor& editor)
    {
        editor.setLookAndFeel (nullptr);
    }

    inline void paintEditorBackground (juce::Graphics& g, juce::Component& component)
    {
        g.fillAll (getPalette().background);

        auto image = bodySpiceImage();
        if (image.isValid())
        {
            const auto bounds = component.getLocalBounds();
            g.setOpacity (0.92f);
            g.drawImage (image,
                         bounds.getX(),
                         bounds.getY() + juce::roundToInt (bounds.getHeight() * 0.45f),
                         bounds.getWidth(),
                         bounds.getHeight(),
                         0,
                         0,
                         image.getWidth(),
                         image.getHeight());
            g.setOpacity (1.0f);
        }
    }
}
