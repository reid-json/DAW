#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <cmath>

class GainPanFilter
{
public:
	void setGain(float gain);
	void setPan(float pan);
	void setSamplingRate(float samplingRate);
	void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&);

private:
	float gain = 1.0f;
	float pan = 0.0f;
	float samplingRate = 44100.0f;
	std::vector<float> dnBuffer;
};
