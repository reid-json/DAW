#include <juce_core/juce_core.h>
#include "dawstate.h"

class DAWStateTest : public juce::UnitTest
{
public:
    DAWStateTest() : juce::UnitTest ("DAWState Test", "DAW") {}

    void runTest() override
    {
        beginTest ("Transport changes state correctly");

        DAWState state;

        expect (state.transportState == DAWState::TransportState::stopped);

        state.play();
        expect (state.transportState == DAWState::TransportState::playing);

        state.pause();
        expect (state.transportState == DAWState::TransportState::paused);

        state.stop();
        expect (state.transportState == DAWState::TransportState::stopped);

        beginTest ("Default tracks exist");

        expectEquals (state.getNumTracks(), 2);

        expectEquals (state.getTrack (0).name, juce::String ("Track 1"));
        expectEquals (state.getTrack (1).name, juce::String ("Track 2"));

        beginTest ("Track count increments");

        state.addTrack();
        expectEquals (state.getNumTracks(), 3);
        expectEquals (state.getTrack (2).name, juce::String ("Track 3"));

        state.addTrack();
        expectEquals (state.getNumTracks(), 4);
        expectEquals (state.getTrack (3).name, juce::String ("Track 4"));

        beginTest ("Track default values are correct");

        const auto& track0 = state.getTrack (0);

        expectWithinAbsoluteError (track0.volumeDb, -6.0f, 0.0001f);
        expectWithinAbsoluteError (track0.pan, 0.0f, 0.0001f);
        expect (! track0.armed);
        expect (! track0.muted);

        beginTest ("Per-track volume value can be updated");

        state.getTrack (0).volumeDb = -12.5f;
        expectWithinAbsoluteError (state.getTrack (0).volumeDb, -12.5f, 0.0001f);

        state.getTrack (1).volumeDb = 0.0f;
        expectWithinAbsoluteError (state.getTrack (1).volumeDb, 0.0f, 0.0001f);

        beginTest ("Per-track pan value can be updated");

        state.getTrack (0).pan = -0.5f;
        expectWithinAbsoluteError (state.getTrack (0).pan, -0.5f, 0.0001f);

        state.getTrack (1).pan = 0.75f;
        expectWithinAbsoluteError (state.getTrack (1).pan, 0.75f, 0.0001f);

        beginTest ("Per-track buttons can be toggled");

        state.getTrack (0).armed = true;
        state.getTrack (1).muted = true;

        expect (state.getTrack (0).armed);
        expect (state.getTrack (1).muted);
        expect (! state.getTrack (0).muted);
        expect (! state.getTrack (1).armed);

        beginTest ("Track can be removed");

        state.removeTrack ();
        expectEquals (state.getNumTracks(), 3);

        expectEquals (state.getTrack (0).name, juce::String ("Track 1"));
        expectEquals (state.getTrack (1).name, juce::String ("Track 3"));
    }
};

static DAWStateTest dawStateTest;