#include "DeviceManager.h"

DeviceManager::DeviceManager()
{
}

void DeviceManager::initialise(juce::AudioDeviceManager& deviceManager, int inputChannels, int outputChannels)
{
    juce::ignoreUnused(inputChannels, outputChannels);

    deviceManagerPtr = &deviceManager;
    deviceManager.addAudioCallback(this);
    
    if (auto device = deviceManager.getCurrentAudioDevice())
        prepareForDevice(*device);
}

void DeviceManager::shutdown()
{
    if (deviceManagerPtr)
        deviceManagerPtr->removeAudioCallback(this);
}

void DeviceManager::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                     int numInputChannels,
                                                     float* const* outputChannelData,
                                                     int numOutputChannels,
                                                     int numSamples,
                                                     const juce::AudioIODeviceCallbackContext& context)
{
    juce::ignoreUnused(context);

    inputNode.processInput(inputChannelData, numInputChannels, numSamples);

    const float* left = inputNode.getLeftBuffer();
    const float* right = inputNode.getRightBuffer();

    if (recordingManager != nullptr)
        recordingManager->processInput(left, right, numSamples);

    std::copy(left, left + numSamples, processedLeft.begin());
    std::copy(right, right + numSamples, processedRight.begin());

    if (!inputMonitoringEnabled)
    {
        juce::FloatVectorOperations::clear(processedLeft.data(), numSamples);
        juce::FloatVectorOperations::clear(processedRight.data(), numSamples);
    }

    juce::MidiBuffer blockMidi;
    midiCollector.removeNextBlockOfMessages(blockMidi, numSamples);
    midiInputNode.processMidiInput(blockMidi);

    float* channelData[] = { processedLeft.data(), processedRight.data() };
    juce::AudioBuffer<float> blockBuffer(channelData, 2, numSamples);
    const auto shouldRenderPlayback = playbackManager != nullptr && playbackManager->isPlaying();
    const auto playbackStartSample = shouldRenderPlayback ? playbackManager->getCurrentSamplePosition() : 0;
    
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

            track->processBlock(tempBuffer, blockMidi, shouldRenderPlayback, playbackStartSample);

            mixBuffer.addFrom(0, 0, tempBuffer, 0, 0, numSamples);
            mixBuffer.addFrom(1, 0, tempBuffer, 1, 0, numSamples);
        }

        std::copy(mixLeft.begin(), mixLeft.begin() + numSamples, processedLeft.begin());
        std::copy(mixRight.begin(), mixRight.begin() + numSamples, processedRight.begin());
    }

    if (shouldRenderPlayback)
    {
        if (arrangementState != nullptr)
            arrangementState->render(blockBuffer, playbackStartSample);
        playbackManager->advance(numSamples);
    }

    outputNode.processOutput(outputChannelData, numOutputChannels, processedLeft.data(), processedRight.data(), numSamples);
}

void DeviceManager::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    prepareForDevice(*device);
}

void DeviceManager::audioDeviceStopped()
{
}

void DeviceManager::prepareForDevice(juce::AudioIODevice& device)
{
    sampleRate = device.getCurrentSampleRate();
    const int blockSize = device.getCurrentBufferSizeSamples();
    inputNode.prepare(sampleRate, blockSize);
    outputNode.prepare(sampleRate, blockSize);
    midiInputNode.prepare(sampleRate, blockSize);
    midiCollector.reset(sampleRate);
    if (playbackManager != nullptr)
        playbackManager->prepare(sampleRate);
    if (recordingManager != nullptr)
        recordingManager->prepare(sampleRate);
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
