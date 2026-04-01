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

        expectEquals (state.trackCount, 5);
        expectEquals (static_cast<int> (state.trackMixerStates.size()), 5);
        expectEquals (state.getTrackName (0), juce::String ("Track 1"));
        expect (state.isTrackSelected (0));

        state.addTrack();
        expectEquals (state.trackCount, 6);
        expectEquals (static_cast<int> (state.trackMixerStates.size()), 6);
        expectEquals (state.getTrackName (5), juce::String ("Track 6"));
        expect (state.isTrackSelected (5));

        beginTest ("Track naming and selection update");

        state.selectTrack (2);
        expect (state.isTrackSelected (2));
        expect (! state.isTrackSelected (0));

        state.setTrackName (2, "Lead Vox");
        expectEquals (state.getTrackName (2), juce::String ("Lead Vox"));

        state.setTrackName (2, "   ");
        expectEquals (state.getTrackName (2), juce::String ("Track 3"));

        beginTest ("Tempo clamps to supported range");

        expectWithinAbsoluteError (state.tempoBpm, 120.0, 0.001);
        state.setTempoBpm (180.0);
        expectWithinAbsoluteError (state.tempoBpm, 180.0, 0.001);
        state.setTempoBpm (20.0);
        expectWithinAbsoluteError (state.tempoBpm, 40.0, 0.001);
        state.setTempoBpm (300.0);
        expectWithinAbsoluteError (state.tempoBpm, 240.0, 0.001);

        beginTest ("Track mixer state toggles");

        expect (! state.getTrackMixerState (0).muted);
        state.toggleTrackMuted (0);
        expect (state.getTrackMixerState (0).muted);

        state.setTrackPan (0, 0.35f);
        expectWithinAbsoluteError (state.getTrackMixerState (0).pan, 0.35f, 0.001f);

        state.setTrackLevel (0, 0.55f);
        expectWithinAbsoluteError (state.getTrackMixerState (0).level, 0.55f, 0.001f);

        beginTest ("Solo and mute rules affect track audibility");

        state = DAWState {};
        expect (! state.anyTrackSoloed());
        expect (state.isTrackAudible (0));
        expect (state.isTrackAudible (1));

        state.toggleTrackSoloed (1);
        expect (state.anyTrackSoloed());
        expect (! state.isTrackAudible (0));
        expect (state.isTrackAudible (1));

        state.toggleTrackMuted (1);
        expect (! state.isTrackAudible (1));

        state.toggleTrackMuted (1);
        state.toggleTrackSoloed (1);
        expect (! state.anyTrackSoloed());
        expect (state.isTrackAudible (0));

        beginTest ("Track arm utilities update arm state");

        state = DAWState {};
        state.toggleTrackArmed (0);
        state.toggleTrackArmed (1);
        expect (state.getTrackMixerState (0).armed);
        expect (state.getTrackMixerState (1).armed);

        state.armOnlyTrack (2);
        expect (! state.getTrackMixerState (0).armed);
        expect (! state.getTrackMixerState (1).armed);
        expect (state.getTrackMixerState (2).armed);

        state.disarmAllTracks();
        expect (! state.getTrackMixerState (0).armed);
        expect (! state.getTrackMixerState (1).armed);
        expect (! state.getTrackMixerState (2).armed);

        beginTest ("FX slot skeleton state updates");

        expect (! state.getTrackFxSlot (0, 0).hasPlugin);
        expectEquals (state.getTrackFxSlotCount (0), 1);
        state.addTrackFxSlot (0);
        expectEquals (state.getTrackFxSlotCount (0), 2);
        state.loadTrackPluginIntoFxSlot (0, 0, "LowpassHighpassFilter");
        expect (state.getTrackFxSlot (0, 0).hasPlugin);
        expectEquals (state.getTrackFxSlot (0, 0).name, juce::String ("LowpassHighpassFilter"));
        state.removeTrackFxSlot (0, 1);
        expectEquals (state.getTrackFxSlotCount (0), 1);

        state.toggleTrackFxSlotBypassed (0, 0);
        expect (state.getTrackFxSlot (0, 0).bypassed);

        expectEquals (state.getMasterFxSlotCount (), 1);
        state.addMasterFxSlot ();
        expectEquals (state.getMasterFxSlotCount (), 2);
        state.loadMasterPluginIntoFxSlot (0, "LowpassHighpassFilter");
        expect (state.getMasterFxSlot (0).hasPlugin);
        expectEquals (state.getMasterFxSlot (0).name, juce::String ("LowpassHighpassFilter"));
        state.removeMasterFxSlot (1);
        expectEquals (state.getMasterFxSlotCount (), 1);
        state.clearMasterFxSlot (0);
        expect (! state.getMasterFxSlot (0).hasPlugin);

    }
};

static DAWStateTest dawStateTest;
