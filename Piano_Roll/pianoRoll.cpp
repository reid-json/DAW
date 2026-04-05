#include "pianoRoll.h"

#include <algorithm>
#include <cmath>

namespace PianoRoll
{
    juce::Rectangle<float> getNoteBounds (const Note& note)
    {
        const auto row = getRowForMidiNote (note.midiNote);
        const auto x = (float) (note.startBeat * beatWidth);
        const auto y = (float) (row * noteRowHeight);
        const auto w = (float) juce::jmax (12.0, note.lengthBeats * beatWidth);

        return { x + 1.5f, y + 2.0f, w - 3.0f, (float) noteRowHeight - 4.0f };
    }

    juce::String getNoteName (int midiNote)
    {
        static const char* names[] =
        {
            "C", "C#", "D", "D#", "E", "F",
            "F#", "G", "G#", "A", "A#", "B"
        };

        const auto octave = (midiNote / 12) - 1;
        return juce::String (names[midiNote % 12]) + juce::String (octave);
    }

    bool isBlackKey (int midiNote)
    {
        switch (midiNote % 12)
        {
            case 1: case 3: case 6: case 8: case 10: return true;
            default: return false;
        }
    }

    int getRowForMidiNote (int midiNote)
    {
        return topMidiNote - midiNote;
    }

    int getMidiNoteForRow (int row)
    {
        return topMidiNote - row;
    }
}

//==================================================
PianoKeyboardStrip::PianoKeyboardStrip()
{
    setOpaque (true);
}

void PianoKeyboardStrip::setVerticalScrollOffset (int newScrollOffset)
{
    verticalScrollOffset = newScrollOffset;
    repaint();
}

void PianoKeyboardStrip::paint (juce::Graphics& g)
{
    using namespace PianoRoll;

    g.fillAll (juce::Colour::fromRGB (232, 235, 240));

    // White keys
    for (int row = 0; row < totalRows; ++row)
    {
        const int midiNote = topMidiNote - row;
        const int y = row * noteRowHeight - verticalScrollOffset;

        if (! isBlackKey (midiNote) && y < getHeight() && y + noteRowHeight > 0)
        {
            auto whiteKey = juce::Rectangle<int> (0, y, getWidth(), noteRowHeight);

            g.setColour (juce::Colour::fromRGB (242, 244, 247));
            g.fillRect (whiteKey);

            g.setColour (juce::Colour::fromRGB (185, 190, 198));
            g.drawRect (whiteKey, 1);

            g.setColour (juce::Colours::black.withAlpha (0.78f));
            g.setFont (12.0f);
            g.drawText (getNoteName (midiNote),
                        10, y, getWidth() - 12, noteRowHeight,
                        juce::Justification::centredLeft);
        }
    }

    // Black keys
    for (int row = 0; row < totalRows; ++row)
    {
        const int midiNote = topMidiNote - row;
        const int y = row * noteRowHeight - verticalScrollOffset;

        if (isBlackKey (midiNote) && y < getHeight() && y + noteRowHeight > 0)
        {
            auto blackKey = juce::Rectangle<float> (0.0f, (float) y + 1.0f,
                                                    (float) getWidth() * 0.72f,
                                                    (float) noteRowHeight - 2.0f);

            g.setColour (juce::Colour::fromRGB (36, 40, 48));
            g.fillRoundedRectangle (blackKey, 3.0f);

            g.setColour (juce::Colour::fromRGB (66, 72, 82));
            g.drawRoundedRectangle (blackKey, 3.0f, 1.0f);

            g.setColour (juce::Colours::white.withAlpha (0.88f));
            g.setFont (12.0f);
            g.drawText (getNoteName (midiNote),
                        juce::Rectangle<int> ((int) blackKey.getX() + 10, y,
                                              (int) blackKey.getWidth() - 12, noteRowHeight),
                        juce::Justification::centredLeft);
        }
    }

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawVerticalLine (getWidth() - 1, 0.0f, (float) getHeight());
}

//==================================================
PianoGridComponent::PianoGridComponent()
{
    setOpaque (true);
}

void PianoGridComponent::setNotes (std::vector<PianoRoll::Note> newNotes)
{
    notes = std::move (newNotes);
    repaint();
}

void PianoGridComponent::setSelectedNoteIds (std::set<int> newSelectedNoteIds)
{
    selectedNoteIds = std::move (newSelectedNoteIds);
    repaint();
}

void PianoGridComponent::setMarqueeRect (std::optional<juce::Rectangle<int>> newMarqueeRect)
{
    marqueeRect = std::move (newMarqueeRect);
    repaint();
}

void PianoGridComponent::setScrollOffsets (int newHorizontalOffset, int newVerticalOffset)
{
    horizontalScrollOffset = newHorizontalOffset;
    verticalScrollOffset = newVerticalOffset;
    repaint();
}

void PianoGridComponent::setMouseDownCallback (std::function<void (const juce::MouseEvent&)> callback)
{
    mouseDownCallback = std::move (callback);
}

void PianoGridComponent::setMouseDragCallback (std::function<void (const juce::MouseEvent&)> callback)
{
    mouseDragCallback = std::move (callback);
}

void PianoGridComponent::setMouseUpCallback (std::function<void (const juce::MouseEvent&)> callback)
{
    mouseUpCallback = std::move (callback);
}

void PianoGridComponent::setMouseWheelCallback (std::function<void (const juce::MouseEvent&, const juce::MouseWheelDetails&)> callback)
{
    mouseWheelCallback = std::move (callback);
}

void PianoGridComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (15, 17, 22));
    paintGrid (g);
    paintNotes (g);
    paintMarquee (g);
    paintPlayhead (g);
}

void PianoGridComponent::mouseDown (const juce::MouseEvent& event)
{
    if (mouseDownCallback != nullptr)
        mouseDownCallback (event);
}

void PianoGridComponent::mouseDrag (const juce::MouseEvent& event)
{
    if (mouseDragCallback != nullptr)
        mouseDragCallback (event);
}

void PianoGridComponent::mouseUp (const juce::MouseEvent& event)
{
    if (mouseUpCallback != nullptr)
        mouseUpCallback (event);
}

