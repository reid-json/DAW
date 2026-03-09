#pragma once

struct DAWState
{
    enum class TransportState
    {
        stopped,
        playing,
        paused
    };

    TransportState transportState = TransportState::stopped;
    int trackCount = 2;
    int sidebarWidth = 240;
    double playhead = 0.0;
    double volumeDb = -6.0;

    void play()    { transportState = TransportState::playing; }
    void stop()    { transportState = TransportState::stopped; playhead = 0.0; }
    void pause()   { transportState = TransportState::paused; }
    void restart() { transportState = TransportState::stopped; playhead = 0.0; }

    void addTrack() { ++trackCount; }

    void tick (double dt)
    {
        if (transportState == TransportState::playing)
            playhead += dt;
    }
};