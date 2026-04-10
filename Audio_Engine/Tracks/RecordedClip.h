#pragma once

#include <juce_core/juce_core.h>
#include <vector>

struct RecordedClip
{
    int clipId = 0;
    juce::String name;
    juce::uint32 takeId = 0;
    double sampleRate = 44100.0;
    juce::int64 startSample = 0;
    std::vector<float> leftChannel;
    std::vector<float> rightChannel;

    int getNumSamples() const { return static_cast<int>(leftChannel.size()); }
    double getLengthSeconds() const { return sampleRate > 0.0 ? static_cast<double>(getNumSamples()) / sampleRate : 0.0; }
};
