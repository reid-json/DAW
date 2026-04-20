#include "PluginProcessor.h"
#include "PluginEditor.h"

LowpassHighpassFilterAudioProcessor::LowpassHighpassFilterAudioProcessor()
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
parameters(*this, nullptr, juce::Identifier("LowpassAndHighpassPlugin"), 
{std::make_unique<juce::AudioParameterFloat>("cutoff_frequency", "Cutoff Frequency", juce::NormalisableRange{20.f, 20000.f, 0.1f, 0.2f, false}, 500.f),  
std::make_unique<juce::AudioParameterBool>("highpass", "Highpass", false)})
{
    cutoffFrequencyParameter = parameters.getRawParameterValue("cutoff_frequency");
    highpassParameter = parameters.getRawParameterValue("highpass");
}

LowpassHighpassFilterAudioProcessor::~LowpassHighpassFilterAudioProcessor()
{
}

const juce::String LowpassHighpassFilterAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool LowpassHighpassFilterAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool LowpassHighpassFilterAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool LowpassHighpassFilterAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double LowpassHighpassFilterAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int LowpassHighpassFilterAudioProcessor::getNumPrograms()
{
    return 1;
}

int LowpassHighpassFilterAudioProcessor::getCurrentProgram()
{
    return 0;
}

void LowpassHighpassFilterAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String LowpassHighpassFilterAudioProcessor::getProgramName (int index)
{
    return {};
}

void LowpassHighpassFilterAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void LowpassHighpassFilterAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    filter.setSamplingRate(static_cast<float>(sampleRate));
}

void LowpassHighpassFilterAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool LowpassHighpassFilterAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // Keep the processor limited to mono and stereo layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // Require matching input and output layouts for non-synth builds.
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void LowpassHighpassFilterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that do not have matching input data.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const auto cutoffFrequency = cutoffFrequencyParameter->load();
    const auto highpass = *highpassParameter < 0.5f ? false : true;

    filter.setCutOffFrequency(cutoffFrequency);
    filter.setHighpass(highpass);

    filter.processBlock(buffer, midiMessages);
}

bool LowpassHighpassFilterAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* LowpassHighpassFilterAudioProcessor::createEditor()
{
    return new LowpassHighpassFilterAudioProcessorEditor (*this, parameters);
}

void LowpassHighpassFilterAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ignoreUnused(destData);
}

void LowpassHighpassFilterAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused(data, sizeInBytes);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LowpassHighpassFilterAudioProcessor();
}
