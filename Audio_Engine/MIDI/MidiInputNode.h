#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

class MidiInputNode
{
public:
    MidiInputNode() = default;
    ~MidiInputNode() = default;

    void prepare(double sampleRate, int blockSize);
    
    void processMidiInput(const juce::MidiBuffer& midiMessages);
    
    const juce::MidiBuffer& getMidiBuffer() const { return midiBuffer; }

private:
    juce::MidiBuffer midiBuffer;
};
