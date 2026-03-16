#include "GainPanFilter.h"

void GainPanFilter::setGain(float gain)
{
	this->gain = gain;
}

void GainPanFilter::setPan(float pan)
{
	this->pan = pan;
}

void GainPanFilter::setSamplingRate(float samplingRate)
{
	this->samplingRate = samplingRate;
}

void GainPanFilter::processBlock(juce::AudioBuffer<float>& /**/buffer/**/, juce::MidiBuffer&)
{
    //audio processing code
    constexpr float PI = 3.14159265359f;

    int numSamples = buffer.getNumSamples();

    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getWritePointer(1);

    float angle = (pan + 1.0f) * (PI * 0.25f);

    float leftGain = std::cos(angle) * gain;
    float rightGain = std::sin(angle) * gain;

    for (int i = 0; i < numSamples; ++i)
    {
        leftChannel[i] *= leftGain;
        rightChannel[i] *= rightGain;
    }
}
