#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <cmath>

class Compressor {
public:
	void setThreshold(float threshold);
	void setRatio(float ratio);
	void setGain(float gain);
	void setSamplingRate(float samplingRate);
	void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&);
	void setAttack(float attackMs);
	void setRelease(float releaseMs);
private:
	float threshold = 0.0f;
	float ratio = 1.0f;
	float gain = 0.0f;
	float samplingRate = 44100.0f;
	std::vector<float> dnBuffer;
	float attack = 10.0f;
	float release = 100.0f;
	float attackCoeff = 0.0f;
	float releaseCoeff = 0.0f;
	std::vector<float> envelope;
};
