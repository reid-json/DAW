#include "pianoRoll.h"
#include <algorithm>
#include <cmath>


namespace PianoRoll
{
    juce::Rectangle<float> getNoteBounds (const Note& note)
    {
        auto row = noteToRow (note.midiNote);
        auto x = (float) (note.startBeat * beatWidth);
        auto y = (float) (row * rowHeight);
        auto w = (float) juce::jmax (12.0, note.lengthBeats * beatWidth);
        return { x + 1.5f, y + 2.0f, w - 3.0f, (float) rowHeight - 4.0f };
    }

    juce::String getNoteName (int midiNote)
    {
        static const char* names[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
        return juce::String (names[midiNote % 12]) + juce::String ((midiNote / 12) - 1);
    }

    bool isBlackKey (int midiNote)
    {
        switch (midiNote % 12)
        {
            case 1: case 3: case 6: case 8: case 10: return true;
            default: return false;
        }
    }

    int noteToRow (int midiNote) { return topNote - midiNote; }
    int rowToNote (int row)      { return topNote - row; }
}

namespace
{
    const auto pianoRollBackground = juce::Colour (0xff18181b);
    const auto pianoRollPanel = juce::Colour (0xff18181b);
    const auto pianoRollGridDark = juce::Colour (0xff18181b);
    const auto pianoRollGridLight = juce::Colour (0xff1f1f23);
    const auto pianoRollTimeline = juce::Colour (0xff232329);
    const auto pianoRollPlayhead = juce::Colours::white.withAlpha (0.96f);
    const auto pianoRollText = juce::Colours::white;

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

    juce::Image loadSpriteImage (const juce::String& fileName)
    {
        const auto spriteFile = findResourcesDir().getChildFile ("ui")
                                                  .getChildFile ("sprites")
                                                  .getChildFile (fileName);
        return spriteFile.existsAsFile() ? juce::ImageFileFormat::loadFrom (spriteFile) : juce::Image{};
    }

    juce::Image createHorizontallyFlippedImage (const juce::Image& source)
    {
        if (! source.isValid())
            return {};

        juce::Image flipped (source.getFormat(), source.getWidth(), source.getHeight(), true);
        juce::Graphics g (flipped);
        g.drawImageTransformed (source,
                                juce::AffineTransform (-1.0f, 0.0f, (float) source.getWidth(),
                                                       0.0f, 1.0f, 0.0f),
                                false);
        return flipped;
    }

}

class PianoRollComponent::PianoRollLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    explicit PianoRollLookAndFeel (PianoRollComponent& ownerIn) : owner (ownerIn) {}

    void drawScrollbar (juce::Graphics& g,
                        juce::ScrollBar& scrollbar,
                        int x,
                        int y,
                        int width,
                        int height,
                        bool,
                        int thumbStartPosition,
                        int thumbSize,
                        bool,
                        bool) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
        g.setColour (juce::Colours::white.withAlpha (0.14f));
        g.fillRoundedRectangle (bounds.reduced (1.0f), scrollbar.isVertical() ? width * 0.5f : height * 0.5f);

        juce::Rectangle<float> thumbBounds;
        if (scrollbar.isVertical())
            thumbBounds = { (float) x + 1.0f, (float) thumbStartPosition + 1.0f, (float) width - 2.0f, (float) thumbSize - 2.0f };
        else
            thumbBounds = { (float) thumbStartPosition + 1.0f, (float) y + 1.0f, (float) thumbSize - 2.0f, (float) height - 2.0f };

        const float radius = scrollbar.isVertical() ? (float) width * 0.5f : (float) height * 0.5f;
        g.setColour (juce::Colours::white.withAlpha (0.98f));
        g.fillRoundedRectangle (thumbBounds, radius);
        g.setColour (owner.accentColour);
        g.fillRoundedRectangle (thumbBounds.reduced (1.5f), juce::jmax (0.0f, radius - 1.5f));
    }

    void drawPopupMenuBackground (juce::Graphics& g, int width, int height) override
    {
        g.fillAll (pianoRollBackground);
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
            g.setColour (owner.accentColour);
            g.fillRoundedRectangle (itemArea.reduced (2.0f), 5.0f);
        }

        g.setColour (isActive ? pianoRollText : pianoRollText.withAlpha (0.45f));
        g.setFont (juce::Font (13.0f, juce::Font::bold));
        g.drawText (text, area.reduced (14, 0), juce::Justification::centredLeft, true);

        if (isTicked)
        {
            auto tickArea = area.withTrimmedLeft (juce::jmax (0, area.getWidth() - 18));
            g.drawText ("*", tickArea, juce::Justification::centred, false);
        }
    }

private:
    PianoRollComponent& owner;
};

// ScrollListener

void PianoRollComponent::ScrollListener::scrollBarMoved (juce::ScrollBar* bar, double newStart)
{
    if (bar == &owner.hScroll)
        owner.scrollX = (int) std::round (newStart);
    else if (bar == &owner.vScroll)
        owner.scrollY = (int) std::round (newStart);

    owner.updateViewport();
}

// constructor

