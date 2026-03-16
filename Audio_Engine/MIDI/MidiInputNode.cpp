#include "MidiInputNode.h"

void MidiInputNode::prepare(double sampleRate, int blockSize)
{
    midiBuffer.clear();
}

void MidiInputNode::processMidiInput(const juce::MidiBuffer& midiMessages)
{
    midiBuffer.clear();
    midiBuffer.addEvents(midiMessages, 0, -1, 0);
}
