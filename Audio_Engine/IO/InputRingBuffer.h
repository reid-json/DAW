#pragma once
#include <vector>
#include <atomic>
#include <algorithm>
#include <cmath>

class InputRingBuffer
{
public:
    explicit InputRingBuffer(int size = 48000 * 10);

    void push(const float* data, int numSamples);

    float getRecentPeak(int windowSize) const;

private:
    std::vector<float> buffer;
    std::atomic<int> writeIndex;
};