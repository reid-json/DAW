#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

class InputNode
{
public:
    InputNode() = default;
    ~InputNode() = default;

    void prepare(double sampleRate, int blockSize);
    
    void processInput(const float* const* inputChannels, int numInputChannels, int numSamples);
    
    const float* getLeftBuffer() const { return leftBuffer.data(); }
    const float* getRightBuffer() const { return rightBuffer.data(); }

private:
    std::vector<float> leftBuffer;
    std::vector<float> rightBuffer;
};
