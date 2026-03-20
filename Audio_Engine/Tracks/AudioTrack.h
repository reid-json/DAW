#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

class AudioTrack
{
public:
    AudioTrack() = default;


    void recordSamples(const float* const* input, int numInputChannels, int numSamples);

   
    void playSamples(float* left, float* right, int numSamples);

    void resetReadPosition();

private:
    juce::AudioBuffer<float> recorded { 2, 0 }; 
    int writePos = 0;
    int readPos  = 0;
};