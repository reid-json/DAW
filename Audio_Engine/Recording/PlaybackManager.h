#pragma once

#include <atomic>
#include <juce_core/juce_core.h>

class PlaybackManager
{
public:
    void prepare(double newSampleRate) { sampleRate.store(newSampleRate); }

    void start()
    {
        currentSamplePosition.store(0);
        paused.store(false);
        playing.store(true);
    }

    void pause()
    {
        if (!playing.load())
            return;

        playing.store(false);
        paused.store(true);
    }

    void resume()
    {
        if (!paused.load())
            return;

        paused.store(false);
        playing.store(true);
    }

    void stop()
    {
        playing.store(false);
        paused.store(false);
        currentSamplePosition.store(0);
    }

    void advance(int numSamples)
    {
        if (playing.load() && numSamples > 0)
            currentSamplePosition.fetch_add(numSamples);
    }

    bool isPlaying() const { return playing.load(); }
    bool isPaused() const { return paused.load(); }
    juce::int64 getCurrentSamplePosition() const { return currentSamplePosition.load(); }
    double getSampleRate() const { return sampleRate.load(); }
    double getCurrentPositionSeconds() const
    {
        const auto currentRate = sampleRate.load();
        return currentRate > 0.0 ? static_cast<double>(currentSamplePosition.load()) / currentRate : 0.0;
    }

private:
    std::atomic<double> sampleRate { 44100.0 };
    std::atomic<juce::int64> currentSamplePosition { 0 };
    std::atomic<bool> playing { false };
    std::atomic<bool> paused { false };
};