void PianoGridComponent::mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    if (mouseWheelCallback != nullptr)
        mouseWheelCallback (event, wheel);
    else
        juce::Component::mouseWheelMove (event, wheel);
}

void PianoGridComponent::paintGrid (juce::Graphics& g)
{
    using namespace PianoRoll;

    for (int row = 0; row < totalRows; ++row)
    {
        const int midiNote = topMidiNote - row;
        const int y = row * noteRowHeight - verticalScrollOffset;

        if (y >= getHeight() || y + noteRowHeight <= 0)
            continue;

        g.setColour (isBlackKey (midiNote)
                         ? juce::Colour::fromRGB (20, 24, 33)
                         : juce::Colour::fromRGB (24, 29, 40));
        g.fillRect (0, y, getWidth(), noteRowHeight);

        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.drawHorizontalLine (y, 0.0f, (float) getWidth());
    }

    for (int beat = 0; beat <= totalBeats; ++beat)
    {
        const int x = beat * beatWidth - horizontalScrollOffset;
        const bool major = (beat % 4 == 0);

        if (x < -beatWidth || x > getWidth() + beatWidth)
            continue;

        g.setColour (major
                         ? juce::Colours::white.withAlpha (0.14f)
                         : juce::Colours::white.withAlpha (0.07f));
        g.drawVerticalLine (x, 0.0f, (float) getHeight());

        if (beat < totalBeats)
        {
            const int subX = x + beatWidth / 2;
            g.setColour (juce::Colours::white.withAlpha (0.035f));
            g.drawVerticalLine (subX, 0.0f, (float) getHeight());
        }
    }
}

void PianoGridComponent::paintNotes (juce::Graphics& g)
{
    using namespace PianoRoll;

    for (const auto& note : notes)
    {
        const int row = getRowForMidiNote (note.midiNote);

        if (row < 0 || row >= totalRows)
            continue;

        auto r = getNoteBounds (note).translated ((float) -horizontalScrollOffset,
                                                  (float) -verticalScrollOffset);
        const auto isSelected = selectedNoteIds.contains (note.id);
        g.setColour (isSelected ? juce::Colour::fromRGB (88, 150, 255)
                                : juce::Colour::fromRGB (58, 122, 254));
        g.fillRoundedRectangle (r, 6.0f);

        g.setColour (isSelected ? juce::Colours::white.withAlpha (0.50f)
                                : juce::Colours::white.withAlpha (0.18f));
        g.drawRoundedRectangle (r, 6.0f, isSelected ? 1.5f : 1.0f);

        if (r.getWidth() > 28.0f)
        {
            g.setColour (juce::Colours::white.withAlpha (0.95f));
            g.setFont (12.0f);
            g.drawText (note.label, r.toNearestInt().reduced (8, 0),
                        juce::Justification::centredLeft, false);
        }
    }
}

void PianoGridComponent::paintMarquee (juce::Graphics& g)
{
    if (! marqueeRect.has_value())
        return;

    g.setColour (juce::Colour::fromRGB (88, 150, 255).withAlpha (0.18f));
    g.fillRect (*marqueeRect);

    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.drawRect (*marqueeRect, 1);
}

void PianoGridComponent::paintPlayhead (juce::Graphics& g)
{
    using namespace PianoRoll;

    const int playheadBeat = 2;
    const int x = playheadBeat * beatWidth - horizontalScrollOffset;

    g.setColour (juce::Colour::fromRGB (58, 122, 254));
    g.drawLine ((float) x, 0.0f, (float) x, (float) getHeight(), 2.0f);
}

//==================================================
void PianoRollComponent::ScrollbarListener::scrollBarMoved (juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart)
{
    if (scrollBarThatHasMoved == &owner.horizontalScrollBar)
        owner.horizontalScrollOffset = (int) std::round (newRangeStart);
    else if (scrollBarThatHasMoved == &owner.verticalScrollBar)
        owner.verticalScrollOffset = (int) std::round (newRangeStart);

    owner.updateViewport();
}

//==================================================
PianoRollComponent::PianoRollComponent()
{
    addAndMakeVisible (keyboardStrip);
    addAndMakeVisible (gridComponent);
    addAndMakeVisible (horizontalScrollBar);
    addAndMakeVisible (verticalScrollBar);
    setOpaque (true);
    setWantsKeyboardFocus (true);
    setMouseCursor (juce::MouseCursor::NormalCursor);
    horizontalScrollBar.addListener (&scrollbarListener);
    verticalScrollBar.addListener (&scrollbarListener);
    horizontalScrollBar.setAutoHide (false);
    verticalScrollBar.setAutoHide (false);
    gridComponent.setMouseDownCallback ([this] (const juce::MouseEvent& event)
                                        {
                                            handleGridMouseDown (event);
                                        });
    gridComponent.setMouseDragCallback ([this] (const juce::MouseEvent& event)
                                        {
                                            handleGridMouseDrag (event);
                                        });
    gridComponent.setMouseUpCallback ([this] (const juce::MouseEvent& event)
                                      {
                                          handleGridMouseUp (event);
                                      });
    gridComponent.setMouseWheelCallback ([this] (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
                                         {
                                             handleMouseWheelScroll (event, wheel);
                                         });
}

void PianoRollComponent::setTrackContext (juce::String newTrackName, juce::String newPatternName)
{
    trackName = newTrackName;
    patternName = newPatternName;
    repaint();
}

void PianoRollComponent::setNotes (std::vector<PianoRoll::Note> newNotes)
{
    suppressNotesChanged = true;
    notes = std::move (newNotes);
    nextNoteId = 1;

    for (const auto& note : notes)
        nextNoteId = juce::jmax (nextNoteId, note.id + 1);

    selectedNoteIds.clear();
    pendingDrawNote.reset();
    resizeState.reset();
    moveState.reset();
    marqueeState.reset();
    refreshGridNotes();
    suppressNotesChanged = false;
}

void PianoRollComponent::setNotesChangedCallback (std::function<void (const std::vector<PianoRoll::Note>&)> callback)
{
    notesChangedCallback = std::move (callback);
}

void PianoRollComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (15, 17, 22));

    auto headerArea = getLocalBounds().removeFromTop (PianoRoll::headerHeight);
    paintToolbar (g, headerArea.removeFromTop (PianoRoll::toolbarHeight));
    paintTimeline (g, headerArea);
}

