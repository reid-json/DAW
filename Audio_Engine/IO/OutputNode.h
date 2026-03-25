#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

class OutputNode
{
public:
    OutputNode() = default;
    ~OutputNode() = default;

    void prepare(double sampleRate, int blockSize);
    
    void processOutput(float* const* outputChannels, int numOutputChannels, const float* left, const float* right, int numSamples);
};
