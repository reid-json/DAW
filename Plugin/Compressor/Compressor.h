#pragma once
#include <vector>
#include <cmath>
#include "JuceHeader.h"

class Compressor {
public:
	void setThreshold(float threshold);
	void setRatio(float ratio);
	void setGain(float gain);
	void setSamplingRate(float samplingRate);
	void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&);
	//attack & release
	void setAttack(float attackMs);
	void setRelease(float releaseMs);
private:
	float threshold;
	float ratio;
	float gain;
	float samplingRate;
	std::vector<float> dnBuffer;
	//attack & release
	float attack = 10.0f;
	float release = 100.0f;
	float attackCoeff = 0.0f;
	float releaseCoeff = 0.0f;
	std::vector<float> envelope;
};