PianoRollComponent::PianoRollComponent()
    : headerSpiceImage (loadSpriteImage ("headerSpice.png"))
    , bodySpiceImage (loadSpriteImage ("bodySpice.png"))
    , selectToolIcon (loadSpriteImage ("selecticon.png"))
    , drawToolIcon (loadSpriteImage ("drawicon.png"))
    , eraseToolIcon (loadSpriteImage ("eraseicon.png"))
{
    pianoRollLookAndFeel = std::make_unique<PianoRollLookAndFeel> (*this);
    addAndMakeVisible (hScroll);
    addAndMakeVisible (vScroll);
    setOpaque (true);
    setWantsKeyboardFocus (true);
    hScroll.setLookAndFeel (pianoRollLookAndFeel.get());
    vScroll.setLookAndFeel (pianoRollLookAndFeel.get());
    hScroll.addListener (&scrollListener);
    vScroll.addListener (&scrollListener);
    hScroll.setAutoHide (false);
    vScroll.setAutoHide (false);
}

PianoRollComponent::~PianoRollComponent()
{
    hScroll.setLookAndFeel (nullptr);
    vScroll.setLookAndFeel (nullptr);
}

// public setters

void PianoRollComponent::setNotes (std::vector<PianoRoll::Note> newNotes)
{
    notes = std::move (newNotes);
    nextId = 1;
    for (auto& n : notes)
        nextId = juce::jmax (nextId, n.id + 1);

    selectedIds.clear();
    drawingNote.reset();
    resizing.reset();
    moving.reset();
    marqueeState.reset();
    refreshDisplay();
}

void PianoRollComponent::setOnSavePattern (std::function<void (const std::vector<PianoRoll::Note>&)> cb)
{
    onSavePatternRequested = std::move (cb);
}

void PianoRollComponent::setOnGetAvailableInstruments (std::function<juce::StringArray()> cb)
{
    onGetAvailableInstruments = std::move (cb);
}

void PianoRollComponent::setOnInstrumentChanged (std::function<void (const juce::String&)> cb)
{
    onInstrumentChangeRequested = std::move (cb);
}

void PianoRollComponent::setInstrumentName (const juce::String& name)
{
    instrumentName = name;
    repaint();
}

void PianoRollComponent::setThemeAssets (juce::Image newHeaderSpiceImage,
                                         juce::Image newBodySpiceImage,
                                         juce::Colour newAccentColour)
{
    headerSpiceImage = std::move (newHeaderSpiceImage);
    bodySpiceImage = std::move (newBodySpiceImage);
    accentColour = newAccentColour;
    repaint();
}

// painting

void PianoRollComponent::paint (juce::Graphics& g)
{
    using namespace PianoRoll;
    g.fillAll (pianoRollBackground);

    auto header = getLocalBounds().removeFromTop (headerH);
    paintToolbar (g, header.removeFromTop (toolbarH));
    paintTimeline (g, header);

    // Keyboard strip
    {
        juce::Graphics::ScopedSaveState sss (g);
        g.reduceClipRegion (keyArea);
        paintKeys (g);
    }

    // Grid + notes + marquee
    {
        juce::Graphics::ScopedSaveState sss (g);
        g.reduceClipRegion (gridArea);
        paintGrid (g);
        paintNotes (g);
        paintMarquee (g);

        // playhead
        int px = gridArea.getX() + 2 * beatWidth - scrollX;
        g.setColour (pianoRollPlayhead);
        g.drawLine ((float) px, (float) gridArea.getY(), (float) px, (float) gridArea.getBottom(), 2.0f);
    }
}

void PianoRollComponent::paintKeys (juce::Graphics& g)
{
    using namespace PianoRoll;

    g.fillAll (juce::Colour::fromRGB (232, 235, 240));

    // White keys first, then black on top
    for (int pass = 0; pass < 2; ++pass)
    {
        for (int row = 0; row < numRows; ++row)
        {
            int midi = topNote - row;
            int y = keyArea.getY() + row * rowHeight - scrollY;
            if (y >= keyArea.getBottom() || y + rowHeight <= keyArea.getY()) continue;

            bool black = isBlackKey (midi);
            if ((pass == 0) == black) continue;

            if (black)
            {
                auto r = juce::Rectangle<float> ((float) keyArea.getX(),
                                                  (float) y + 1.0f,
                                                  (float) keyArea.getWidth() * 0.72f,
                                                  (float) rowHeight - 2.0f);
                g.setColour (juce::Colour::fromRGB (36, 40, 48));
                g.fillRoundedRectangle (r, 3.0f);
                g.setColour (juce::Colour::fromRGB (66, 72, 82));
                g.drawRoundedRectangle (r, 3.0f, 1.0f);

                g.setColour (juce::Colours::white.withAlpha (0.88f));
                g.setFont (12.0f);
                g.drawText (getNoteName (midi), r.toNearestInt().withTrimmedLeft (10),
                            juce::Justification::centredLeft);
            }
            else
            {
                auto r = juce::Rectangle<int> (keyArea.getX(), y, keyArea.getWidth(), rowHeight);
                g.setColour (juce::Colour::fromRGB (242, 244, 247));
                g.fillRect (r);
                g.setColour (juce::Colour::fromRGB (185, 190, 198));
                g.drawRect (r, 1);

                g.setColour (juce::Colours::black.withAlpha (0.78f));
                g.setFont (12.0f);
                g.drawText (getNoteName (midi), r.withTrimmedLeft (10),
                            juce::Justification::centredLeft);
            }
        }
    }

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawVerticalLine (keyArea.getRight() - 1, (float) keyArea.getY(), (float) keyArea.getBottom());
}

