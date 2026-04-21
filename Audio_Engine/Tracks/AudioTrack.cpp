#include "AudioTrack.h"
#include <algorithm>
#include <cstring>

void AudioTrack::recordSamples(const float* const* input, int numInputChannels, int numSamples)
{
    
    if (recorded.getNumSamples() < writePos + numSamples)
        recorded.setSize(2, writePos + numSamples, true, true, true);

    const float* leftIn  = input[0];
    const float* rightIn = (numInputChannels > 1 ? input[1] : nullptr);

   
    recorded.copyFrom(0, writePos, leftIn, numSamples);

   
    if (rightIn != nullptr)
        recorded.copyFrom(1, writePos, rightIn, numSamples);
    else
        recorded.copyFrom(1, writePos, leftIn, numSamples); 

    writePos += numSamples;
}

void AudioTrack::playSamples(float* left, float* right, int numSamples)
{
    const int available = recorded.getNumSamples() - readPos;
    const int toCopy    = std::min(numSamples, available);

    if (toCopy > 0)
    {
        std::memcpy(left,
                    recorded.getReadPointer(0, readPos),
                    sizeof(float) * toCopy);

        std::memcpy(right,
                    recorded.getReadPointer(1, readPos),
                    sizeof(float) * toCopy);

        readPos += toCopy;
    }

    
    if (toCopy < numSamples)
    {
        std::fill(left  + toCopy, left  + numSamples, 0.0f);
        std::fill(right + toCopy, right + numSamples, 0.0f);
    }
}

void AudioTrack::resetReadPosition()
{
    readPos = 0;
}