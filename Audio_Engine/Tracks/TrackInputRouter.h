#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "../MIDI/MidiSynth.h"
#include <memory>
#include <vector>

class TrackInputRouter
{
public:
    TrackInputRouter();
    ~TrackInputRouter() = default;

    void prepare(double sampleRate, int blockSize);
    
    void processAudio(float* left, float* right, int numSamples);
    void processBlock(juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi);
    
    void handleMidi(const juce::MidiBuffer& midiMessages);
    
    void setInputEnabled(bool enabled) { inputEnabled = enabled; }
    
    void setAudioInputEnabled(bool enabled) { audioEnabled = enabled; }
    void setMidiEnabled(bool enabled) { midiEnabled = enabled; }
    
    void setGain(float gainDb);
    void setMidiVolume(float vol) { if (synth) synth->setVolume(vol); }

private:
    std::unique_ptr<MidiSynth> synth;
    std::vector<float> synthLeft;
    std::vector<float> synthRight;
    float gain = 1.0f;
    bool inputEnabled = true;
    bool audioEnabled = true;
    bool midiEnabled = true;
    double sampleRate = 44100.0;
    int blockSize = 512;
    juce::uint32 startTime = 0;
};