void PianoRollComponent::paintGrid (juce::Graphics& g)
{
    using namespace PianoRoll;

    int gx = gridArea.getX();

    // Row backgrounds
    for (int row = 0; row < numRows; ++row)
    {
        int midi = topNote - row;
        int y = gridArea.getY() + row * rowHeight - scrollY;
        if (y >= gridArea.getBottom() || y + rowHeight <= gridArea.getY()) continue;

        g.setColour (isBlackKey (midi) ? pianoRollGridDark
                                       : pianoRollGridLight);
        g.fillRect (gx, y, gridArea.getWidth(), rowHeight);

        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.drawHorizontalLine (y, (float) gx, (float) gridArea.getRight());
    }

    if (bodySpiceImage.isValid())
    {
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (gridArea);
        g.setOpacity (0.92f);
        g.drawImage (bodySpiceImage,
                     gridArea.getX(),
                     gridArea.getY() + juce::roundToInt (gridArea.getHeight() * 0.45f),
                     gridArea.getWidth(),
                     gridArea.getHeight(),
                     0,
                     0,
                     bodySpiceImage.getWidth(),
                     bodySpiceImage.getHeight());
        g.setOpacity (1.0f);
    }

    // Beat lines
    for (int beat = 0; beat <= numBeats; ++beat)
    {
        int x = gx + beat * beatWidth - scrollX;
        if (x < gx - beatWidth || x > gridArea.getRight() + beatWidth) continue;

        bool major = (beat % 4 == 0);
        g.setColour (juce::Colours::white.withAlpha (major ? 0.14f : 0.07f));
        g.drawVerticalLine (x, (float) gridArea.getY(), (float) gridArea.getBottom());

        if (beat < numBeats)
        {
            g.setColour (juce::Colours::white.withAlpha (0.035f));
            g.drawVerticalLine (x + beatWidth / 2, (float) gridArea.getY(), (float) gridArea.getBottom());
        }
    }
}

void PianoRollComponent::paintNotes (juce::Graphics& g)
{
    using namespace PianoRoll;

    auto allNotes = notes;
    if (drawingNote.has_value())
        allNotes.push_back (*drawingNote);

    for (auto& note : allNotes)
    {
        int row = noteToRow (note.midiNote);
        if (row < 0 || row >= numRows) continue;

        auto r = getNoteBounds (note).translated ((float) (gridArea.getX() - scrollX),
                                                   (float) (gridArea.getY() - scrollY));
        bool selected = selectedIds.count (note.id) > 0;

        g.setColour (selected ? accentColour.brighter (0.18f)
                              : accentColour);
        g.fillRoundedRectangle (r, 6.0f);

        g.setColour (juce::Colours::white.withAlpha (selected ? 0.60f : 0.24f));
        g.drawRoundedRectangle (r, 6.0f, selected ? 1.5f : 1.0f);

        if (r.getWidth() > 28.0f)
        {
            g.setColour (juce::Colours::white.withAlpha (0.95f));
            g.setFont (12.0f);
            g.drawText (note.label, r.toNearestInt().reduced (8, 0),
                        juce::Justification::centredLeft, false);
        }
    }
}

void PianoRollComponent::paintMarquee (juce::Graphics& g)
{
    auto rect = getMarqueeRect();
    if (! rect.has_value()) return;

    g.setColour (accentColour.withAlpha (0.18f));
    g.fillRect (*rect);
    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.drawRect (*rect, 1);
}

void PianoRollComponent::paintToolbar (juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour (pianoRollBackground);
    g.fillRect (area);

    if (headerSpiceImage.isValid())
    {
        const float sourceAspect = (float) headerSpiceImage.getWidth() / (float) juce::jmax (1, headerSpiceImage.getHeight());
        const float drawHeight = (float) area.getHeight();
        const float drawWidth = drawHeight * sourceAspect;
        const auto dest = juce::Rectangle<float> ((float) area.getX(),
                                                  (float) area.getY(),
                                                  drawWidth,
                                                  drawHeight);
        g.drawImage (headerSpiceImage,
                     juce::roundToInt (dest.getX()),
                     juce::roundToInt (dest.getY()),
                     juce::roundToInt (dest.getWidth()),
                     juce::roundToInt (dest.getHeight()),
                     0,
                     0,
                     headerSpiceImage.getWidth(),
                     headerSpiceImage.getHeight());
    }

    paintButton (g, btnRects.select, "Select", tool == Tool::select, false, &selectToolIcon);
    paintButton (g, btnRects.draw,   "Draw",   tool == Tool::draw, false, &drawToolIcon);
    paintButton (g, btnRects.erase,  "Erase",  tool == Tool::erase, false, &eraseToolIcon);

    auto instLabel = instrumentName.isEmpty() ? juce::String ("No Instrument") : instrumentName;
    paintButton (g, btnRects.instrument, instLabel, instrumentName.isNotEmpty());

    bool canSave = ! instrumentName.isEmpty();
    paintButton (g, btnRects.save, "Save Pattern", false, canSave);

    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawHorizontalLine (area.getBottom() - 1, 0.0f, (float) area.getRight());
}

