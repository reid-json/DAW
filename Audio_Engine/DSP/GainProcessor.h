#pragma once
#include "DSPProcessor.h"
#include <atomic>

class GainProcessor : public DSPProcessor
{
public:
    GainProcessor() = default;

    void setGain(float g) { gain.store(g); }

    void prepare(double sampleRate, int blockSize) override {}
    void reset() override {}

    void process(float* left, float* right, int numSamples) override;

private:
    std::atomic<float> gain { 1.0f };
};