#include "DeviceManager.h"

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

    float* channelData[] = { processedLeft.data(), processedRight.data() };
    juce::AudioBuffer<float> blockBuffer(channelData, 2, numSamples);
    const auto shouldRenderPlayback = playbackManager != nullptr && playbackManager->isPlaying();
    const auto playbackStartSample = shouldRenderPlayback ? playbackManager->getCurrentSamplePosition() : 0;

    if (shouldRenderPlayback)
    {
        if (arrangementState != nullptr)
            arrangementState->render(blockBuffer, playbackStartSample);
        playbackManager->advance(numSamples);
    }

    if (fxProcessCallback)
        fxProcessCallback(blockBuffer);

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
    const double sr = device.getCurrentSampleRate();
    const int blockSize = device.getCurrentBufferSizeSamples();
    inputNode.prepare(sr, blockSize);
    if (playbackManager != nullptr)
        playbackManager->prepare(sr);
    if (recordingManager != nullptr)
        recordingManager->prepare(sr);
    processedLeft.resize(blockSize);
    processedRight.resize(blockSize);
}