void PianoRollComponent::paintTimeline (juce::Graphics& g, juce::Rectangle<int> area)
{
    using namespace PianoRoll;

    g.setColour (pianoRollTimeline);
    g.fillRect (area);

    auto keyHeader = area.removeFromLeft (keyWidth);
    g.setColour (pianoRollText);
    g.setFont (juce::Font (13.0f, juce::Font::bold));
    g.drawText ("Bars", keyHeader.reduced (12, 0), juce::Justification::centredLeft);

    // Beat numbers and lines
    for (int beat = 0; beat <= numBeats; ++beat)
    {
        int x = area.getX() + beat * beatWidth - scrollX;
        if (x < area.getX() - beatWidth || x > area.getRight() + beatWidth) continue;

        bool major = (beat % 4 == 0);
        g.setColour (juce::Colours::white.withAlpha (major ? 0.16f : 0.08f));
        g.drawVerticalLine (x, (float) area.getY(), (float) area.getBottom());

        if (beat < numBeats)
        {
            g.setColour (juce::Colours::white.withAlpha (0.04f));
            g.drawVerticalLine (x + beatWidth / 2, (float) area.getY() + 18.0f, (float) area.getBottom());

            g.setColour (pianoRollText);
            g.setFont (juce::Font (13.0f, juce::Font::bold));
            g.drawText (juce::String (beat + 1), x + 4, area.getY() + 12, 28, 14,
                        juce::Justification::centredLeft);
        }
    }

    // Playhead
    int px = area.getX() + 2 * beatWidth - scrollX;
    g.setColour (pianoRollPlayhead);
    g.drawLine ((float) px, (float) area.getY(), (float) px, (float) area.getBottom(), 2.0f);

    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawHorizontalLine (area.getBottom() - 1, 0.0f, (float) getWidth());
    g.drawVerticalLine (keyWidth - 1, (float) area.getY(), (float) area.getBottom());
}

void PianoRollComponent::paintButton (juce::Graphics& g, juce::Rectangle<int> area,
                                      const juce::String& text, bool active, bool accent,
                                      const juce::Image* icon)
{
    const auto disabledAccent = accentColour.darker (0.35f);
    auto bg = active ? accentColour.brighter (0.12f)
                     : accent ? accentColour
                              : disabledAccent;
    auto bounds = area.toFloat();
    g.setColour (juce::Colours::white.withAlpha (0.98f));
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (bg);
    g.fillRoundedRectangle (bounds.reduced (1.5f), 6.5f);

    if (icon != nullptr && icon->isValid())
    {
        g.setOpacity (1.0f);
        g.drawImageWithin (*icon,
                           area.getX() + 6,
                           area.getY() + 5,
                           juce::jmax (1, area.getWidth() - 12),
                           juce::jmax (1, area.getHeight() - 10),
                           juce::RectanglePlacement::centred);
        g.setOpacity (1.0f);
        return;
    }

    g.setColour (pianoRollText);
    g.setFont (juce::Font (13.0f, juce::Font::bold));
    g.drawText (text, area, juce::Justification::centred);
}

// layout

void PianoRollComponent::resized()
{
    using namespace PianoRoll;

    auto bounds = getLocalBounds();
    btnRects = layoutButtons (bounds.removeFromTop (toolbarH));
    bounds.removeFromTop (timelineH);

    constexpr int scrollSize = 14;
    auto vBar = bounds.removeFromRight (scrollSize);
    auto hBar = bounds.removeFromBottom (scrollSize);
    vBar.removeFromBottom (scrollSize);

    keyArea = bounds.removeFromLeft (keyWidth);
    gridArea = bounds;

    hScroll.setBounds (hBar.withX (gridArea.getX()).withWidth (gridArea.getWidth()));
    vScroll.setBounds (vBar.withY (gridArea.getY()).withHeight (gridArea.getHeight()));
    updateScrollbars();
    updateViewport();
}

PianoRollComponent::ButtonRects PianoRollComponent::layoutButtons (juce::Rectangle<int> area) const
{
    ButtonRects r;
    auto content = area.reduced (12, 9);

    auto toolBtns = content.removeFromLeft (228);
    r.select = toolBtns.removeFromLeft (74).reduced (2, 0);
    r.draw   = toolBtns.removeFromLeft (70).reduced (2, 0);
    r.erase  = toolBtns.removeFromLeft (70).reduced (2, 0);

    content.removeFromLeft (10);
    r.instrument = content.removeFromLeft (158).reduced (2, 0);
    content.removeFromLeft (10);
    r.save = content.removeFromLeft (118).reduced (2, 0);
    return r;
}

bool PianoRollComponent::isOverButton (juce::Point<int> pos) const
{
    return btnRects.select.contains (pos) || btnRects.draw.contains (pos)
        || btnRects.erase.contains (pos)  || btnRects.save.contains (pos)
        || btnRects.instrument.contains (pos);
}

juce::String PianoRollComponent::getToolbarTooltip (juce::Point<int> pos) const
{
    if (btnRects.select.contains (pos))
        return "Select notes, resize and move bars, and select multiple notes";

    if (btnRects.draw.contains (pos))
        return "Draw notes into the piano roll";

    if (btnRects.erase.contains (pos))
        return "Erase notes from the piano roll";

    return {};
}

