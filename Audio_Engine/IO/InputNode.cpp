#include "InputNode.h"
#include <cstring>

void InputNode::prepare(double sampleRate, int blockSize)
{
    juce::ignoreUnused(sampleRate);
    leftBuffer.resize(blockSize);
    rightBuffer.resize(blockSize);
}

void InputNode::processInput(const float* const* inputChannels, int numInputChannels, int numSamples)
{
    if (numInputChannels > 0)
        std::memcpy(leftBuffer.data(), inputChannels[0], numSamples * sizeof(float));
    else
        std::fill(leftBuffer.begin(), leftBuffer.end(), 0.0f);
    
    if (numInputChannels > 1)
        std::memcpy(rightBuffer.data(), inputChannels[1], numSamples * sizeof(float));
    else
        std::memcpy(rightBuffer.data(), leftBuffer.data(), numSamples * sizeof(float));
}
