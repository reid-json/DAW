#include "TrackInputRouter.h"
#include <cmath>

TrackInputRouter::TrackInputRouter()
    : synth(std::make_unique<MidiSynth>())
{
    gain = 1.0f;
    startTime = juce::Time::getMillisecondCounter();
}

void TrackInputRouter::prepare(double sr, int bs)
{
    sampleRate = sr;
    blockSize = bs;
    synthLeft.resize(bs);
    synthRight.resize(bs);
    synth->prepare(sr, bs);
}

void TrackInputRouter::processAudio(float* left, float* right, int numSamples)
{
    // applying gain to audio input
    if (inputEnabled && audioEnabled)
    {
        juce::FloatVectorOperations::multiply(left, gain, numSamples);
        juce::FloatVectorOperations::multiply(right, gain, numSamples);
    }
    else
    {
        juce::FloatVectorOperations::clear(left, numSamples);
        juce::FloatVectorOperations::clear(right, numSamples);
    }
    
    // Mix in midi synth output
    if (midiEnabled)
    {
        if (static_cast<int>(synthLeft.size()) < numSamples)
        {
            synthLeft.resize(numSamples);
            synthRight.resize(numSamples);
        }
        
        synth->processAudio(synthLeft.data(), synthRight.data(), numSamples);
        juce::FloatVectorOperations::add(left, synthLeft.data(), numSamples);
        juce::FloatVectorOperations::add(right, synthRight.data(), numSamples);
    }
}

void TrackInputRouter::processBlock(juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi)
{
    const int numSamples = buffer.getNumSamples();
    
    if (juce::Time::getMillisecondCounter() - startTime < 1000)
    {
        buffer.clear();
        return;
    }

    if (!inputEnabled || !audioEnabled)
    {
        buffer.clear();
    }
    else if (std::abs(gain - 1.0f) > 0.001f)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.applyGain(ch, 0, numSamples, gain);
    }
    
    if (midiEnabled && synth)
    {
        synth->renderNextBlock(buffer, midi, 0, numSamples);
    }
}

void TrackInputRouter::handleMidi(const juce::MidiBuffer& midiMessages)
{
    if (midiEnabled)
        synth->processMidi(midiMessages, blockSize);
}

void TrackInputRouter::setGain(float gainDb)
{
    gain = std::pow(10.0f, gainDb / 20.0f);
}