bool PianoRollComponent::handleButtonClick (juce::Point<int> pos)
{
    auto toolBtnArea = btnRects.select.getUnion (btnRects.draw).getUnion (btnRects.erase);

    if (btnRects.select.contains (pos) && tool != Tool::select)
    {
        tool = Tool::select;
        repaint (toolBtnArea);
        return true;
    }
    if (btnRects.draw.contains (pos) && tool != Tool::draw)
    {
        tool = Tool::draw;
        repaint (toolBtnArea);
        return true;
    }
    if (btnRects.erase.contains (pos) && tool != Tool::erase)
    {
        tool = Tool::erase;
        repaint (toolBtnArea);
        return true;
    }
    if (btnRects.instrument.contains (pos))
    {
        juce::StringArray instruments;
        if (onGetAvailableInstruments)
            instruments = onGetAvailableInstruments();

        juce::PopupMenu menu;
        menu.setLookAndFeel (pianoRollLookAndFeel.get());
        if (instruments.isEmpty())
            menu.addItem (1, "No instruments found", false);
        else
            for (int i = 0; i < instruments.size(); ++i)
                menu.addItem (i + 1, instruments[i], true, instruments[i] == instrumentName);

        menu.showMenuAsync (juce::PopupMenu::Options(), [this, instruments] (int result)
        {
            if (result > 0 && result <= instruments.size())
            {
                instrumentName = instruments[result - 1];
                if (onInstrumentChangeRequested)
                    onInstrumentChangeRequested (instrumentName);
                repaint();
            }
        });
        return true;
    }
    if (btnRects.save.contains (pos))
    {
        if (instrumentName.isEmpty() || notes.empty())
            return true;

        if (onSavePatternRequested)
        {
            onSavePatternRequested (notes);
            notes.clear();
            selectedIds.clear();
            nextId = 1;
            repaint();
        }
        return true;
    }
    return false;
}

// mouse handling

void PianoRollComponent::mouseMove (const juce::MouseEvent& e)
{
    setMouseCursor (isOverButton (e.getPosition())
                        ? juce::MouseCursor::PointingHandCursor
                        : juce::MouseCursor::NormalCursor);

    setTooltip (getToolbarTooltip (e.getPosition()));
}

void PianoRollComponent::mouseExit (const juce::MouseEvent&)
{
    setMouseCursor (juce::MouseCursor::NormalCursor);
    setTooltip ({});
}

void PianoRollComponent::mouseDown (const juce::MouseEvent& e)
{
    if (handleButtonClick (e.getPosition()))
        return;

    if (gridArea.contains (e.getPosition()) || keyArea.contains (e.getPosition()))
        gridMouseDown (e);
}

void PianoRollComponent::mouseDrag (const juce::MouseEvent& e)
{
    gridMouseDrag (e);
}

void PianoRollComponent::mouseUp (const juce::MouseEvent& e)
{
    gridMouseUp (e);
}

void PianoRollComponent::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    handleScroll (e, wheel);
}

// grid mouse logic

void PianoRollComponent::gridMouseDown (const juce::MouseEvent& e)
{
    grabKeyboardFocus();
    auto pos = toGridPos (e.getPosition());

    if (tool == Tool::select)
    {
        if (auto edge = findNoteEdge (pos))
        {
            selectOne (edge->first);
            startResize (edge->first, edge->second);
        }
        else if (auto idx = findNoteAt (pos))
        {
            bool ctrl = e.mods.isCtrlDown() || e.mods.isCommandDown();
            bool shift = e.mods.isShiftDown();

            if (ctrl)
            {
                auto sel = selectedIds;
                if (sel.count (notes[*idx].id))
                    sel.erase (notes[*idx].id);
                else
                    sel.insert (notes[*idx].id);
                setSelection (std::move (sel));
            }
            else if (shift)
            {
                auto sel = selectedIds;
                sel.insert (notes[*idx].id);
                setSelection (std::move (sel));
            }
            else if (! selectedIds.count (notes[*idx].id))
            {
                selectOne (idx);
            }

            if (! shift && ! ctrl)
                startMove (*idx, pos);
        }
        else
        {
            startMarquee (e);
        }
        return;
    }

    if (tool == Tool::draw)  { startDraw (pos); return; }
    if (tool == Tool::erase) { eraseAt (pos); }
}

void PianoRollComponent::gridMouseDrag (const juce::MouseEvent& e)
{
    auto pos = toGridPos (e.getPosition());

    if (tool == Tool::select)
    {
        if (marqueeState.has_value())   { updateMarquee (pos); return; }
        if (resizing.has_value())       { updateResize (pos);  return; }
        if (moving.has_value())         { updateMove (pos);    return; }
    }

    if (tool == Tool::draw && drawingNote.has_value())
        updateDraw (pos);
}

void PianoRollComponent::gridMouseUp (const juce::MouseEvent& e)
{
    auto pos = toGridPos (e.getPosition());

    if (tool == Tool::select)
    {
        if (marqueeState.has_value()) { updateMarquee (pos); endMarquee(); return; }
        if (resizing.has_value())     { updateResize (pos);  endResize();  return; }
        if (moving.has_value())       { updateMove (pos);    endMove();    return; }
    }

    if (tool == Tool::draw && drawingNote.has_value())
    {
        updateDraw (pos);
        commitDraw();
    }
}

// note finding

std::optional<size_t> PianoRollComponent::findNoteAt (juce::Point<int> pos) const
{
    using namespace PianoRoll;

    double clickBeat = pos.x / (double) beatWidth;
    int row = juce::jlimit (0, numRows - 1, pos.y / rowHeight);
    int midi = rowToNote (row);

    for (size_t i = notes.size(); i-- > 0;)
    {
        auto& n = notes[i];
        if (n.midiNote == midi && clickBeat >= n.startBeat && clickBeat <= n.startBeat + n.lengthBeats)
            return i;
    }
    return std::nullopt;
}

