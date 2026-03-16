#include "DeviceManager.h"

DeviceManager::DeviceManager()
{
}

void DeviceManager::initialise(juce::AudioDeviceManager& deviceManager, int inputChannels, int outputChannels)
{
    deviceManagerPtr = &deviceManager;
    deviceManager.addAudioCallback(this);
    
    if (auto device = deviceManager.getCurrentAudioDevice())
    {
        sampleRate = device->getCurrentSampleRate();
        int blockSize = device->getCurrentBufferSizeSamples();
        inputNode.prepare(sampleRate, blockSize);
        outputNode.prepare(sampleRate, blockSize);
        midiInputNode.prepare(sampleRate, blockSize);
        midiCollector.reset(sampleRate);
        processedLeft.resize(blockSize);
        processedRight.resize(blockSize);
        mixLeft.resize(blockSize);
        mixRight.resize(blockSize);
        tempLeft.resize(blockSize);
        tempRight.resize(blockSize);
        
        if (tracks)
        {
            for (auto& track : *tracks)
                track->prepare(sampleRate, blockSize);
        }
    }
}

void DeviceManager::shutdown()
{
    if (deviceManagerPtr)
        deviceManagerPtr->removeAudioCallback(this);
}
// inputChannelData is for raw hardware input
// outputChannelData is where the final audio has to be written to
// numSamples = block size
// context = timing

void DeviceManager::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                     int numInputChannels,
                                                     float* const* outputChannelData,
                                                     int numOutputChannels,
                                                     int numSamples,
                                                     const juce::AudioIODeviceCallbackContext& context)
{
    // input from device
    inputNode.processInput(inputChannelData, numInputChannels, numSamples);
    
    // get input buffers
    const float* left = inputNode.getLeftBuffer();
    const float* right = inputNode.getRightBuffer();
    
    // Copy to processing buffers 
    std::copy(left, left + numSamples, processedLeft.begin());
    std::copy(right, right + numSamples, processedRight.begin());
    
    // Process in block for midi sync 
    juce::MidiBuffer blockMidi;
    midiCollector.removeNextBlockOfMessages(blockMidi, numSamples);
    
    midiInputNode.processMidiInput(blockMidi);
    

    
   
    float* channelData[] = { processedLeft.data(), processedRight.data() };
    juce::AudioBuffer<float> blockBuffer(channelData, 2, numSamples);
    
    if (tracks && tracks->size() > 0)
    {
        float* mixData[] = { mixLeft.data(), mixRight.data() };
        juce::AudioBuffer<float> mixBuffer(mixData, 2, numSamples);
        mixBuffer.clear();
        
        for (auto& track : *tracks)
        {
            float* tempData[] = { tempLeft.data(), tempRight.data() };
            juce::AudioBuffer<float> tempBuffer(tempData, 2, numSamples);
            
          
            std::copy(processedLeft.begin(), processedLeft.begin() + numSamples, tempLeft.begin());
            std::copy(processedRight.begin(), processedRight.begin() + numSamples, tempRight.begin());
            
            track->processBlock(tempBuffer, blockMidi);
            
            mixBuffer.addFrom(0, 0, tempBuffer, 0, 0, numSamples);
            mixBuffer.addFrom(1, 0, tempBuffer, 1, 0, numSamples);
        }
        
        // copy back to processed buffers
        std::copy(mixLeft.begin(), mixLeft.begin() + numSamples, processedLeft.begin());
        std::copy(mixRight.begin(), mixRight.begin() + numSamples, processedRight.begin());
    }
    
    // output to audio device
    outputNode.processOutput(outputChannelData, numOutputChannels, processedLeft.data(), processedRight.data(), numSamples);
}

void DeviceManager::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    sampleRate = device->getCurrentSampleRate();
    int blockSize = device->getCurrentBufferSizeSamples();
    inputNode.prepare(sampleRate, blockSize);
    outputNode.prepare(sampleRate, blockSize);
    midiInputNode.prepare(sampleRate, blockSize);
    midiCollector.reset(sampleRate);
    processedLeft.resize(blockSize);
    processedRight.resize(blockSize);
    mixLeft.resize(blockSize);
    mixRight.resize(blockSize);
    tempLeft.resize(blockSize);
    tempRight.resize(blockSize);
    
    if (tracks)
    {
        for (auto& track : *tracks)
            track->prepare(sampleRate, blockSize);
    }
}

void DeviceManager::audioDeviceStopped()
{
}
