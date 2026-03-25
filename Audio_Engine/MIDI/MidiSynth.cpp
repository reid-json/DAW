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
    synth.setCurrentPlaybackSampleRate(sr);
    synth.allNotesOff(0, false);

    spec.sampleRate = sr;
    spec.maximumBlockSize = static_cast<juce::uint32>(bs);
    spec.numChannels = 2;
    
    limiter.prepare(spec);
    limiter.setThreshold(-0.1f);
    limiter.setRelease(50.0f);

    *lowPassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sr, 2000.0);
    lowPassFilter.prepare(spec);
}

void MidiSynth::renderNextBlock(juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi, int startSample, int numSamples)
{
    synth.renderNextBlock(buffer, midi, startSample, numSamples);

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    lowPassFilter.process(context);

    limiter.process(context);
}
