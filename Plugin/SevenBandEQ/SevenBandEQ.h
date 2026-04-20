#pragma once
#include <vector>
#include <array>
#include "JuceHeader.h"

class SevenBandEQ
{
public:
    SevenBandEQ();

    void setSamplingRate(float sampleRate);
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&);

    void setBandGain(int bandIndex, float gain);
    void setBandFreq(int bandIndex, float freq);
    void setBandQ(int bandIndex, float Q);
    void setBandBypass(int bandIndex, bool bypass);

private:
    struct Band
    {
        float gain = 0.0f;    // dB
        float freq = 1000.0f; // Hz
        float Q = 1.0f;
        bool bypass = false;
    };

    struct Biquad
    {
        float a0 = 1.0f, a1 = 0.0f, a2 = 0.0f;
        float b1 = 0.0f, b2 = 0.0f;

        float x1 = 0.0f, x2 = 0.0f; // previous input samples
        float y1 = 0.0f, y2 = 0.0f; // previous output samples

        void reset()
        {
            x1 = x2 = 0.0f;
            y1 = y2 = 0.0f;
        }

        float processSample(float x)
        {
            float y = a0 * x + a1 * x1 + a2 * x2 - b1 * y1 - b2 * y2;

            x2 = x1;
            x1 = x;
            y2 = y1;
            y1 = y;

            return y;
        }
    };

    void updateBandCoefficients(int bandIndex);

    std::vector<Band> bands;                     // 7 bands
    std::vector<std::vector<Biquad>> filters;    // filters[channel][band]
    float samplingRate = 44100.0f;
    int numChannels = 0;
};