std::optional<std::pair<size_t, PianoRollComponent::Edge>>
PianoRollComponent::findNoteEdge (juce::Point<int> pos) const
{
    using namespace PianoRoll;

    auto idx = findNoteAt (pos);
    if (! idx.has_value()) return std::nullopt;

    double clickBeat = pos.x / (double) beatWidth;
    auto& n = notes[*idx];
    double threshold = 8.0 / (double) beatWidth;

    if (std::abs (clickBeat - n.startBeat) <= threshold)
        return std::make_pair (*idx, Edge::left);
    if (std::abs (clickBeat - (n.startBeat + n.lengthBeats)) <= threshold)
        return std::make_pair (*idx, Edge::right);

    return std::nullopt;
}

std::optional<size_t> PianoRollComponent::findNoteById (int id) const
{
    for (size_t i = 0; i < notes.size(); ++i)
        if (notes[i].id == id) return i;
    return std::nullopt;
}

// selection

void PianoRollComponent::setSelection (std::set<int> ids)
{
    selectedIds = std::move (ids);
    repaint (gridArea);
}

void PianoRollComponent::selectOne (std::optional<size_t> index)
{
    if (! index.has_value())
        setSelection ({});
    else
        setSelection ({ notes[*index].id });
}

void PianoRollComponent::startMarquee (const juce::MouseEvent& e)
{
    auto pos = toGridPos (e.getPosition());
    marqueeState = MarqueeInfo { pos, pos,
                                 e.mods.isShiftDown() ? selectedIds : std::set<int> {} };
    if (! e.mods.isShiftDown())
        setSelection ({});
    repaint (gridArea);
}

void PianoRollComponent::updateMarquee (juce::Point<int> pos)
{
    if (! marqueeState.has_value()) return;
    marqueeState->current = pos;

    std::set<int> sel = marqueeState->baseSelection;
    auto rect = juce::Rectangle<int>::leftTopRightBottom (
        juce::jmin (marqueeState->start.x, pos.x),
        juce::jmin (marqueeState->start.y, pos.y),
        juce::jmax (marqueeState->start.x, pos.x),
        juce::jmax (marqueeState->start.y, pos.y)).toFloat();

    for (auto& n : notes)
        if (PianoRoll::getNoteBounds (n).intersects (rect))
            sel.insert (n.id);

    setSelection (std::move (sel));
    repaint (gridArea);
}

void PianoRollComponent::endMarquee()
{
    marqueeState.reset();
    repaint (gridArea);
}

// note moving

void PianoRollComponent::startMove (size_t index, juce::Point<int> pos)
{
    using namespace PianoRoll;
    auto& n = notes[index];
    double clickBeat = pos.x / (double) beatWidth;
    int clickRow = juce::jlimit (0, numRows - 1, pos.y / rowHeight);
    int noteRow = noteToRow (n.midiNote);

    std::vector<MoveSnapshot> snaps;
    for (auto& c : notes)
        if (selectedIds.count (c.id))
            snaps.push_back ({ c.id, c.startBeat, c.midiNote, c.lengthBeats });

    moving = MoveInfo { n.id, clickBeat - n.startBeat, clickRow - noteRow,
                        n.startBeat, n.midiNote, std::move (snaps) };
}

void PianoRollComponent::updateMove (juce::Point<int> pos)
{
    using namespace PianoRoll;
    if (! moving.has_value()) return;

    double step = getStepBeats();
    double rawBeat = (pos.x / (double) beatWidth) - moving->beatOffset;
    double snapped = std::round (rawBeat / step) * step;
    double timeDelta = snapped - moving->anchorBeat;

    int ptrRow = juce::jlimit (0, numRows - 1, pos.y / rowHeight);
    int rowDelta = (ptrRow - moving->rowOffset) - noteToRow (moving->anchorNote);

    // Clamp so nothing goes out of bounds
    for (auto& s : moving->snapshots)
    {
        timeDelta = juce::jlimit (-s.startBeat, (double) numBeats - (s.startBeat + s.lengthBeats), timeDelta);
        int sRow = noteToRow (s.midiNote);
        rowDelta = juce::jlimit (-sRow, (numRows - 1) - sRow, rowDelta);
    }

    for (auto& s : moving->snapshots)
    {
        auto idx = findNoteById (s.noteId);
        if (! idx.has_value()) continue;
        auto& note = notes[*idx];
        note.startBeat = s.startBeat + timeDelta;
        note.midiNote = rowToNote (noteToRow (s.midiNote) + rowDelta);
        note.label = getNoteName (note.midiNote);
    }
    refreshDisplay();
}

void PianoRollComponent::endMove() { moving.reset(); }

// note resizing

void PianoRollComponent::startResize (size_t index, Edge edge)
{
    auto& n = notes[index];
    resizing = ResizeInfo { n.id, n.startBeat, n.startBeat + n.lengthBeats, edge };
}

