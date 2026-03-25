#include "RecordingManager.h"

void RecordingManager::prepare(double newSampleRate)
{
    sampleRate = newSampleRate;
}

bool RecordingManager::startRecording()
{
    if (recordingActive)
        return false;

    recordedSampleCount = 0;
    leftChannel.clear();
    rightChannel.clear();
    currentTakeId = ++takeCounter;
    recordingActive = true;
    return true;
}

bool RecordingManager::stopRecording(std::vector<RecordedClip>& finishedClips)
{
    if (!recordingActive)
        return false;

    if (!leftChannel.empty())
        finishedClips.push_back(buildClip("Take " + juce::String(currentTakeId)));

    currentTakeId = 0;
    recordedSampleCount = 0;
    recordingActive = false;
    leftChannel.clear();
    rightChannel.clear();
    return true;
}

void RecordingManager::processInput(const float* left, const float* right, int numSamples)
{
    if (!recordingActive || numSamples <= 0 || left == nullptr || right == nullptr)
        return;

    leftChannel.insert(leftChannel.end(), left, left + numSamples);
    rightChannel.insert(rightChannel.end(), right, right + numSamples);
    recordedSampleCount += numSamples;
}

RecordedClip RecordingManager::buildClip(const juce::String& clipName)
{
    RecordedClip clip;
    clip.clipId = nextClipId++;
    clip.name = clipName;
    clip.takeId = currentTakeId;
    clip.sampleRate = sampleRate;
    clip.startSample = 0;
    clip.leftChannel = leftChannel;
    clip.rightChannel = rightChannel;
    return clip;
}
