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
    void renderNextBlock(juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi, int startSample, int numSamples);

private:
    juce::Synthesiser synth;
    juce::dsp::Limiter<float> limiter;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> lowPassFilter;
    juce::dsp::ProcessSpec spec;
};
