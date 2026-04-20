#include "jive_LookAndFeel.h"

namespace jive
{
    bool LookAndFeel::areScrollbarButtonsVisible()
    {
        return false;
    }

    void LookAndFeel::drawScrollbarButton(juce::Graphics& /* g */,
                                          juce::ScrollBar& /* scrollbar */,
                                          int /* width */,
                                          int /* height */,
                                          int /* buttonDirection */,
                                          bool /* isScrollbarVertical */,
                                          bool /* isMouseOverButton */,
                                          bool /* isButtonDown */)
    {
    }

    void LookAndFeel::drawScrollbar(juce::Graphics& g,
                                    juce::ScrollBar& /* scrollbar */,
                                    int x,
                                    int y,
                                    int width,
                                    int height,
                                    bool isScrollbarVertical,
                                    int thumbStartPosition,
                                    int thumbSize,
                                    bool isMouseOver,
                                    bool isMouseDown)
    {
        auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();

        g.setColour (juce::Colour (0x1cffffff));
        g.fillRoundedRectangle (bounds.reduced (1.0f), 4.0f);

        juce::Rectangle<float> thumbBounds;
        if (isScrollbarVertical)
            thumbBounds = { static_cast<float> (x + 2), static_cast<float> (thumbStartPosition), static_cast<float> (width - 4), static_cast<float> (thumbSize) };
        else
            thumbBounds = { static_cast<float> (thumbStartPosition), static_cast<float> (y + 2), static_cast<float> (thumbSize), static_cast<float> (height - 4) };

        auto thumbColour = juce::Colour (0xff4c88ff);
        if (isMouseDown)
            thumbColour = thumbColour.brighter (0.2f);
        else if (isMouseOver)
            thumbColour = thumbColour.withMultipliedBrightness (1.1f);

        g.setColour (thumbColour.withAlpha (0.9f));
        g.fillRoundedRectangle (thumbBounds, 4.0f);
    }

    juce::ImageEffectFilter* LookAndFeel::getScrollbarEffect()
    {
        return nullptr;
    }

    int LookAndFeel::getMinimumScrollbarThumbSize(juce::ScrollBar&)
    {
        return 24;
    }

    int LookAndFeel::getDefaultScrollbarWidth()
    {
        return 12;
    }

    int LookAndFeel::getScrollbarButtonSize(juce::ScrollBar&)
    {
        return 0;
    }

    void LookAndFeel::drawButtonBackground(juce::Graphics&,
                                           juce::Button&,
                                           const juce::Colour& /* backgroundColour */,
                                           bool /* shouldDrawButtonAsHighlighted */,
                                           bool /* shouldDrawButtonAsDown */)
    {
    }

    juce::Font LookAndFeel::getTextButtonFont(juce::TextButton&,
                                              int /* buttonHeight */)
    {
#if JUCE_MAJOR_VERSION < 8
        return {};
#else
        return juce::FontOptions{};
#endif
    }

    int LookAndFeel::getTextButtonWidthToFitText(juce::TextButton&,
                                                 int /* buttonHeight */)
    {
        return 0;
    }

    void LookAndFeel::drawButtonText(juce::Graphics&,
                                     juce::TextButton&,
                                     bool /* shouldDrawButtonAsHighlighted */,
                                     bool /* shouldDrawButtonAsDown */)
    {
    }

    void LookAndFeel::drawToggleButton(juce::Graphics&,
                                       juce::ToggleButton&,
                                       bool /* shouldDrawButtonAsHighlighted */,
                                       bool /* shouldDrawButtonAsDown */)
    {
    }

    void LookAndFeel::changeToggleButtonWidthToFitText(juce::ToggleButton&)
    {
    }

    void LookAndFeel::drawTickBox(juce::Graphics&,
                                  juce::Component&,
                                  float /* x */,
                                  float /* y */,
                                  float /* w */,
                                  float /* h */,
                                  bool /* ticked */,
                                  bool /* isEnabled */,
                                  bool /* shouldDrawButtonAsHighlighted */,
                                  bool /* shouldDrawButtonAsDown */)
    {
    }

    void LookAndFeel::drawDrawableButton(juce::Graphics&,
                                         juce::DrawableButton&,
                                         bool /* shouldDrawButtonAsHighlighted */,
                                         bool /* shouldDrawButtonAsDown */)
    {
    }

    void LookAndFeel::fillTextEditorBackground(juce::Graphics&,
                                               int /* width */,
                                               int /* height */,
                                               juce::TextEditor&)
    {
    }

    void LookAndFeel::drawTextEditorOutline(juce::Graphics&,
                                            int /* width */,
                                            int /* height */,
                                            juce::TextEditor&)
    {
    }

    juce::CaretComponent* LookAndFeel::createCaretComponent(juce::Component* /* keyFocusOwner */)
    {
        return nullptr;
    }

    void LookAndFeel::drawComboBox(juce::Graphics&,
                                   int /* width */,
                                   int /* height */,
                                   bool /* isButtonDown */,
                                   int /* buttonX */,
                                   int /* buttonY */,
                                   int /* buttonW */,
                                   int /* buttonH */,
                                   juce::ComboBox&)
    {
    }

    juce::Font LookAndFeel::getComboBoxFont(juce::ComboBox&)
    {
#if JUCE_MAJOR_VERSION < 8
        return {};
#else
        return juce::FontOptions{};
#endif
    }

    void LookAndFeel::positionComboBoxText(juce::ComboBox&,
                                           juce::Label& /* labelToPosition */)
    {
    }

    juce::PopupMenu::Options LookAndFeel::getOptionsForComboBoxPopupMenu(juce::ComboBox&,
                                                                         juce::Label&)
    {
        return {};
    }

    void LookAndFeel::drawComboBoxTextWhenNothingSelected(juce::Graphics&,
                                                          juce::ComboBox&,
                                                          juce::Label&)
    {
    }

    void LookAndFeel::drawCornerResizer(juce::Graphics&,
                                        int /* w */,
                                        int /* h */,
                                        bool /* isMouseOver */,
                                        bool /* isMouseDragging */)
    {
    }

    void LookAndFeel::drawResizableFrame(juce::Graphics&,
                                         int /* w */,
                                         int /* h */,
                                         const juce::BorderSize<int>&)
    {
    }

    void LookAndFeel::drawResizableWindowBorder(juce::Graphics&,
                                                int /* w */,
                                                int /* h */,
                                                const juce::BorderSize<int>& /* border */,
                                                juce::ResizableWindow&)
    {
    }
} // namespace jive