void PianoRollComponent::paintToolbar (juce::Graphics& g, juce::Rectangle<int> area)
{
    using namespace PianoRoll;

    g.setColour (juce::Colour::fromRGB (18, 23, 33));
    g.fillRect (area);

    auto contentArea = area.reduced (12, 8);
    auto leftArea = contentArea.removeFromLeft (260);
    auto rightArea = contentArea.removeFromRight (250);
    auto centerArea = contentArea;
    const auto layout = layoutToolbar (area);

    g.setColour (juce::Colours::white.withAlpha (0.95f));
    g.setFont (16.0f);
    g.drawText (trackName, leftArea.removeFromTop (18),
                juce::Justification::centredLeft);

    g.setColour (juce::Colours::white.withAlpha (0.52f));
    g.setFont (12.0f);
    g.drawText (patternName, leftArea,
                juce::Justification::centredLeft);

    auto toolButtons = centerArea.removeFromLeft (216);
    paintToolbarButton (g, layout.selectButton, "Select", activeTool == EditorTool::select);
    paintToolbarButton (g, layout.drawButton, "Draw", activeTool == EditorTool::draw);
    paintToolbarButton (g, layout.eraseButton, "Erase", activeTool == EditorTool::erase);

    centerArea.removeFromLeft (20);
    auto editButtons = centerArea.removeFromLeft (204);
    paintToolbarButton (g, editButtons.removeFromLeft (76).reduced (2, 0), "Quantize");
    paintToolbarButton (g, editButtons.removeFromLeft (78).reduced (2, 0), "Duplicate");
    paintToolbarButton (g, editButtons.removeFromLeft (54).reduced (2, 0), "Delete");

    rightArea.removeFromLeft (156);
    paintToolbarButton (g, layout.snapButton, snapEnabled ? "Snap On" : "Snap Off", snapEnabled, true);
    paintToolbarButton (g, layout.gridButton, getGridDivisionLabel());

    rightArea.removeFromLeft (8);
    auto zoomArea = rightArea.removeFromLeft (72);
    paintIconButton (g, zoomArea.removeFromLeft (32).reduced (2, 0), "-");
    paintIconButton (g, zoomArea.removeFromLeft (32).reduced (2, 0), "+");

    paintIconButton (g, rightArea.removeFromLeft (34).reduced (2, 0), "...");

    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawHorizontalLine (area.getBottom() - 1, 0.0f, (float) area.getRight());
}

void PianoRollComponent::paintTimeline (juce::Graphics& g, juce::Rectangle<int> area)
{
    using namespace PianoRoll;

    g.setColour (juce::Colour::fromRGB (22, 27, 38));
    g.fillRect (area);

    auto keyboardHeader = area.removeFromLeft (keyColumnWidth);
    g.setColour (juce::Colours::white.withAlpha (0.72f));
    g.setFont (13.0f);
    g.drawText ("Bars", keyboardHeader.reduced (12, 0),
                juce::Justification::centredLeft);

    auto timelineArea = area;
    const auto loopStartX = timelineArea.getX() + (4 * beatWidth) - horizontalScrollOffset;
    const auto loopWidth = 8 * beatWidth;
    auto loopRect = juce::Rectangle<float> ((float) loopStartX + 2.0f,
                                            (float) timelineArea.getY() + 6.0f,
                                            (float) loopWidth - 4.0f,
                                            6.0f);

    g.setColour (juce::Colour::fromRGB (58, 122, 254).withAlpha (0.35f));
    g.fillRoundedRectangle (loopRect, 3.0f);

    for (int beat = 0; beat <= totalBeats; ++beat)
    {
        const int x = timelineArea.getX() + beat * beatWidth - horizontalScrollOffset;
        const bool major = (beat % 4 == 0);

        if (x < timelineArea.getX() - beatWidth || x > timelineArea.getRight() + beatWidth)
            continue;

        g.setColour (major
                         ? juce::Colours::white.withAlpha (0.16f)
                         : juce::Colours::white.withAlpha (0.08f));
        g.drawVerticalLine (x, (float) timelineArea.getY(), (float) timelineArea.getBottom());

        if (beat < totalBeats)
        {
            const int subX = x + beatWidth / 2;
            g.setColour (juce::Colours::white.withAlpha (0.04f));
            g.drawVerticalLine (subX, (float) timelineArea.getY() + 18.0f, (float) timelineArea.getBottom());
        }

        if (beat < totalBeats)
        {
            g.setColour (juce::Colours::white.withAlpha (0.72f));
            g.setFont (12.0f);
            g.drawText (juce::String (beat + 1),
                        x + 4, timelineArea.getY() + 12, 28, 14,
                        juce::Justification::centredLeft);
        }
    }

    const int playheadBeat = 2;
    const int playheadX = timelineArea.getX() + playheadBeat * beatWidth - horizontalScrollOffset;
    g.setColour (juce::Colour::fromRGB (58, 122, 254));
    g.drawLine ((float) playheadX, (float) timelineArea.getY(),
                (float) playheadX, (float) timelineArea.getBottom(), 2.0f);

    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawHorizontalLine (timelineArea.getBottom() - 1, 0.0f, (float) getWidth());
    g.drawVerticalLine (keyColumnWidth - 1, (float) timelineArea.getY(), (float) timelineArea.getBottom());
}