void PianoRollComponent::updateResize (juce::Point<int> pos)
{
    using namespace PianoRoll;
    if (! resizing.has_value()) return;

    auto idx = findNoteById (resizing->noteId);
    if (! idx.has_value()) return;

    auto& n = notes[*idx];
    double step = getStepBeats();
    double raw = juce::jlimit (0.0, (double) numBeats, pos.x / (double) beatWidth);
    double minLen = step;

    if (resizing->edge == Edge::right)
    {
        double end = std::ceil (raw / step) * step;
        end = juce::jlimit (resizing->startBeat + minLen, (double) numBeats, end);
        n.startBeat = resizing->startBeat;
        n.lengthBeats = juce::jmax (minLen, end - resizing->startBeat);
    }
    else
    {
        double start = std::floor (raw / step) * step;
        start = juce::jlimit (0.0, resizing->endBeat - minLen, start);
        n.startBeat = start;
        n.lengthBeats = juce::jmax (minLen, resizing->endBeat - start);
    }
    refreshDisplay();
}

void PianoRollComponent::endResize() { resizing.reset(); }

// note drawing

void PianoRollComponent::startDraw (juce::Point<int> pos)
{
    using namespace PianoRoll;
    int row = juce::jlimit (0, numRows - 1, pos.y / rowHeight);
    double raw = juce::jlimit (0.0, (double) numBeats, pos.x / (double) beatWidth);
    double step = getStepBeats();
    double start = std::floor (raw / step) * step;
    start = juce::jlimit (0.0, (double) numBeats - step, start);
    int midi = rowToNote (row);

    drawingNote = PianoRoll::Note { nextId++, start, step, midi, getNoteName (midi) };
    refreshDisplay();
}

void PianoRollComponent::updateDraw (juce::Point<int> pos)
{
    using namespace PianoRoll;
    if (! drawingNote.has_value()) return;

    double step = getStepBeats();
    double raw = juce::jlimit (0.0, (double) numBeats, pos.x / (double) beatWidth);
    double end = std::ceil (raw / step) * step;
    double minEnd = drawingNote->startBeat + step;
    end = juce::jlimit (minEnd, (double) numBeats, end);

    drawingNote->lengthBeats = juce::jmax (step, end - drawingNote->startBeat);
    refreshDisplay();
}

void PianoRollComponent::commitDraw()
{
    if (! drawingNote.has_value()) return;

    auto newNote = *drawingNote;
    drawingNote.reset();

    // Check for duplicates
    bool isDuplicate = std::any_of (notes.begin(), notes.end(), [&] (const PianoRoll::Note& n) {
        return n.midiNote == newNote.midiNote
            && std::abs (n.startBeat - newNote.startBeat) < 0.0001
            && std::abs (n.lengthBeats - newNote.lengthBeats) < 0.0001;
    });

    if (! isDuplicate)
    {
        notes.push_back (newNote);
        std::sort (notes.begin(), notes.end(), [] (auto& a, auto& b) {
            return std::abs (a.startBeat - b.startBeat) < 0.0001
                       ? a.midiNote > b.midiNote
                       : a.startBeat < b.startBeat;
        });
        setSelection ({ newNote.id });
    }
    refreshDisplay();
}

// note erasing

void PianoRollComponent::eraseAt (juce::Point<int> pos)
{
    using namespace PianoRoll;
    int row = juce::jlimit (0, numRows - 1, pos.y / rowHeight);
    double beat = pos.x / (double) beatWidth;
    int midi = rowToNote (row);

    auto before = notes.size();
    notes.erase (std::remove_if (notes.begin(), notes.end(), [&] (auto& n) {
        return n.midiNote == midi && beat >= n.startBeat && beat <= n.startBeat + n.lengthBeats;
    }), notes.end());

    if (notes.size() != before)
    {
        // Clean up selection for removed notes
        for (auto it = selectedIds.begin(); it != selectedIds.end();)
        {
            bool exists = std::any_of (notes.begin(), notes.end(), [id = *it] (auto& n) { return n.id == id; });
            it = exists ? std::next (it) : selectedIds.erase (it);
        }
        refreshDisplay();
    }
}

void PianoRollComponent::deleteSelected()
{
    if (selectedIds.empty()) return;

    notes.erase (std::remove_if (notes.begin(), notes.end(),
                                 [this] (auto& n) { return selectedIds.count (n.id) > 0; }),
                 notes.end());
    setSelection ({});
    refreshDisplay();
}

// keyboard shortcuts

bool PianoRollComponent::keyPressed (const juce::KeyPress& key)
{
    int code = key.getKeyCode();

    if (code == juce::KeyPress::deleteKey || code == juce::KeyPress::backspaceKey)
    {
        deleteSelected();
        return true;
    }

    if (selectedIds.empty()) return false;

    double beatStep = getStepBeats();

    if (code == juce::KeyPress::leftKey)  { nudgeSelected (-beatStep, 0); return true; }
    if (code == juce::KeyPress::rightKey) { nudgeSelected (beatStep, 0);  return true; }
    if (code == juce::KeyPress::upKey)    { nudgeSelected (0.0, -1);      return true; }
    if (code == juce::KeyPress::downKey)  { nudgeSelected (0.0, 1);       return true; }

    return false;
}

