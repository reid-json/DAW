#include "GainProcessor.h"

void GainProcessor::process(float* left, float* right, int numSamples)
{
    const float g = gain.load();

    for (int i = 0; i < numSamples; i++)
    {
        left[i]  *= g;
        right[i] *= g;
    }
}