void PianoRollComponent::paintToolbarButton (juce::Graphics& g,
                                             juce::Rectangle<int> area,
                                             const juce::String& text,
                                             bool isActive,
                                             bool isAccent)
{
    const auto background = isActive ? juce::Colour::fromRGB (58, 122, 254)
                                     : isAccent ? juce::Colour::fromRGB (29, 56, 94)
                                                : juce::Colour::fromRGB (28, 34, 46);
    const auto border = isActive ? juce::Colours::white.withAlpha (0.28f)
                                 : isAccent ? juce::Colour::fromRGB (79, 127, 194).withAlpha (0.55f)
                                            : juce::Colours::white.withAlpha (0.10f);

    g.setColour (background);
    g.fillRoundedRectangle (area.toFloat(), 7.0f);

    g.setColour (border);
    g.drawRoundedRectangle (area.toFloat(), 7.0f, 1.0f);

    g.setColour (juce::Colours::white.withAlpha (isActive ? 0.96f : 0.86f));
    g.setFont (12.0f);
    g.drawText (text, area, juce::Justification::centred);
}

void PianoRollComponent::paintIconButton (juce::Graphics& g,
                                          juce::Rectangle<int> area,
                                          const juce::String& text)
{
    paintToolbarButton (g, area, text, false, false);
}

void PianoRollComponent::resized()
{
    auto bounds = getLocalBounds();
    toolbarLayout = layoutToolbar (bounds.removeFromTop (PianoRoll::toolbarHeight));
    bounds.removeFromTop (PianoRoll::timelineHeight);

    constexpr int scrollbarThickness = 14;
    auto verticalBarBounds = bounds.removeFromRight (scrollbarThickness);
    auto horizontalBarBounds = bounds.removeFromBottom (scrollbarThickness);
    verticalBarBounds.removeFromBottom (scrollbarThickness);

    keyboardStrip.setBounds (bounds.removeFromLeft (PianoRoll::keyColumnWidth));
    gridComponent.setBounds (bounds);
    horizontalScrollBar.setBounds (horizontalBarBounds.withX (keyboardStrip.getRight())
                                                     .withWidth (gridComponent.getWidth()));
    verticalScrollBar.setBounds (verticalBarBounds.withY (gridComponent.getY())
                                                 .withHeight (gridComponent.getHeight()));
    updateScrollbars();
    updateViewport();
}

void PianoRollComponent::mouseMove (const juce::MouseEvent& event)
{
    if (isToolbarInteractiveArea (event.getPosition()))
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        return;
    }

    setMouseCursor (juce::MouseCursor::NormalCursor);
}

void PianoRollComponent::mouseExit (const juce::MouseEvent&)
{
    setMouseCursor (juce::MouseCursor::NormalCursor);
}

void PianoRollComponent::mouseDown (const juce::MouseEvent& event)
{
    handleToolbarClick (event.getPosition());
}

void PianoRollComponent::mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    handleMouseWheelScroll (event, wheel);
}

PianoRollComponent::ToolbarLayout PianoRollComponent::layoutToolbar (juce::Rectangle<int> area) const
{
    ToolbarLayout layout;

    auto contentArea = area.reduced (12, 8);
    contentArea.removeFromLeft (260);
    auto rightArea = contentArea.removeFromRight (250);
    auto centerArea = contentArea;

    auto toolButtons = centerArea.removeFromLeft (216);
    layout.selectButton = toolButtons.removeFromLeft (68).reduced (2, 0);
    layout.drawButton = toolButtons.removeFromLeft (64).reduced (2, 0);
    layout.eraseButton = toolButtons.removeFromLeft (64).reduced (2, 0);

    rightArea.removeFromLeft (2);
    layout.snapButton = rightArea.removeFromLeft (90).reduced (2, 0);
    layout.gridButton = rightArea.removeFromLeft (72).reduced (2, 0);

    return layout;
}

bool PianoRollComponent::isToolbarInteractiveArea (juce::Point<int> position) const
{
    return toolbarLayout.selectButton.contains (position)
        || toolbarLayout.drawButton.contains (position)
        || toolbarLayout.eraseButton.contains (position)
        || toolbarLayout.snapButton.contains (position)
        || toolbarLayout.gridButton.contains (position);
}

bool PianoRollComponent::handleToolbarClick (juce::Point<int> position)
{
    if (toolbarLayout.selectButton.contains (position))
    {
        if (activeTool != EditorTool::select)
        {
            activeTool = EditorTool::select;
            repaint (toolbarLayout.selectButton.getUnion (toolbarLayout.drawButton).getUnion (toolbarLayout.eraseButton));
        }

        return true;
    }

    if (toolbarLayout.drawButton.contains (position))
    {
        if (activeTool != EditorTool::draw)
        {
            activeTool = EditorTool::draw;
            repaint (toolbarLayout.selectButton.getUnion (toolbarLayout.drawButton).getUnion (toolbarLayout.eraseButton));
        }

        return true;
    }

    if (toolbarLayout.eraseButton.contains (position))
    {
        if (activeTool != EditorTool::erase)
        {
            activeTool = EditorTool::erase;
            repaint (toolbarLayout.selectButton.getUnion (toolbarLayout.drawButton).getUnion (toolbarLayout.eraseButton));
        }

        return true;
    }

    if (toolbarLayout.snapButton.contains (position))
    {
        snapEnabled = ! snapEnabled;
        repaint (toolbarLayout.snapButton);
        return true;
    }

    if (toolbarLayout.gridButton.contains (position))
    {
        showGridDivisionMenu();
        return true;
    }

    return false;
}

void PianoRollComponent::handleGridMouseDown (const juce::MouseEvent& event)
{
    grabKeyboardFocus();
    const auto contentPosition = getContentPosition (event.getPosition());

    if (activeTool == EditorTool::select)
    {
        if (const auto resizeTarget = findResizableNoteAt (contentPosition))
        {
            selectSingleNote (resizeTarget->first);
            beginNoteResize (resizeTarget->first, resizeTarget->second);
        }
        else if (const auto noteIndex = findNoteAt (contentPosition))
        {
            if (isToggleSelectionModifierDown (event.mods))
            {
                auto newSelection = selectedNoteIds;

                if (newSelection.contains (notes[*noteIndex].id))
                    newSelection.erase (notes[*noteIndex].id);
                else
                    newSelection.insert (notes[*noteIndex].id);

                setSelectedNoteIds (std::move (newSelection));
            }
            else if (event.mods.isShiftDown())
            {
                auto newSelection = selectedNoteIds;
                newSelection.insert (notes[*noteIndex].id);
                setSelectedNoteIds (std::move (newSelection));
            }
            else if (! selectedNoteIds.contains (notes[*noteIndex].id))
            {
                selectSingleNote (noteIndex);
            }

            if (! event.mods.isShiftDown() && ! isToggleSelectionModifierDown (event.mods))
                beginNoteMove (*noteIndex, contentPosition);
        }
        else
        {
            beginMarqueeSelection (event);
        }

        return;
    }

    if (activeTool == EditorTool::draw)
    {
        beginNoteDrawAt (contentPosition);
        return;
    }

    if (activeTool == EditorTool::erase)
        eraseNoteAt (contentPosition);
}

