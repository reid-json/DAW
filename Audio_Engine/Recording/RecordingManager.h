#pragma once

#include "../Tracks/TrackInputRouter.h"

class RecordingManager
{
public:
    void prepare(double newSampleRate);

    bool startRecording();
    bool stopRecording(std::vector<RecordedClip>& finishedClips);

    void processInput(const float* left, const float* right, int numSamples);

    bool isRecording() const { return recordingActive; }
    juce::int64 getRecordedSampleCount() const { return recordedSampleCount; }
    double getSampleRate() const { return sampleRate; }

private:
    RecordedClip buildClip(const juce::String& clipName);

    double sampleRate = 44100.0;
    int nextClipId = 1;
    juce::uint32 takeCounter = 0;
    juce::uint32 currentTakeId = 0;
    juce::int64 recordedSampleCount = 0;
    bool recordingActive = false;
    std::vector<float> leftChannel;
    std::vector<float> rightChannel;
};
