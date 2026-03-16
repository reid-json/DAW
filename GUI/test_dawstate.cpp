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

        beginTest ("Track count increments");

        expectEquals (state.trackCount, 2);

        state.addTrack();
        expectEquals (state.trackCount, 3);

        state.addTrack();
        expectEquals (state.trackCount, 4);

        beginTest ("Volume dB value can be updated");

        expectWithinAbsoluteError (state.volumeDb, -6.0, 0.0001);

        state.volumeDb = -12.5;
        expectWithinAbsoluteError (state.volumeDb, -12.5, 0.0001);

        state.volumeDb = 0.0;
        expectWithinAbsoluteError (state.volumeDb, 0.0, 0.0001);
    }
};

static DAWStateTest dawStateTest;