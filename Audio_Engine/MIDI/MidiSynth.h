#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>

class MidiSynth
{
public:
    MidiSynth();
    ~MidiSynth() = default;

    void prepare(double sampleRate, int blockSize);
    void processMidi(const juce::MidiBuffer& midiMessages, int numSamples);
    void processAudio(float* left, float* right, int numSamples);
    void renderNextBlock(juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi, int startSample, int numSamples);
    void setVolume(float vol) { volume = vol; }

private:
    juce::Synthesiser synth;
    double sampleRate = 44100.0;
    float volume = 0.5f;
    juce::AudioBuffer<float> internalBuffer;
    
    juce::dsp::Limiter<float> limiter;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> lowPassFilter;
    juce::dsp::ProcessSpec spec;
};
