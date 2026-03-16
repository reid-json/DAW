#pragma once
#include <vector>
#include "JuceHeader.h"

class GainPanFilter
{
public:
	void setGain(float gain);
	void setPan(float pan);
	void setSamplingRate(float samplingRate);
	//function that processes audio, AudioBuffer stores audio samples
	void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&);

private:
	float gain;
	float pan;
	float samplingRate;
	//buffer for outpass filter
	std::vector<float> dnBuffer;
};