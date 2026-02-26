#pragma once

class DSPProcessor
{
public:
    virtual ~DSPProcessor() = default;

    virtual void prepare(double sampleRate, int blockSize) = 0;
    virtual void process(float* left, float* right, int numSamples) = 0;
    virtual void reset() = 0;
};