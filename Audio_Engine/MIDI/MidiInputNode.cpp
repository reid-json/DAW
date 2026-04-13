#include "MidiInputNode.h"

void MidiInputNode::prepare(double sampleRate, int blockSize)
{
    juce::ignoreUnused(sampleRate, blockSize);
    midiBuffer.clear();
}

void MidiInputNode::processMidiInput(const juce::MidiBuffer& midiMessages)
{
    midiBuffer.clear();
    midiBuffer.addEvents(midiMessages, 0, -1, 0);
}
