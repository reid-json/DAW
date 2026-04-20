#include "SevenBandEQ.h"
#include <cmath>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
// Constructor
SevenBandEQ::SevenBandEQ()
{
    bands.resize(7);
    filters.resize(7); // each band has a vector of filters, one per channel
}

//==============================================================================
// Set sample rate
void SevenBandEQ::setSamplingRate(float sr)
{
    samplingRate = sr;

    // Update all band coefficients
    for (int i = 0; i < 7; ++i)
        updateBandCoefficients(i);
}

//==============================================================================
// Set band parameters
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

//==============================================================================
// Update biquad coefficients for one band
void SevenBandEQ::updateBandCoefficients(int bandIndex)
{
    const auto& band = bands[bandIndex];

    float A = std::pow(10.0f, band.gain / 40.0f);             // linear gain
    float omega = 2.0f * juce::MathConstants<float>::pi * band.freq / samplingRate;
    float alpha = std::sin(omega) / (2.0f * band.Q);
    float cosw = std::cos(omega);

    // Peaking EQ formula from RBJ Audio EQ Cookbook
    float b0 = 1.0f + alpha * A;
    float b1 = -2.0f * cosw;
    float b2 = 1.0f - alpha * A;

    float a0 = 1.0f + alpha / A;
    float a1 = -2.0f * cosw;
    float a2 = 1.0f - alpha / A;

    // Normalize coefficients
    b0 /= a0;
    b1 /= a0;
    b2 /= a0;
    a1 /= a0;
    a2 /= a0;

    // Apply to each channel
    for (auto& filter : filters[bandIndex])
    {
        filter.a0 = b0;
        filter.a1 = b1;
        filter.a2 = b2;
        filter.b1 = a1;
        filter.b2 = a2;
    }
}
//==============================================================================
// Process audio block
void SevenBandEQ::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    // If channels changed, resize filter arrays
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

    // Apply each band
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