void PianoRollComponent::nudgeSelected (double beatDelta, int rowDelta)
{
    using namespace PianoRoll;
    if (selectedIds.empty()) return;

    // Clamp deltas so nothing goes out of bounds
    for (auto& n : notes)
    {
        if (! selectedIds.count (n.id)) continue;
        beatDelta = juce::jlimit (-n.startBeat, (double) numBeats - (n.startBeat + n.lengthBeats), beatDelta);
        int row = noteToRow (n.midiNote);
        rowDelta = juce::jlimit (-row, (numRows - 1) - row, rowDelta);
    }

    for (auto& n : notes)
    {
        if (! selectedIds.count (n.id)) continue;
        n.startBeat += beatDelta;
        n.midiNote = rowToNote (noteToRow (n.midiNote) + rowDelta);
        n.label = getNoteName (n.midiNote);
    }
    refreshDisplay();
}

// helpers

juce::Point<int> PianoRollComponent::toGridPos (juce::Point<int> local) const
{
    return { local.x - gridArea.getX() + scrollX,
             local.y - gridArea.getY() + scrollY };
}

std::optional<juce::Rectangle<int>> PianoRollComponent::getMarqueeRect() const
{
    if (! marqueeState.has_value()) return std::nullopt;

    // Convert content-space marquee to screen-space for painting
    int ox = gridArea.getX() - scrollX;
    int oy = gridArea.getY() - scrollY;
    return juce::Rectangle<int>::leftTopRightBottom (
        juce::jmin (marqueeState->start.x, marqueeState->current.x) + ox,
        juce::jmin (marqueeState->start.y, marqueeState->current.y) + oy,
        juce::jmax (marqueeState->start.x, marqueeState->current.x) + ox,
        juce::jmax (marqueeState->start.y, marqueeState->current.y) + oy);
}

void PianoRollComponent::handleScroll (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    float hDelta = std::abs (wheel.deltaX) > 0.0f ? wheel.deltaX
                                                   : (wheel.isReversed ? -wheel.deltaY : wheel.deltaY);
    float vDelta = wheel.isReversed ? -wheel.deltaY : wheel.deltaY;

    if (std::abs (wheel.deltaX) > 0.0f)
        scrollX -= (int) std::round (hDelta * 120.0f);
    else if (e.mods.isShiftDown())
        scrollX -= (int) std::round (vDelta * 120.0f);
    else
        scrollY -= (int) std::round (vDelta * 120.0f);

    updateViewport();
}

void PianoRollComponent::updateScrollbars()
{
    using namespace PianoRoll;
    int cw = numBeats * beatWidth;
    int ch = numRows * rowHeight;

    hScroll.setRangeLimits (0.0, (double) cw);
    hScroll.setCurrentRange ((double) scrollX, (double) scrollX + gridArea.getWidth());
    hScroll.setSingleStepSize (getStepBeats() * beatWidth);

    vScroll.setRangeLimits (0.0, (double) ch);
    vScroll.setCurrentRange ((double) scrollY, (double) scrollY + gridArea.getHeight());
    vScroll.setSingleStepSize ((double) rowHeight);
}

void PianoRollComponent::updateViewport()
{
    using namespace PianoRoll;
    int maxX = juce::jmax (0, numBeats * beatWidth - gridArea.getWidth());
    int maxY = juce::jmax (0, numRows * rowHeight - gridArea.getHeight());

    scrollX = juce::jlimit (0, maxX, scrollX);
    scrollY = juce::jlimit (0, maxY, scrollY);

    hScroll.setCurrentRangeStart ((double) scrollX);
    vScroll.setCurrentRangeStart ((double) scrollY);
    repaint();
}

void PianoRollComponent::refreshDisplay()
{
    updateScrollbars();
    repaint();
}

double PianoRollComponent::getStepBeats() const
{
    return 1.0; // always snap to 1/4 note
}

// PianoRollWindow

PianoRollWindow::PianoRollWindow()
    : juce::DocumentWindow ("Piano Roll",
                            juce::Colour::fromRGB (15, 17, 22),
                            juce::DocumentWindow::closeButton)
{
    setUsingNativeTitleBar (true);
    setResizable (true, true);
    setResizeLimits (800, 500, 1800, 1100);
    content = new PianoRollComponent();
    setContentOwned (content, false);
    centreWithSize (1100, 700);
}

void PianoRollWindow::setNotes (std::vector<PianoRoll::Note> notes)
{
    if (content != nullptr)
        content->setNotes (std::move (notes));
}

void PianoRollWindow::setOnSavePattern (std::function<void (const std::vector<PianoRoll::Note>&)> cb)
{
    if (content != nullptr)
        content->setOnSavePattern (std::move (cb));
}

void PianoRollWindow::setOnGetAvailableInstruments (std::function<juce::StringArray()> cb)
{
    if (content != nullptr)
        content->setOnGetAvailableInstruments (std::move (cb));
}

void PianoRollWindow::setOnInstrumentChanged (std::function<void (const juce::String&)> cb)
{
    if (content != nullptr)
        content->setOnInstrumentChanged (std::move (cb));
}

void PianoRollWindow::setInstrumentName (const juce::String& name)
{
    if (content != nullptr)
        content->setInstrumentName (name);
}

void PianoRollWindow::setThemeAssets (juce::Image newHeaderSpiceImage,
                                      juce::Image newBodySpiceImage,
                                      juce::Colour newAccentColour)
{
    if (content != nullptr)
        content->setThemeAssets (std::move (newHeaderSpiceImage),
                                 std::move (newBodySpiceImage),
                                 newAccentColour);
}

void PianoRollWindow::closeButtonPressed()
{
    setVisible (false);
}
