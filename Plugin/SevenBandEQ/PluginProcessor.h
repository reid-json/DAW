#pragma once
#include <JuceHeader.h>
#include "SevenBandEQ.h"

class SevenBandEQAudioProcessor : public juce::AudioProcessor
{
public:
    SevenBandEQAudioProcessor();
    ~SevenBandEQAudioProcessor() override;

    //============================================================================== Audio
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //============================================================================== Editor
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //============================================================================== Info
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //============================================================================== Programs
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //============================================================================== State
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //============================================================================== Parameters
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    SevenBandEQ eq; // DSP engine

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SevenBandEQAudioProcessor)
};