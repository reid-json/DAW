#pragma once
#include <vector>
#include <memory>
#include "../dsp/DSPProcessor.h"

class DSPChain
{
public:
    void addProcessor(std::unique_ptr<DSPProcessor> p);
    void prepare(double sampleRate, int blockSize);
    void process(float* left, float* right, int numSamples);

private:
    std::vector<std::unique_ptr<DSPProcessor>> processors;
};