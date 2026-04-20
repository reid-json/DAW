#include "PluginProcessor.h"
#include "PluginEditor.h"

CompressorAudioProcessor::CompressorAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    parameters(*this, nullptr, juce::Identifier("CompressorPlugin"),
        {std::make_unique<juce::AudioParameterFloat>(
            "threshold", "Threshold",
            juce::NormalisableRange{-60.0f, 0.0f, 0.1f}, -24.0f),
        std::make_unique<juce::AudioParameterFloat>(
            "ratio", "Ratio",
            juce::NormalisableRange{1.0f, 20.f, 0.1f }, 2.0f),
        std::make_unique<juce::AudioParameterFloat>(
            "gain", "Gain",
            juce::NormalisableRange{-24.0f, 24.0f, 0.1f}, 0.0f),
        std::make_unique<juce::AudioParameterFloat>(
            "attack", "Attack",
            juce::NormalisableRange{1.0f, 400.0f, 1.0f}, 10.0f),
        std::make_unique<juce::AudioParameterFloat>(
            "release", "Release",
            juce::NormalisableRange{1.0f, 4000.0f, 1.0f}, 200.0f)})
{
    thresholdParameter = parameters.getRawParameterValue("threshold");
    ratioParameter = parameters.getRawParameterValue("ratio");
    gainParameter = parameters.getRawParameterValue("gain");
    attackParameter = parameters.getRawParameterValue("attack");
    releaseParameter = parameters.getRawParameterValue("release");
}

CompressorAudioProcessor::~CompressorAudioProcessor() {}

const juce::String CompressorAudioProcessor::getName() const { return "Compressor"; }
bool CompressorAudioProcessor::acceptsMidi() const { return false; }
bool CompressorAudioProcessor::producesMidi() const { return false; }
bool CompressorAudioProcessor::isMidiEffect() const { return false; }
double CompressorAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int CompressorAudioProcessor::getNumPrograms() { return 1; }
int CompressorAudioProcessor::getCurrentProgram() { return 0; }
void CompressorAudioProcessor::setCurrentProgram(int) {}
const juce::String CompressorAudioProcessor::getProgramName(int) { return {}; }
void CompressorAudioProcessor::changeProgramName(int, const juce::String&) {}

void CompressorAudioProcessor::prepareToPlay(double sampleRate, int)
{
    filter.setSamplingRate(static_cast<float>(sampleRate));
}

void CompressorAudioProcessor::releaseResources() {}

bool CompressorAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void CompressorAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    float threshold = *parameters.getRawParameterValue("threshold");
    float ratio = *parameters.getRawParameterValue("ratio");
    float gainDb = *parameters.getRawParameterValue("gain");
    float attack = *parameters.getRawParameterValue("attack");
    float release = *parameters.getRawParameterValue("release");

    filter.setThreshold(threshold);
    filter.setRatio(ratio);
    filter.setGain(gainDb);
    filter.setAttack(attack);
    filter.setRelease(release);

    filter.processBlock(buffer, midiMessages);
}

bool CompressorAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* CompressorAudioProcessor::createEditor()
{
    return new CompressorAudioProcessorEditor(*this, parameters);
}

void CompressorAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void CompressorAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}
