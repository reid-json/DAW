#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace PianoRoll
{
    static constexpr int headerHeight    = 40;
    static constexpr int keyColumnWidth  = 96;
    static constexpr int noteRowHeight   = 24;
    static constexpr int beatWidth       = 48;
    static constexpr int totalRows       = 36;
    static constexpr int totalBeats      = 32;
    static constexpr int topMidiNote     = 84; // C6

    struct Note
    {
        int startBeat = 0;
        int lengthBeats = 1;
        int midiNote = 60;
        juce::String label = "Note";
    };

    juce::String getNoteName (int midiNote);
    bool isBlackKey (int midiNote);
    int getRowForMidiNote (int midiNote);
}

//==================================================
class PianoKeyboardStrip : public juce::Component
{
public:
    PianoKeyboardStrip();

    void paint (juce::Graphics& g) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoKeyboardStrip)
};

//==================================================
class PianoGridComponent : public juce::Component
{
public:
    PianoGridComponent();

    void paint (juce::Graphics& g) override;

    void setNotes (std::vector<PianoRoll::Note> newNotes);

private:
    std::vector<PianoRoll::Note> notes;

    void paintGrid (juce::Graphics& g);
    void paintNotes (juce::Graphics& g);
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

private:
    PianoKeyboardStrip keyboardStrip;
    PianoGridComponent gridComponent;

    void paintHeader (juce::Graphics& g, juce::Rectangle<int> area);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoRollComponent)
};

//==================================================
class PianoRollWindow : public juce::DocumentWindow
{
public:
    PianoRollWindow();
    void closeButtonPressed() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoRollWindow)
};