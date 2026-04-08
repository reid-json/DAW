#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <optional>
#include <set>
#include <vector>

namespace PianoRoll
{
    static constexpr int toolbarHeight   = 46;
    static constexpr int timelineHeight  = 34;
    static constexpr int headerHeight    = toolbarHeight + timelineHeight;
    static constexpr int keyColumnWidth  = 96;
    static constexpr int noteRowHeight   = 24;
    static constexpr int beatWidth       = 48;
    static constexpr int totalRows       = 36;
    static constexpr int totalBeats      = 32;
    static constexpr int topMidiNote     = 84; // C6

    struct Note
    {
        int id = 0;
        double startBeat = 0.0;
        double lengthBeats = 1.0;
        int midiNote = 60;
        juce::String label = "Note";
    };

    juce::String getNoteName (int midiNote);
    bool isBlackKey (int midiNote);
    int getRowForMidiNote (int midiNote);
    int getMidiNoteForRow (int row);
}

//==================================================
class PianoKeyboardStrip : public juce::Component
{
public:
    PianoKeyboardStrip();

    void paint (juce::Graphics& g) override;
    void setVerticalScrollOffset (int newScrollOffset);

private:
    int verticalScrollOffset = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoKeyboardStrip)
};

//==================================================
class PianoGridComponent : public juce::Component
{
public:
    PianoGridComponent();

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;
    void mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    void setNotes (std::vector<PianoRoll::Note> newNotes);
    void setSelectedNoteIds (std::set<int> newSelectedNoteIds);
    void setMarqueeRect (std::optional<juce::Rectangle<int>> newMarqueeRect);
    void setScrollOffsets (int newHorizontalOffset, int newVerticalOffset);
    void setMouseDownCallback (std::function<void (const juce::MouseEvent&)> callback);
    void setMouseDragCallback (std::function<void (const juce::MouseEvent&)> callback);
    void setMouseUpCallback (std::function<void (const juce::MouseEvent&)> callback);
    void setMouseWheelCallback (std::function<void (const juce::MouseEvent&, const juce::MouseWheelDetails&)> callback);

private:
    std::vector<PianoRoll::Note> notes;
    std::set<int> selectedNoteIds;
    std::optional<juce::Rectangle<int>> marqueeRect;
    std::function<void (const juce::MouseEvent&)> mouseDownCallback;
    std::function<void (const juce::MouseEvent&)> mouseDragCallback;
    std::function<void (const juce::MouseEvent&)> mouseUpCallback;
    std::function<void (const juce::MouseEvent&, const juce::MouseWheelDetails&)> mouseWheelCallback;
    int horizontalScrollOffset = 0;
    int verticalScrollOffset = 0;

    void paintGrid (juce::Graphics& g);
    void paintNotes (juce::Graphics& g);
    void paintMarquee (juce::Graphics& g);
    void paintPlayhead (juce::Graphics& g);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoGridComponent)
};

//==================================================
class PianoRollComponent : public juce::Component
{
public:
    PianoRollComponent();

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent& event) override;
    void mouseExit (const juce::MouseEvent& event) override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    void setTrackContext (juce::String newTrackName, juce::String newPatternName);
    void setNotes (std::vector<PianoRoll::Note> newNotes);
    void setNotesChangedCallback (std::function<void (const std::vector<PianoRoll::Note>&)> callback);