void PianoRollComponent::handleGridMouseDrag (const juce::MouseEvent& event)
{
    const auto contentPosition = getContentPosition (event.getPosition());

    if (activeTool == EditorTool::select && marqueeState.has_value())
    {
        updateMarqueeSelection (contentPosition);
        return;
    }

    if (activeTool == EditorTool::select && resizeState.has_value())
    {
        updateNoteResize (contentPosition);
        return;
    }

    if (activeTool == EditorTool::select && moveState.has_value())
    {
        updateNoteMove (contentPosition);
        return;
    }

    if (activeTool == EditorTool::draw && pendingDrawNote.has_value())
        updatePendingDrawNote (contentPosition);
}

void PianoRollComponent::handleGridMouseUp (const juce::MouseEvent& event)
{
    const auto contentPosition = getContentPosition (event.getPosition());

    if (activeTool == EditorTool::select && marqueeState.has_value())
    {
        updateMarqueeSelection (contentPosition);
        endMarqueeSelection();
        return;
    }

    if (activeTool == EditorTool::select && resizeState.has_value())
    {
        updateNoteResize (contentPosition);
        endNoteResize();
        return;
    }

    if (activeTool == EditorTool::select && moveState.has_value())
    {
        updateNoteMove (contentPosition);
        endNoteMove();
        return;
    }

    if (activeTool != EditorTool::draw || ! pendingDrawNote.has_value())
        return;

    updatePendingDrawNote (contentPosition);
    commitPendingDrawNote();
}

std::optional<size_t> PianoRollComponent::findNoteAt (juce::Point<int> position) const
{
    using namespace PianoRoll;

    const auto clickedBeat = position.x / (double) beatWidth;
    const auto row = juce::jlimit (0, totalRows - 1, position.y / noteRowHeight);
    const auto midiNote = getMidiNoteForRow (row);

    for (size_t i = notes.size(); i-- > 0;)
    {
        const auto& note = notes[i];
        const auto noteEndBeat = note.startBeat + note.lengthBeats;

        if (note.midiNote == midiNote
            && clickedBeat >= note.startBeat
            && clickedBeat <= noteEndBeat)
            return i;
    }

    return std::nullopt;
}

std::optional<std::pair<size_t, PianoRollComponent::ResizeEdge>> PianoRollComponent::findResizableNoteAt (juce::Point<int> position) const
{
    using namespace PianoRoll;

    const auto noteIndex = findNoteAt (position);

    if (! noteIndex.has_value())
        return std::nullopt;

    const auto clickedBeat = position.x / (double) beatWidth;
    const auto& note = notes[*noteIndex];
    const auto noteStartBeat = note.startBeat;
    const auto noteEndBeat = note.startBeat + note.lengthBeats;
    const auto edgeThresholdBeats = 8.0 / (double) beatWidth;

    if (std::abs (clickedBeat - noteStartBeat) <= edgeThresholdBeats)
        return std::make_pair (*noteIndex, ResizeEdge::left);

    if (std::abs (clickedBeat - noteEndBeat) <= edgeThresholdBeats)
        return std::make_pair (*noteIndex, ResizeEdge::right);

    return std::nullopt;
}

std::optional<size_t> PianoRollComponent::findNoteIndexById (int noteId) const
{
    for (size_t i = 0; i < notes.size(); ++i)
        if (notes[i].id == noteId)
            return i;

    return std::nullopt;
}

void PianoRollComponent::setSelectedNoteIds (std::set<int> noteIds)
{
    selectedNoteIds = std::move (noteIds);
    gridComponent.setSelectedNoteIds (selectedNoteIds);
}

void PianoRollComponent::selectSingleNote (std::optional<size_t> noteIndex)
{
    if (! noteIndex.has_value())
    {
        setSelectedNoteIds ({});
        return;
    }

    setSelectedNoteIds ({ notes[*noteIndex].id });
}

void PianoRollComponent::beginMarqueeSelection (const juce::MouseEvent& event)
{
    const auto contentPosition = getContentPosition (event.getPosition());
    marqueeState = MarqueeState { contentPosition,
                                  contentPosition,
                                  event.mods.isShiftDown() ? selectedNoteIds : std::set<int> {} };

    if (! event.mods.isShiftDown())
        setSelectedNoteIds ({});

    gridComponent.setMarqueeRect (getMarqueeRect());
}

void PianoRollComponent::updateMarqueeSelection (juce::Point<int> position)
{
    if (! marqueeState.has_value())
        return;

    marqueeState->current = position;

    std::set<int> marqueeSelection = marqueeState->baseSelectionIds;
    const auto marquee = getMarqueeRectInContentSpace();

    if (marquee.has_value())
    {
        const auto marqueeBounds = marquee->toFloat();

        for (const auto& note : notes)
        {
            if (PianoRoll::getNoteBounds (note).intersects (marqueeBounds))
                marqueeSelection.insert (note.id);
        }
    }

    setSelectedNoteIds (std::move (marqueeSelection));
    gridComponent.setMarqueeRect (getMarqueeRect());
}

void PianoRollComponent::endMarqueeSelection()
{
    marqueeState.reset();
    gridComponent.setMarqueeRect (std::nullopt);
}

