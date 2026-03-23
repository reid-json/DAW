#include "MidiSynth.h"

struct SineVoice : public juce::SynthesiserVoice
{
    SineVoice() 
    {
        adsrParams.attack = 0.020f;
        adsrParams.decay = 0.1f;
        adsrParams.sustain = 1.0f;
        adsrParams.release = 0.3f;
        adsr.setParameters(adsrParams);
    }

    bool canPlaySound(juce::SynthesiserSound* sound) override { return sound != nullptr; }

    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override
    {
        phase = 0.0f;
        auto freq = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        phaseIncrement = freq / getSampleRate();
        adsr.setSampleRate(getSampleRate());
        this->velocity = velocity;
        adsr.noteOn();
    }

    void stopNote(float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            adsr.noteOff();
        }
        else
        {
            adsr.reset();
            clearCurrentNote();
        }
    }

    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        if (!adsr.isActive()) return;

        for (int i = 0; i < numSamples; ++i)
        {
            auto env = adsr.getNextSample();
            auto val = std::sin(2.0 * juce::MathConstants<double>::pi * phase) * velocity * env;
            
            for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
                outputBuffer.addSample(ch, startSample + i, (float)val);

            phase += phaseIncrement;
            if (phase >= 1.0) phase -= 1.0;
        }

        if (!adsr.isActive())
            clearCurrentNote();
    }

    double phase = 0.0, phaseIncrement = 0.0;
    float velocity = 0.0f;
    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParams;
};

struct SineSound : public juce::SynthesiserSound
{
    SineSound() {}
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

MidiSynth::MidiSynth()
{
    for (int i = 0; i < 16; ++i)
        synth.addVoice(new SineVoice());
    synth.addSound(new SineSound());
}

void MidiSynth::prepare(double sr, int bs)
{
    sampleRate = sr;
    synth.setCurrentPlaybackSampleRate(sr);
    synth.allNotesOff(0, false);
    internalBuffer.setSize(2, bs);


    spec.sampleRate = sr;
    spec.maximumBlockSize = (juce::uint32)bs;
    spec.numChannels = 2;
    
    limiter.prepare(spec);
    limiter.setThreshold(-0.1f);
    limiter.setRelease(50.0f);

    *lowPassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sr, 2000.0);
    lowPassFilter.prepare(spec);
}

void MidiSynth::processMidi(const juce::MidiBuffer&, int) {}

void MidiSynth::processAudio(float* left, float* right, int numSamples)
{
    internalBuffer.clear();
    juce::MidiBuffer emptyMidi;
    synth.renderNextBlock(internalBuffer, emptyMidi, 0, numSamples);
    
    if (internalBuffer.getNumChannels() > 0)
        juce::FloatVectorOperations::multiply(internalBuffer.getWritePointer(0), volume, numSamples);
    if (internalBuffer.getNumChannels() > 1)
        juce::FloatVectorOperations::multiply(internalBuffer.getWritePointer(1), volume, numSamples);

    std::copy(internalBuffer.getReadPointer(0), internalBuffer.getReadPointer(0) + numSamples, left);
    if (internalBuffer.getNumChannels() > 1)
        std::copy(internalBuffer.getReadPointer(1), internalBuffer.getReadPointer(1) + numSamples, right);
    else
        std::copy(internalBuffer.getReadPointer(0), internalBuffer.getReadPointer(0) + numSamples, right);
}

void MidiSynth::renderNextBlock(juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi, int startSample, int numSamples)
{
    synth.renderNextBlock(buffer, midi, startSample, numSamples);

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    lowPassFilter.process(context);

    limiter.process(context);
}
