#pragma once
#include "../Root/AudioEngine.h"

class AudioInputHandler
{
public:
    AudioInputHandler(AudioEngine& engine);

    void handleIncomingAudio(const float* data, int numSamples);

private:
    AudioEngine& engineRef;
};