void PianoRollComponent::beginNoteMove (size_t noteIndex, juce::Point<int> position)
{
    using namespace PianoRoll;

    const auto& note = notes[noteIndex];
    const auto clickedBeat = position.x / (double) beatWidth;
    const auto clickedRow = juce::jlimit (0, totalRows - 1, position.y / noteRowHeight);
    const auto noteRow = getRowForMidiNote (note.midiNote);

    std::vector<MoveNoteSnapshot> snapshots;

    for (const auto& candidate : notes)
        if (selectedNoteIds.contains (candidate.id))
            snapshots.push_back ({ candidate.id, candidate.startBeat, candidate.midiNote, candidate.label, candidate.lengthBeats });

    moveState = MoveState { note.id,
                            clickedBeat - note.startBeat,
                            clickedRow - noteRow,
                            note.startBeat,
                            note.midiNote,
                            std::move (snapshots) };
}

void PianoRollComponent::updateNoteMove (juce::Point<int> position)
{
    using namespace PianoRoll;

    if (! moveState.has_value())
        return;

    const auto stepBeats = getGridStepBeats();
    const auto rawBeat = (position.x / (double) beatWidth) - moveState->pointerOffsetBeats;
    const auto snappedBeat = std::round (rawBeat / stepBeats) * stepBeats;
    auto timeDelta = (snapEnabled ? snappedBeat : rawBeat) - moveState->anchorStartBeat;

    const auto pointerRow = juce::jlimit (0, totalRows - 1, position.y / noteRowHeight);
    const auto rawRow = pointerRow - moveState->pointerRowOffset;
    auto rowDelta = rawRow - getRowForMidiNote (moveState->anchorMidiNote);

    for (const auto& snapshot : moveState->snapshots)
    {
        const auto minTimeDelta = -snapshot.startBeat;
        const auto maxTimeDelta = (double) totalBeats - (snapshot.startBeat + snapshot.lengthBeats);
        timeDelta = juce::jlimit (minTimeDelta, maxTimeDelta, timeDelta);

        const auto snapshotRow = getRowForMidiNote (snapshot.midiNote);
        const auto minRowDelta = -snapshotRow;
        const auto maxRowDelta = (totalRows - 1) - snapshotRow;
        rowDelta = juce::jlimit (minRowDelta, maxRowDelta, rowDelta);
    }

    for (const auto& snapshot : moveState->snapshots)
    {
        const auto noteIndex = findNoteIndexById (snapshot.noteId);

        if (! noteIndex.has_value())
            continue;

        auto& note = notes[*noteIndex];
        note.startBeat = snapshot.startBeat + timeDelta;
        note.midiNote = getMidiNoteForRow (getRowForMidiNote (snapshot.midiNote) + rowDelta);
        note.label = getNoteName (note.midiNote);
    }

    refreshGridNotes();
}

void PianoRollComponent::endNoteMove()
{
    moveState.reset();
}

void PianoRollComponent::beginNoteResize (size_t noteIndex, ResizeEdge edge)
{
    const auto& note = notes[noteIndex];
    resizeState = ResizeState { note.id, note.startBeat, note.startBeat + note.lengthBeats, edge };
}

void PianoRollComponent::updateNoteResize (juce::Point<int> position)
{
    using namespace PianoRoll;

    if (! resizeState.has_value())
        return;

    const auto noteIndex = findNoteIndexById (resizeState->noteId);

    if (! noteIndex.has_value())
        return;

    auto& note = notes[*noteIndex];
    const auto stepBeats = getGridStepBeats();
    const auto rawBeat = juce::jlimit (0.0, (double) totalBeats, position.x / (double) beatWidth);
    const auto minimumLength = snapEnabled ? stepBeats : (1.0 / (double) beatWidth);

    if (resizeState->edge == ResizeEdge::right)
    {
        const auto snappedEndBeat = std::ceil (rawBeat / stepBeats) * stepBeats;
        const auto targetEndBeat = snapEnabled ? snappedEndBeat : rawBeat;
        const auto clampedEndBeat = juce::jlimit (resizeState->startBeat + minimumLength,
                                                  (double) totalBeats,
                                                  juce::jmax (resizeState->startBeat + minimumLength, targetEndBeat));

        note.startBeat = resizeState->startBeat;
        note.lengthBeats = juce::jmax (minimumLength, clampedEndBeat - resizeState->startBeat);
    }
    else
    {
        const auto snappedStartBeat = std::floor (rawBeat / stepBeats) * stepBeats;
        const auto targetStartBeat = snapEnabled ? snappedStartBeat : rawBeat;
        const auto clampedStartBeat = juce::jlimit (0.0,
                                                    resizeState->endBeat - minimumLength,
                                                    juce::jmin (resizeState->endBeat - minimumLength, targetStartBeat));

        note.startBeat = clampedStartBeat;
        note.lengthBeats = juce::jmax (minimumLength, resizeState->endBeat - clampedStartBeat);
    }

    refreshGridNotes();
}

void PianoRollComponent::endNoteResize()
{
    resizeState.reset();
}

void PianoRollComponent::beginNoteDrawAt (juce::Point<int> position)
{
    using namespace PianoRoll;

    const auto row = juce::jlimit (0, totalRows - 1, position.y / noteRowHeight);
    const auto rawBeat = juce::jlimit (0.0, (double) totalBeats, position.x / (double) beatWidth);
    const auto stepBeats = getGridStepBeats();
    const auto snappedBeat = std::floor (rawBeat / stepBeats) * stepBeats;
    const auto startBeat = snapEnabled ? snappedBeat : rawBeat;
    const auto clampedStartBeat = juce::jlimit (0.0, juce::jmax (0.0, (double) totalBeats - stepBeats), startBeat);
    const auto midiNote = getMidiNoteForRow (row);
    pendingDrawNote = Note { nextNoteId++, clampedStartBeat, stepBeats, midiNote, getNoteName (midiNote) };
    refreshGridNotes();
}

void PianoRollComponent::updatePendingDrawNote (juce::Point<int> position)
{
    using namespace PianoRoll;

    if (! pendingDrawNote.has_value())
        return;

    const auto stepBeats = getGridStepBeats();
    const auto rawBeat = juce::jlimit (0.0, (double) totalBeats, position.x / (double) beatWidth);
    const auto snappedEndBeat = std::ceil (rawBeat / stepBeats) * stepBeats;
    const auto targetEndBeat = snapEnabled ? snappedEndBeat : rawBeat;
    const auto minimumEndBeat = pendingDrawNote->startBeat + stepBeats;
    const auto clampedEndBeat = juce::jlimit (minimumEndBeat, (double) totalBeats, juce::jmax (minimumEndBeat, targetEndBeat));

    pendingDrawNote->lengthBeats = juce::jmax (stepBeats, clampedEndBeat - pendingDrawNote->startBeat);
    refreshGridNotes();
}

