#include "InputRingBuffer.h"

InputRingBuffer::InputRingBuffer(int size)
    : buffer(size, 0.0f), writeIndex(0)
{
}

void InputRingBuffer::push(const float* data, int numSamples)
{
    const int size = (int)buffer.size();
    int idx = writeIndex.load(std::memory_order_relaxed);

    for (int i = 0; i < numSamples; ++i)
    {
        buffer[idx] = data[i];
        idx = (idx + 1) % size;
    }

    writeIndex.store(idx, std::memory_order_release);
}

float InputRingBuffer::getRecentPeak(int windowSize) const
{
    const int size = (int)buffer.size();
    windowSize = std::min(windowSize, size);

    int idx = writeIndex.load(std::memory_order_acquire);
    float peak = 0.0f;

    for (int i = 0; i < windowSize; ++i)
    {
        idx = (idx - 1 + size) % size;
        peak = std::max(peak, std::abs(buffer[idx]));
    }

    return peak;
}