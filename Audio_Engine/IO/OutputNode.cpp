#include "OutputNode.h"
#include <cstring>

void OutputNode::processOutput(float* const* outputChannels, int numOutputChannels, const float* left, const float* right, int numSamples)
{
    if (numOutputChannels > 0 && left != nullptr)
        std::memcpy(outputChannels[0], left, numSamples * sizeof(float));
    
    if (numOutputChannels > 1 && right != nullptr)
        std::memcpy(outputChannels[1], right, numSamples * sizeof(float));
    else if (numOutputChannels > 1 && left != nullptr)
        std::memcpy(outputChannels[1], left, numSamples * sizeof(float));
}