void PianoRollComponent::commitPendingDrawNote()
{
    if (! pendingDrawNote.has_value())
        return;

    const auto newNote = *pendingDrawNote;
    pendingDrawNote.reset();

    const auto duplicatesExisting = [&newNote] (const PianoRoll::Note& note)
    {
        return note.midiNote == newNote.midiNote
            && std::abs (note.startBeat - newNote.startBeat) < 0.0001
            && std::abs (note.lengthBeats - newNote.lengthBeats) < 0.0001;
    };

    if (! std::any_of (notes.begin(), notes.end(), duplicatesExisting))
    {
        notes.push_back (newNote);
        std::sort (notes.begin(), notes.end(),
                   [] (const PianoRoll::Note& a, const PianoRoll::Note& b)
                   {
                       if (std::abs (a.startBeat - b.startBeat) < 0.0001)
                           return a.midiNote > b.midiNote;

                       return a.startBeat < b.startBeat;
                   });
        setSelectedNoteIds ({ newNote.id });
    }

    refreshGridNotes();
}

void PianoRollComponent::eraseNoteAt (juce::Point<int> position)
{
    using namespace PianoRoll;

    const auto row = juce::jlimit (0, totalRows - 1, position.y / noteRowHeight);
    const auto clickedBeat = position.x / (double) beatWidth;
    const auto midiNote = getMidiNoteForRow (row);
    const auto originalSize = notes.size();

    notes.erase (std::remove_if (notes.begin(), notes.end(),
                                 [clickedBeat, midiNote] (const Note& note)
                                 {
                                     const auto noteEnd = note.startBeat + note.lengthBeats;
                                     return note.midiNote == midiNote
                                         && clickedBeat >= note.startBeat
                                         && clickedBeat <= noteEnd;
                                 }),
                 notes.end());

    if (notes.size() != originalSize)
    {
        for (auto it = selectedNoteIds.begin(); it != selectedNoteIds.end();)
        {
            if (std::none_of (notes.begin(), notes.end(), [id = *it] (const Note& note) { return note.id == id; }))
                it = selectedNoteIds.erase (it);
            else
                ++it;
        }

        refreshGridNotes();
    }
}

void PianoRollComponent::deleteSelectedNotes()
{
    if (selectedNoteIds.empty())
        return;

    notes.erase (std::remove_if (notes.begin(), notes.end(),
                                 [this] (const PianoRoll::Note& note)
                                 {
                                     return selectedNoteIds.contains (note.id);
                                 }),
                 notes.end());

    setSelectedNoteIds ({});
    refreshGridNotes();
}

bool PianoRollComponent::keyPressed (const juce::KeyPress& key)
{
    const auto keyCode = key.getKeyCode();

    if (keyCode == juce::KeyPress::deleteKey || keyCode == juce::KeyPress::backspaceKey)
    {
        deleteSelectedNotes();
        return true;
    }

    if (selectedNoteIds.empty())
        return false;

    if (keyCode == juce::KeyPress::leftKey)
    {
        nudgeSelectedNotes (snapEnabled ? -getGridStepBeats() : -(1.0 / (double) PianoRoll::beatWidth), 0);
        return true;
    }

    if (keyCode == juce::KeyPress::rightKey)
    {
        nudgeSelectedNotes (snapEnabled ? getGridStepBeats() : (1.0 / (double) PianoRoll::beatWidth), 0);
        return true;
    }

    if (keyCode == juce::KeyPress::upKey)
    {
        nudgeSelectedNotes (0.0, -1);
        return true;
    }

    if (keyCode == juce::KeyPress::downKey)
    {
        nudgeSelectedNotes (0.0, 1);
        return true;
    }

    return false;
}

void PianoRollComponent::nudgeSelectedNotes (double beatDelta, int rowDelta)
{
    using namespace PianoRoll;

    if (selectedNoteIds.empty())
        return;

    auto clampedBeatDelta = beatDelta;
    auto clampedRowDelta = rowDelta;

    for (const auto& note : notes)
    {
        if (! selectedNoteIds.contains (note.id))
            continue;

        clampedBeatDelta = juce::jlimit (-note.startBeat,
                                         (double) totalBeats - (note.startBeat + note.lengthBeats),
                                         clampedBeatDelta);

        const auto row = getRowForMidiNote (note.midiNote);
        clampedRowDelta = juce::jlimit (-row, (totalRows - 1) - row, clampedRowDelta);
    }

    for (auto& note : notes)
    {
        if (! selectedNoteIds.contains (note.id))
            continue;

        note.startBeat += clampedBeatDelta;
        note.midiNote = getMidiNoteForRow (getRowForMidiNote (note.midiNote) + clampedRowDelta);
        note.label = getNoteName (note.midiNote);
    }

    refreshGridNotes();
}

std::optional<juce::Rectangle<int>> PianoRollComponent::getMarqueeRect() const
{
    if (! marqueeState.has_value())
        return std::nullopt;

    return juce::Rectangle<int>::leftTopRightBottom (juce::jmin (marqueeState->origin.x, marqueeState->current.x) - horizontalScrollOffset,
                                                     juce::jmin (marqueeState->origin.y, marqueeState->current.y) - verticalScrollOffset,
                                                     juce::jmax (marqueeState->origin.x, marqueeState->current.x) - horizontalScrollOffset,
                                                     juce::jmax (marqueeState->origin.y, marqueeState->current.y) - verticalScrollOffset);
}

std::optional<juce::Rectangle<int>> PianoRollComponent::getMarqueeRectInContentSpace() const
{
    if (! marqueeState.has_value())
        return std::nullopt;

    return juce::Rectangle<int>::leftTopRightBottom (juce::jmin (marqueeState->origin.x, marqueeState->current.x),
                                                     juce::jmin (marqueeState->origin.y, marqueeState->current.y),
                                                     juce::jmax (marqueeState->origin.x, marqueeState->current.x),
                                                     juce::jmax (marqueeState->origin.y, marqueeState->current.y));
}

