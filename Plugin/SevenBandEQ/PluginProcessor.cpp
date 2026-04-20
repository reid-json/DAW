#include "PluginProcessor.h"
#include "PluginEditor.h"

SevenBandEQAudioProcessor::SevenBandEQAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    apvts(*this, nullptr, "Parameters", createParameters())
{
}

SevenBandEQAudioProcessor::~SevenBandEQAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout SevenBandEQAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    for (int i = 0; i < 7; ++i)
    {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            "band" + std::to_string(i) + "Gain",
            "Band " + std::to_string(i) + " Gain",
            -24.0f, 24.0f, 0.0f
        ));
    }

    return { params.begin(), params.end() };
}

const juce::String SevenBandEQAudioProcessor::getName() const { return "SevenBandEQ"; }
bool SevenBandEQAudioProcessor::acceptsMidi() const { return false; }
bool SevenBandEQAudioProcessor::producesMidi() const { return false; }
bool SevenBandEQAudioProcessor::isMidiEffect() const { return false; }
double SevenBandEQAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int SevenBandEQAudioProcessor::getNumPrograms() { return 1; }
int SevenBandEQAudioProcessor::getCurrentProgram() { return 0; }
void SevenBandEQAudioProcessor::setCurrentProgram(int) {}
const juce::String SevenBandEQAudioProcessor::getProgramName(int) { return {}; }
void SevenBandEQAudioProcessor::changeProgramName(int, const juce::String&) {}

void SevenBandEQAudioProcessor::prepareToPlay(double sampleRate, int)
{
    eq.setSamplingRate((float)sampleRate);
}

void SevenBandEQAudioProcessor::releaseResources() {}

void SevenBandEQAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    for (int i = 0; i < 7; ++i)
    {
        float gain = apvts.getRawParameterValue("band" + std::to_string(i) + "Gain")->load();
        eq.setBandGain(i, gain);
    }

    juce::MidiBuffer emptyMidi;
    eq.processBlock(buffer, emptyMidi);
}

juce::AudioProcessorEditor* SevenBandEQAudioProcessor::createEditor()
{
    return new SevenBandEQAudioProcessorEditor(*this);
}

void SevenBandEQAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream(destData, true);
    apvts.state.writeToStream(stream);
}

void SevenBandEQAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    juce::ValueTree tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid())
        apvts.replaceState(tree);
}
