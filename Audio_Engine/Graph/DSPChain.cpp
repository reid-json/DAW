#include "DSPChain.h"

void DSPChain::addProcessor(std::unique_ptr<DSPProcessor> p)
{
    processors.push_back(std::move(p));
}

void DSPChain::prepare(double sampleRate, int blockSize)
{
    for (auto& p : processors)
        p->prepare(sampleRate, blockSize);
}

void DSPChain::process(float* left, float* right, int numSamples)
{
    for (auto& p : processors)
        p->process(left, right, numSamples);
}