juce::Point<int> PianoRollComponent::getContentPosition (juce::Point<int> localPosition) const
{
    return { localPosition.x + horizontalScrollOffset, localPosition.y + verticalScrollOffset };
}

bool PianoRollComponent::isToggleSelectionModifierDown (const juce::ModifierKeys& mods) const
{
    return mods.isCtrlDown() || mods.isCommandDown();
}

void PianoRollComponent::handleMouseWheelScroll (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    const auto horizontalDelta = std::abs (wheel.deltaX) > 0.0f ? wheel.deltaX
                                                                : (wheel.isReversed ? -wheel.deltaY : wheel.deltaY);
    const auto verticalDelta = wheel.isReversed ? -wheel.deltaY : wheel.deltaY;

    if (std::abs (wheel.deltaX) > 0.0f)
    {
        horizontalScrollOffset -= (int) std::round (horizontalDelta * 120.0f);
    }
    else if (event.mods.isShiftDown())
    {
        horizontalScrollOffset -= (int) std::round (verticalDelta * 120.0f);
    }
    else
    {
        verticalScrollOffset -= (int) std::round (verticalDelta * 120.0f);
    }

    updateViewport();
}

void PianoRollComponent::notifyNotesChanged() const
{
    if (! suppressNotesChanged && notesChangedCallback != nullptr)
        notesChangedCallback (notes);
}

void PianoRollComponent::updateScrollbars()
{
    const auto contentWidth = PianoRoll::totalBeats * PianoRoll::beatWidth;
    const auto contentHeight = PianoRoll::totalRows * PianoRoll::noteRowHeight;

    horizontalScrollBar.setRangeLimits (0.0, (double) contentWidth);
    horizontalScrollBar.setCurrentRange (juce::Range<double> ((double) horizontalScrollOffset,
                                                              (double) horizontalScrollOffset + gridComponent.getWidth()));
    horizontalScrollBar.setSingleStepSize ((double) (snapEnabled ? getGridStepBeats() * PianoRoll::beatWidth
                                                                 : 12.0));

    verticalScrollBar.setRangeLimits (0.0, (double) contentHeight);
    verticalScrollBar.setCurrentRange (juce::Range<double> ((double) verticalScrollOffset,
                                                            (double) verticalScrollOffset + gridComponent.getHeight()));
    verticalScrollBar.setSingleStepSize ((double) PianoRoll::noteRowHeight);
}

void PianoRollComponent::updateViewport()
{
    const auto maxHorizontalOffset = juce::jmax (0, PianoRoll::totalBeats * PianoRoll::beatWidth - gridComponent.getWidth());
    const auto maxVerticalOffset = juce::jmax (0, PianoRoll::totalRows * PianoRoll::noteRowHeight - gridComponent.getHeight());

    horizontalScrollOffset = juce::jlimit (0, maxHorizontalOffset, horizontalScrollOffset);
    verticalScrollOffset = juce::jlimit (0, maxVerticalOffset, verticalScrollOffset);

    keyboardStrip.setVerticalScrollOffset (verticalScrollOffset);
    gridComponent.setScrollOffsets (horizontalScrollOffset, verticalScrollOffset);
    horizontalScrollBar.setCurrentRangeStart ((double) horizontalScrollOffset);
    verticalScrollBar.setCurrentRangeStart ((double) verticalScrollOffset);
    repaint();
}

void PianoRollComponent::refreshGridNotes()
{
    auto displayedNotes = notes;

    if (pendingDrawNote.has_value())
        displayedNotes.push_back (*pendingDrawNote);

    gridComponent.setNotes (std::move (displayedNotes));
    gridComponent.setSelectedNoteIds (selectedNoteIds);
    gridComponent.setMarqueeRect (getMarqueeRect());
    updateScrollbars();
    if (! pendingDrawNote.has_value())
        notifyNotesChanged();
}

int PianoRollComponent::getGridDivisionSubsteps() const
{
    switch (gridDivisionIndex)
    {
        case 0: return 1;
        case 1: return 2;
        case 2: return 4;
        case 3: return 8;
        default: return 4;
    }
}

double PianoRollComponent::getGridStepBeats() const
{
    return 1.0 / (double) getGridDivisionSubsteps();
}

void PianoRollComponent::showGridDivisionMenu()
{
    juce::PopupMenu menu;
    const auto options = getGridDivisionOptions();

    for (int i = 0; i < options.size(); ++i)
        menu.addItem (i + 1, options[i], true, i == gridDivisionIndex);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (toolbarLayout.gridButton),
                        [this] (int selectedId)
                        {
                            if (selectedId <= 0)
                                return;

                            gridDivisionIndex = selectedId - 1;
                            repaint (toolbarLayout.gridButton);
                            refreshGridNotes();
                        });
}

juce::String PianoRollComponent::getGridDivisionLabel() const
{
    const auto options = getGridDivisionOptions();
    return juce::isPositiveAndBelow (gridDivisionIndex, options.size()) ? options[(int) gridDivisionIndex]
                                                                         : "1/16";
}

juce::Array<juce::String> PianoRollComponent::getGridDivisionOptions()
{
    return { "1/4", "1/8", "1/16", "1/32" };
}

//==================================================
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

void PianoRollWindow::setTrackContext (juce::String trackName, juce::String patternName)
{
    if (content != nullptr)
        content->setTrackContext (std::move (trackName), std::move (patternName));
}

void PianoRollWindow::setNotes (std::vector<PianoRoll::Note> notes)
{
    if (content != nullptr)
        content->setNotes (std::move (notes));
}

void PianoRollWindow::setNotesChangedCallback (std::function<void (const std::vector<PianoRoll::Note>&)> callback)
{
    if (content != nullptr)
        content->setNotesChangedCallback (std::move (callback));
}

void PianoRollWindow::closeButtonPressed()
{
    setVisible (false);
}
