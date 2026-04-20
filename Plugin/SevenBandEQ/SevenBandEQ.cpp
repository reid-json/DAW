#include "SevenBandEQ.h"
#include <cmath>

SevenBandEQ::SevenBandEQ()
{
    bands.resize(7);
    filters.resize(7);
}

void SevenBandEQ::setSamplingRate(float sr)
{
    samplingRate = sr;
    for (int i = 0; i < 7; ++i)
        updateBandCoefficients(i);
}

void SevenBandEQ::setBandGain(int bandIndex, float gain)
{
    bands[bandIndex].gain = gain;
    updateBandCoefficients(bandIndex);
}

void SevenBandEQ::setBandFreq(int bandIndex, float freq)
{
    bands[bandIndex].freq = freq;
    updateBandCoefficients(bandIndex);
}

void SevenBandEQ::setBandQ(int bandIndex, float Q)
{
    bands[bandIndex].Q = Q;
    updateBandCoefficients(bandIndex);
}

void SevenBandEQ::setBandBypass(int bandIndex, bool bypass)
{
    bands[bandIndex].bypass = bypass;
}

// Peaking EQ formula from RBJ Audio EQ Cookbook
void SevenBandEQ::updateBandCoefficients(int bandIndex)
{
    const auto& band = bands[bandIndex];

    float A = std::pow(10.0f, band.gain / 40.0f);
    float omega = 2.0f * juce::MathConstants<float>::pi * band.freq / samplingRate;
    float alpha = std::sin(omega) / (2.0f * band.Q);
    float cosw = std::cos(omega);

    float b0 = 1.0f + alpha * A;
    float b1 = -2.0f * cosw;
    float b2 = 1.0f - alpha * A;

    float a0 = 1.0f + alpha / A;
    float a1 = -2.0f * cosw;
    float a2 = 1.0f - alpha / A;

    b0 /= a0;
    b1 /= a0;
    b2 /= a0;
    a1 /= a0;
    a2 /= a0;

    for (auto& filter : filters[bandIndex])
    {
        filter.a0 = b0;
        filter.a1 = b1;
        filter.a2 = b2;
        filter.b1 = a1;
        filter.b2 = a2;
    }
}

void SevenBandEQ::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    if (filters[0].size() != numChannels)
    {
        for (int band = 0; band < 7; ++band)
        {
            filters[band].resize(numChannels);
            for (auto& filter : filters[band])
                filter.reset();
            updateBandCoefficients(band);
        }
    }

    for (int band = 0; band < 7; ++band)
    {
        if (bands[band].bypass)
            continue;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* samples = buffer.getWritePointer(ch);
            auto& filter = filters[band][ch];

            for (int i = 0; i < numSamples; ++i)
                samples[i] = filter.processSample(samples[i]);
        }
    }
}
