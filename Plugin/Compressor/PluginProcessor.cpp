/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
NewProjectAudioProcessor::NewProjectAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#else
    :
#endif
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
            juce::NormalisableRange{1.0f, 4000.0f, 1.0f}, 200.0f)}){
            thresholdParameter = parameters.getRawParameterValue("threshold");
            ratioParameter = parameters.getRawParameterValue("ratio");
            gainParameter = parameters.getRawParameterValue("gain");
            attackParameter = parameters.getRawParameterValue("attack");
            releaseParameter = parameters.getRawParameterValue("release");
        }

NewProjectAudioProcessor::~NewProjectAudioProcessor()
{
}

//==============================================================================
const juce::String NewProjectAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool NewProjectAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool NewProjectAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool NewProjectAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double NewProjectAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int NewProjectAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int NewProjectAudioProcessor::getCurrentProgram()
{
    return 0;
}

void NewProjectAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String NewProjectAudioProcessor::getProgramName (int index)
{
    return {};
}

void NewProjectAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void NewProjectAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    filter.setSamplingRate(static_cast<float>(sampleRate));

    /*filter.setAttack(*parameters.getRawParameterValue("attack"));
    filter.setRelease(*parameters.getRawParameterValue("release"));*/
}

void NewProjectAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool NewProjectAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void NewProjectAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    // Clear extra output channels
    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Load parameter values from ValueTreeState
    float threshold = *parameters.getRawParameterValue("threshold");
    float ratio = *parameters.getRawParameterValue("ratio");
    float gainDb = *parameters.getRawParameterValue("gain"); // in dB
    float attack = *parameters.getRawParameterValue("attack");
    float release = *parameters.getRawParameterValue("release");

    // Set parameters in the compressor
    filter.setThreshold(threshold);
    filter.setRatio(ratio);
    filter.setGain(gainDb);
    filter.setAttack(attack);
    filter.setRelease(release);

    // Process audio buffer
    filter.processBlock(buffer, /*juce::MidiBuffer{}*/ midiMessages);
}

//==============================================================================
bool NewProjectAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* NewProjectAudioProcessor::createEditor()
{
    return new NewProjectAudioProcessorEditor (*this, parameters);
}

//==============================================================================
void NewProjectAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void NewProjectAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NewProjectAudioProcessor();
}
