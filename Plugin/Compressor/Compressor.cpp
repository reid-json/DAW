#include "Compressor.h"
#include <cmath>

void Compressor::setThreshold(float threshold)
{
	this->threshold = threshold;
}

void Compressor::setRatio(float ratio) 
{
	this->ratio = ratio;
}

void Compressor::setGain(float gain)
{
	this->gain = gain;
}

void Compressor::setSamplingRate(float samplingRate)
{
	this->samplingRate = samplingRate;

    setAttack(attack);
    setRelease(release);
}

/*Add for attack/Release*/
void Compressor::setAttack(float attackMs)
{
    this->attack = attackMs;
    attackCoeff = std::exp(-1.0f / (0.001f * attack * samplingRate));
}

void Compressor::setRelease(float releaseMs)
{
    this->release = releaseMs;
    releaseCoeff = std::exp(-1.0f / (0.001f * release * samplingRate));
}
/**/

void Compressor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    if (envelope.size() != buffer.getNumChannels()) {
        envelope.resize(buffer.getNumChannels(), 1.0f);
    }

    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    // Convert gain dB -> linear for makeup gain
    float linearGain = std::pow(10.0f, gain / 20.0f);
    for (int channel = 0; channel < numChannels; ++channel)
    {
        float* channelData = buffer.getWritePointer(channel);

        for (int i = 0; i < numSamples; ++i)
        {
            float inputSample = channelData[i];

            // Calculate level in dB
            float level = std::abs(inputSample);
            if (level < 1e-6f)
                level = 1e-6f;
            float levelDB = 20.0f * std::log10(level);

            float gainReductionDB = 0.0f;

            // Apply compression if above threshold
            if (levelDB > threshold)
            {
                float excessDB = levelDB - threshold;
                float compressedDB = excessDB / ratio;
                gainReductionDB = excessDB - compressedDB;
            }

            // Convert gain reduction to linear
            float targetGain = std::pow(10.0f, -gainReductionDB / 20.0f);

            // Apply attack/release smoothing
            float& env = envelope[channel];

            if (targetGain < env)
                env = attackCoeff * (env - targetGain) + targetGain;
            else
                env = releaseCoeff * (env - targetGain) + targetGain;

            // Apply smoothed gain + makeup gain
            channelData[i] = inputSample * env * linearGain;
        }
    }
}