private:
    enum class EditorTool
    {
        select,
        draw,
        erase
    };

    struct ToolbarLayout
    {
        juce::Rectangle<int> selectButton;
        juce::Rectangle<int> drawButton;
        juce::Rectangle<int> eraseButton;
        juce::Rectangle<int> snapButton;
        juce::Rectangle<int> gridButton;
    };

    enum class ResizeEdge
    {
        left,
        right
    };

    struct ResizeState
    {
        int noteId = 0;
        double startBeat = 0.0;
        double endBeat = 0.0;
        ResizeEdge edge = ResizeEdge::right;
    };

    struct MoveNoteSnapshot
    {
        int noteId = 0;
        double startBeat = 0.0;
        int midiNote = 60;
        juce::String label;
        double lengthBeats = 1.0;
    };

    struct MoveState
    {
        int anchorNoteId = 0;
        double pointerOffsetBeats = 0.0;
        int pointerRowOffset = 0;
        double anchorStartBeat = 0.0;
        int anchorMidiNote = 60;
        std::vector<MoveNoteSnapshot> snapshots;
    };

    struct MarqueeState
    {
        juce::Point<int> origin;
        juce::Point<int> current;
        std::set<int> baseSelectionIds;
    };

    class ScrollbarListener : public juce::ScrollBar::Listener
    {
    public:
        explicit ScrollbarListener (PianoRollComponent& ownerToNotify) : owner (ownerToNotify) {}
        void scrollBarMoved (juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;

    private:
        PianoRollComponent& owner;
    };

    PianoKeyboardStrip keyboardStrip;
    PianoGridComponent gridComponent;
    juce::ScrollBar horizontalScrollBar { false };
    juce::ScrollBar verticalScrollBar { true };
    ScrollbarListener scrollbarListener { *this };
    ToolbarLayout toolbarLayout;
    std::vector<PianoRoll::Note> notes;
    std::optional<PianoRoll::Note> pendingDrawNote;
    std::set<int> selectedNoteIds;
    std::optional<ResizeState> resizeState;
    std::optional<MoveState> moveState;
    std::optional<MarqueeState> marqueeState;
    EditorTool activeTool = EditorTool::select;
    bool snapEnabled = true;
    int gridDivisionIndex = 2;
    int nextNoteId = 1;
    int horizontalScrollOffset = 0;
    int verticalScrollOffset = 0;
    juce::String trackName = "Track 1";
    juce::String patternName = "Pattern 1";
    std::function<void (const std::vector<PianoRoll::Note>&)> notesChangedCallback;
    bool suppressNotesChanged = false;

    void paintToolbar (juce::Graphics& g, juce::Rectangle<int> area);
    void paintTimeline (juce::Graphics& g, juce::Rectangle<int> area);
    void paintToolbarButton (juce::Graphics& g,
                             juce::Rectangle<int> area,
                             const juce::String& text,
                             bool isActive = false,
                             bool isAccent = false);
    void paintIconButton (juce::Graphics& g,
                          juce::Rectangle<int> area,
                          const juce::String& text);
    ToolbarLayout layoutToolbar (juce::Rectangle<int> area) const;
    bool isToolbarInteractiveArea (juce::Point<int> position) const;
    bool handleToolbarClick (juce::Point<int> position);
    void handleGridMouseDown (const juce::MouseEvent& event);
    void handleGridMouseDrag (const juce::MouseEvent& event);
    void handleGridMouseUp (const juce::MouseEvent& event);
    std::optional<size_t> findNoteAt (juce::Point<int> position) const;
    std::optional<std::pair<size_t, ResizeEdge>> findResizableNoteAt (juce::Point<int> position) const;
    std::optional<size_t> findNoteIndexById (int noteId) const;
    void setSelectedNoteIds (std::set<int> noteIds);
    void selectSingleNote (std::optional<size_t> noteIndex);
    void beginMarqueeSelection (const juce::MouseEvent& event);
    void updateMarqueeSelection (juce::Point<int> contentPosition);
    void endMarqueeSelection();
    void beginNoteMove (size_t noteIndex, juce::Point<int> position);
    void updateNoteMove (juce::Point<int> position);
    void endNoteMove();
    void beginNoteResize (size_t noteIndex, ResizeEdge edge);
    void updateNoteResize (juce::Point<int> position);
    void endNoteResize();
    void beginNoteDrawAt (juce::Point<int> position);
    void updatePendingDrawNote (juce::Point<int> position);
    void commitPendingDrawNote();
    void eraseNoteAt (juce::Point<int> position);
    void deleteSelectedNotes();
    bool keyPressed (const juce::KeyPress& key) override;
    void nudgeSelectedNotes (double beatDelta, int rowDelta);
    std::optional<juce::Rectangle<int>> getMarqueeRect() const;
    std::optional<juce::Rectangle<int>> getMarqueeRectInContentSpace() const;
    juce::Point<int> getContentPosition (juce::Point<int> localPosition) const;
    bool isToggleSelectionModifierDown (const juce::ModifierKeys& mods) const;
    void handleMouseWheelScroll (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel);
    void notifyNotesChanged() const;
    void updateScrollbars();
    void updateViewport();
    void refreshGridNotes();
    int getGridDivisionSubsteps() const;
    double getGridStepBeats() const;
    void showGridDivisionMenu();
    juce::String getGridDivisionLabel() const;
    static juce::Array<juce::String> getGridDivisionOptions();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoRollComponent)
};

//==================================================
class PianoRollWindow : public juce::DocumentWindow
{
public:
    PianoRollWindow();
    void setTrackContext (juce::String trackName, juce::String patternName);
    void setNotes (std::vector<PianoRoll::Note> notes);
    void setNotesChangedCallback (std::function<void (const std::vector<PianoRoll::Note>&)> callback);
    void closeButtonPressed() override;

private:
    PianoRollComponent* content = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoRollWindow)
};
