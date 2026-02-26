#include "AudioEngine.h"
#include <algorithm>

AudioEngine::AudioEngine()
{
    
    latestInput.setSize(2, 48000, false, false, true);
    processedInput.setSize(2, 48000, false, false, true);

    
    deviceManager.initialise(2, 2, nullptr, true);
    deviceManager.addAudioCallback(this);
}

AudioEngine::~AudioEngine()
{
    deviceManager.removeAudioCallback(this);
}

void AudioEngine::start()
{
    isRunning = true;
}

void AudioEngine::stop()
{
    isRunning = false;
}

void AudioEngine::startRecording()
{
    isRecording = true;
}

void AudioEngine::stopRecording()
{
    isRecording = false;
}

void AudioEngine::startPlayback()
{
    track.resetReadPosition();
    isPlaying = true;
}

void AudioEngine::stopPlayback()
{
    isPlaying = false;
}

void AudioEngine::setInputGain(float newGain)
{
    inputGain = newGain;
}

void AudioEngine::setOutputGain(float newGain)
{
    outputGain = newGain;
}

void AudioEngine::setMonitoringEnabled(bool shouldMonitor)
{
    monitoringEnabled = shouldMonitor;
}

void AudioEngine::process(float* left, float* right, int numSamples)
{
    std::fill(left,  left  + numSamples, 0.0f);
    std::fill(right, right + numSamples, 0.0f);

    if (isPlaying)
        track.playSamples(left, right, numSamples);
}

void AudioEngine::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                   int numInputChannels,
                                                   float* const* outputChannelData,
                                                   int numOutputChannels,
                                                   int numSamples,
                                                   const juce::AudioIODeviceCallbackContext& context)
{
    if (!isRunning || numOutputChannels < 2)
    {
        for (int ch = 0; ch < numOutputChannels; ++ch)
            std::fill(outputChannelData[ch], outputChannelData[ch] + numSamples, 0.0f);
        return;
    }

    float* leftOut  = outputChannelData[0];
    float* rightOut = outputChannelData[1];

    const float* leftIn  = (numInputChannels > 0 ? inputChannelData[0] : nullptr);
    const float* rightIn = (numInputChannels > 1 ? inputChannelData[1] : nullptr);

    if (leftIn == nullptr)
    {
        std::fill(leftOut,  leftOut  + numSamples, 0.0f);
        std::fill(rightOut, rightOut + numSamples, 0.0f);
        return;
    }

    const int maxSamples = std::min(numSamples, processedInput.getNumSamples());

    
    processedInput.clear();
    processedInput.copyFrom(0, 0, leftIn, maxSamples, inputGain);

    if (rightIn != nullptr)
        processedInput.copyFrom(1, 0, rightIn, maxSamples, inputGain);
    else
        processedInput.copyFrom(1, 0, leftIn, maxSamples, inputGain);

    
    latestInput.makeCopyOf(processedInput, true);

    
    inputBuffer.push(processedInput.getReadPointer(0), maxSamples);

    
    if (isRecording)
    {
        const float* recInputs[2];
        recInputs[0] = processedInput.getReadPointer(0);
        recInputs[1] = processedInput.getReadPointer(1);
        track.recordSamples(recInputs, 2, maxSamples);
    }

  
    std::fill(leftOut,  leftOut  + numSamples, 0.0f);
    std::fill(rightOut, rightOut + numSamples, 0.0f);

   
    if (monitoringEnabled && !isRecording)
    {
        const float* monL = processedInput.getReadPointer(0);
        const float* monR = processedInput.getReadPointer(1);

        for (int i = 0; i < maxSamples; ++i)
        {
            leftOut[i]  += monL[i];
            rightOut[i] += monR[i];
        }
    }

    
    if (isPlaying)
        track.playSamples(leftOut, rightOut, numSamples);

    
    dspChain.process(leftOut, rightOut, numSamples);

    
    if (outputGain != 1.0f)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            leftOut[i]  *= outputGain;
            rightOut[i] *= outputGain;
        }
    }
}