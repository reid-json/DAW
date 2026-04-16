#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <optional>
#include <set>
#include <vector>

namespace PianoRoll
{
    static constexpr int toolbarH    = 51;
    static constexpr int timelineH   = 34;
    static constexpr int headerH     = toolbarH + timelineH;
    static constexpr int keyWidth    = 96;
    static constexpr int rowHeight   = 24;
    static constexpr int beatWidth   = 48;
    static constexpr int numRows     = 36;
    static constexpr int numBeats    = 32;
    static constexpr int topNote     = 84; // C6

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
    int noteToRow (int midiNote);
    int rowToNote (int row);
    juce::Rectangle<float> getNoteBounds (const Note& note);
}

//==================================================
class PianoRollComponent : public juce::Component
{
public:
    PianoRollComponent();

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    bool keyPressed (const juce::KeyPress& key) override;

    void setNotes (std::vector<PianoRoll::Note> newNotes);
    void setOnSavePattern (std::function<void (const std::vector<PianoRoll::Note>&)> cb);
    void setOnGetAvailableInstruments (std::function<juce::StringArray()> cb);
    void setOnInstrumentChanged (std::function<void (const juce::String&)> cb);
    void setInstrumentName (const juce::String& name);

private:
    enum class Tool { select, draw, erase };
    enum class Edge { left, right };

    struct ButtonRects
    {
        juce::Rectangle<int> select, draw, erase, save, instrument;
    };

    struct ResizeInfo
    {
        int noteId = 0;
        double startBeat = 0.0, endBeat = 0.0;
        Edge edge = Edge::right;
    };

    struct MoveSnapshot
    {
        int noteId = 0;
        double startBeat = 0.0;
        int midiNote = 60;
        double lengthBeats = 1.0;
    };

    struct MoveInfo
    {
        int anchorId = 0;
        double beatOffset = 0.0;
        int rowOffset = 0;
        double anchorBeat = 0.0;
        int anchorNote = 60;
        std::vector<MoveSnapshot> snapshots;
    };

    struct MarqueeInfo
    {
        juce::Point<int> start, current;
        std::set<int> baseSelection;
    };

    class ScrollListener : public juce::ScrollBar::Listener
    {
    public:
        explicit ScrollListener (PianoRollComponent& o) : owner (o) {}
        void scrollBarMoved (juce::ScrollBar* bar, double newStart) override;
    private:
        PianoRollComponent& owner;
    };

    // Painting
    void paintToolbar (juce::Graphics& g, juce::Rectangle<int> area);
    void paintTimeline (juce::Graphics& g, juce::Rectangle<int> area);
    void paintKeys (juce::Graphics& g);
    void paintGrid (juce::Graphics& g);
    void paintNotes (juce::Graphics& g);
    void paintMarquee (juce::Graphics& g);
    void paintButton (juce::Graphics& g, juce::Rectangle<int> area,
                      const juce::String& text, bool active = false, bool accent = false,
                      const juce::Image* icon = nullptr);

    // Layout / toolbar
    ButtonRects layoutButtons (juce::Rectangle<int> area) const;
    bool isOverButton (juce::Point<int> pos) const;
    bool handleButtonClick (juce::Point<int> pos);

    // Grid mouse handlers
    void gridMouseDown (const juce::MouseEvent& e);
    void gridMouseDrag (const juce::MouseEvent& e);
    void gridMouseUp (const juce::MouseEvent& e);

    // Note finding
    std::optional<size_t> findNoteAt (juce::Point<int> pos) const;
    std::optional<std::pair<size_t, Edge>> findNoteEdge (juce::Point<int> pos) const;
    std::optional<size_t> findNoteById (int id) const;

    // Selection
    void setSelection (std::set<int> ids);
    void selectOne (std::optional<size_t> index);
    void startMarquee (const juce::MouseEvent& e);
    void updateMarquee (juce::Point<int> pos);
    void endMarquee();

    // Note operations
    void startMove (size_t index, juce::Point<int> pos);
    void updateMove (juce::Point<int> pos);
    void endMove();
    void startResize (size_t index, Edge edge);
    void updateResize (juce::Point<int> pos);
    void endResize();
    void startDraw (juce::Point<int> pos);
    void updateDraw (juce::Point<int> pos);
    void commitDraw();
    void eraseAt (juce::Point<int> pos);
    void deleteSelected();
    void nudgeSelected (double beatDelta, int rowDelta);

    // Helpers
    juce::Point<int> toGridPos (juce::Point<int> local) const;
    std::optional<juce::Rectangle<int>> getMarqueeRect() const;
    void handleScroll (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel);
    void updateScrollbars();
    void updateViewport();
    void refreshDisplay();
    double getStepBeats() const;

    // Child components (scrollbars only)
    juce::ScrollBar hScroll { false };
    juce::ScrollBar vScroll { true };
    ScrollListener scrollListener { *this };
    juce::Image selectToolIcon;
    juce::Image drawToolIcon;
    juce::Image eraseToolIcon;

    // Layout rects (computed in resized)
    ButtonRects btnRects;
    juce::Rectangle<int> keyArea, gridArea;

    // State
    std::vector<PianoRoll::Note> notes;
    std::optional<PianoRoll::Note> drawingNote;
    std::set<int> selectedIds;
    std::optional<ResizeInfo> resizing;
    std::optional<MoveInfo> moving;
    std::optional<MarqueeInfo> marqueeState;

    Tool tool = Tool::select;
    int nextId = 1;
    int scrollX = 0, scrollY = 0;
    juce::String instrumentName;
    std::function<void (const std::vector<PianoRoll::Note>&)> onSavePatternRequested;
    std::function<juce::StringArray()> onGetAvailableInstruments;
    std::function<void (const juce::String&)> onInstrumentChangeRequested;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoRollComponent)
};

//==================================================
class PianoRollWindow : public juce::DocumentWindow
{
public:
    PianoRollWindow();
    void setOnSavePattern (std::function<void (const std::vector<PianoRoll::Note>&)> cb);
    void setOnGetAvailableInstruments (std::function<juce::StringArray()> cb);
    void setOnInstrumentChanged (std::function<void (const juce::String&)> cb);
    void setInstrumentName (const juce::String& name);
    void closeButtonPressed() override;

private:
    PianoRollComponent* content = nullptr;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoRollWindow)
};
