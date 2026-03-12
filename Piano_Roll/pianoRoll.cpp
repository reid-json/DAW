#include "pianoRoll.h"

namespace PianoRoll
{
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
}

//==================================================
PianoKeyboardStrip::PianoKeyboardStrip()
{
    setOpaque (true);
}

void PianoKeyboardStrip::paint (juce::Graphics& g)
{
    using namespace PianoRoll;

    g.fillAll (juce::Colour::fromRGB (232, 235, 240));

    // White keys
    for (int row = 0; row < totalRows; ++row)
    {
        const int midiNote = topMidiNote - row;
        const int y = row * noteRowHeight;

        if (! isBlackKey (midiNote))
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
        const int y = row * noteRowHeight;

        if (isBlackKey (midiNote))
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

    notes =
    {
        { 1, 4, 77, "F5"  },
        { 3, 3, 75, "D#5" },
        { 6, 5, 70, "A#4" },
        { 10, 2, 65, "F4" }
    };
}

void PianoGridComponent::setNotes (std::vector<PianoRoll::Note> newNotes)
{
    notes = std::move (newNotes);
    repaint();
}

void PianoGridComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (15, 17, 22));
    paintGrid (g);
    paintNotes (g);
    paintPlayhead (g);
}

void PianoGridComponent::paintGrid (juce::Graphics& g)
{
    using namespace PianoRoll;

    for (int row = 0; row < totalRows; ++row)
    {
        const int midiNote = topMidiNote - row;
        const int y = row * noteRowHeight;

        g.setColour (isBlackKey (midiNote)
                         ? juce::Colour::fromRGB (20, 24, 33)
                         : juce::Colour::fromRGB (24, 29, 40));
        g.fillRect (0, y, getWidth(), noteRowHeight);

        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.drawHorizontalLine (y, 0.0f, (float) getWidth());
    }

    for (int beat = 0; beat <= totalBeats; ++beat)
    {
        const int x = beat * beatWidth;
        const bool major = (beat % 4 == 0);

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

        const int x = note.startBeat * beatWidth;
        const int y = row * noteRowHeight;
        const int w = juce::jmax (beatWidth, note.lengthBeats * beatWidth);

        auto r = juce::Rectangle<float> ((float) x + 2.0f,
                                         (float) y + 2.0f,
                                         (float) w - 4.0f,
                                         (float) noteRowHeight - 4.0f);

        g.setColour (juce::Colour::fromRGB (58, 122, 254));
        g.fillRoundedRectangle (r, 6.0f);

        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.drawRoundedRectangle (r, 6.0f, 1.0f);

        g.setColour (juce::Colours::white.withAlpha (0.95f));
        g.setFont (12.0f);
        g.drawText (note.label, r.toNearestInt().reduced (8, 0),
                    juce::Justification::centredLeft, false);
    }
}

void PianoGridComponent::paintPlayhead (juce::Graphics& g)
{
    using namespace PianoRoll;

    const int playheadBeat = 2;
    const int x = playheadBeat * beatWidth;

    g.setColour (juce::Colour::fromRGB (58, 122, 254));
    g.drawLine ((float) x, 0.0f, (float) x, (float) getHeight(), 2.0f);
}

//==================================================
PianoRollComponent::PianoRollComponent()
{
    addAndMakeVisible (keyboardStrip);
    addAndMakeVisible (gridComponent);
    setOpaque (true);
}

void PianoRollComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (15, 17, 22));

    auto headerArea = getLocalBounds().removeFromTop (PianoRoll::headerHeight);
    paintHeader (g, headerArea);
}

void PianoRollComponent::paintHeader (juce::Graphics& g, juce::Rectangle<int> area)
{
    using namespace PianoRoll;

    g.setColour (juce::Colour::fromRGB (22, 27, 38));
    g.fillRect (area);

    g.setColour (juce::Colours::white.withAlpha (0.92f));
    g.setFont (16.0f);
    g.drawText ("Piano Roll", 12, 8, 140, 24, juce::Justification::centredLeft);

    for (int beat = 0; beat < totalBeats; ++beat)
    {
        const int x = keyColumnWidth + beat * beatWidth;

        g.setColour (juce::Colours::white.withAlpha (0.65f));
        g.setFont (12.0f);
        g.drawText (juce::String (beat + 1), x + 4, 10, 28, 16,
                    juce::Justification::centredLeft);
    }

    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawLine ((float) keyColumnWidth, (float) area.getBottom() - 1.0f,
                (float) getWidth(),      (float) area.getBottom() - 1.0f, 1.0f);
}

void PianoRollComponent::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop (PianoRoll::headerHeight);

    keyboardStrip.setBounds (bounds.removeFromLeft (PianoRoll::keyColumnWidth));
    gridComponent.setBounds (bounds);
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
    setContentOwned (new PianoRollComponent(), false);
    centreWithSize (1100, 700);
}

void PianoRollWindow::closeButtonPressed()
{
    setVisible (false);
}