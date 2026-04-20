#include "PluginProcessor.h"
#include "PluginEditor.h"

GainPanAudioProcessor::GainPanAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    parameters(*this, nullptr, juce::Identifier("GainPanPlugin"),
        {
            std::make_unique<juce::AudioParameterFloat>("gain", "Gain", 0.0f, 2.0f, 1.0f),
            std::make_unique<juce::AudioParameterFloat>("pan",  "Pan", -1.0f, 1.0f, 0.0f)
        })
{
    gainParameter = parameters.getRawParameterValue("gain");
    panParameter = parameters.getRawParameterValue("pan");
}

GainPanAudioProcessor::~GainPanAudioProcessor() {}

const juce::String GainPanAudioProcessor::getName() const { return "GainPanFilter"; }
bool GainPanAudioProcessor::acceptsMidi() const { return false; }
bool GainPanAudioProcessor::producesMidi() const { return false; }
bool GainPanAudioProcessor::isMidiEffect() const { return false; }
double GainPanAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int GainPanAudioProcessor::getNumPrograms() { return 1; }
int GainPanAudioProcessor::getCurrentProgram() { return 0; }
void GainPanAudioProcessor::setCurrentProgram(int) {}
const juce::String GainPanAudioProcessor::getProgramName(int) { return {}; }
void GainPanAudioProcessor::changeProgramName(int, const juce::String&) {}

void GainPanAudioProcessor::prepareToPlay(double sampleRate, int)
{
    filter.setSamplingRate(static_cast<float>(sampleRate));
}

void GainPanAudioProcessor::releaseResources() {}

bool GainPanAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void GainPanAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    const auto gain = gainParameter->load();
    const auto pan = panParameter->load();
    filter.setGain(gain);
    filter.setPan(pan);
    filter.processBlock(buffer, midiMessages);
}

bool GainPanAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* GainPanAudioProcessor::createEditor()
{
    return new GainPanAudioProcessorEditor(*this, parameters);
}

void GainPanAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void GainPanAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}
