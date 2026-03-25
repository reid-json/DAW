#include "TrackInputRouter.h"
#include <cmath>

TrackInputRouter::TrackInputRouter(int id)
    : synth(std::make_unique<MidiSynth>())
    , trackName("Track " + juce::String(id + 1))
    , trackId(id)
{
    startTime = juce::Time::getMillisecondCounter();
}

void TrackInputRouter::prepare(double sr, int bs)
{
    synth->prepare(sr, bs);
}

void TrackInputRouter::processBlock(juce::AudioBuffer<float>& buffer,
                                    const juce::MidiBuffer& midi,
                                    bool shouldRenderRecordedAudio,
                                    juce::int64 playbackStartSample)
{
    const int numSamples = buffer.getNumSamples();

    if (juce::Time::getMillisecondCounter() - startTime < 1000)
    {
        buffer.clear();
        return;
    }

    if (!inputEnabled || !audioEnabled)
    {
        buffer.clear();
    }
    else if (std::abs(gain - 1.0f) > 0.001f)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.applyGain(ch, 0, numSamples, gain);
    }

    if (midiEnabled && synth)
        synth->renderNextBlock(buffer, midi, 0, numSamples);

    if (shouldRenderRecordedAudio)
        renderRecordedClips(buffer, playbackStartSample);
}

void TrackInputRouter::setGain(float gainDb)
{
    gain = std::pow(10.0f, gainDb / 20.0f);
}

void TrackInputRouter::addRecordedClip(RecordedClip clip)
{
    recordedClips.push_back(std::move(clip));
}

void TrackInputRouter::renderRecordedClips(juce::AudioBuffer<float>& buffer, juce::int64 playbackStartSample) const
{
    const auto blockEndSample = playbackStartSample + buffer.getNumSamples();

    for (const auto& clip : recordedClips)
    {
        const auto clipStart = clip.startSample;
        const auto clipEnd = clip.startSample + clip.getNumSamples();

        if (clipEnd <= playbackStartSample || clipStart >= blockEndSample)
            continue;

        const auto overlapStart = juce::jmax(playbackStartSample, clipStart);
        const auto overlapEnd = juce::jmin(blockEndSample, clipEnd);
        const auto overlapLength = static_cast<int>(overlapEnd - overlapStart);
        const auto bufferOffset = static_cast<int>(overlapStart - playbackStartSample);
        const auto clipOffset = static_cast<int>(overlapStart - clipStart);

        if (overlapLength <= 0)
            continue;

        buffer.addFrom(0, bufferOffset, clip.leftChannel.data() + clipOffset, overlapLength);
        buffer.addFrom(1, bufferOffset, clip.rightChannel.data() + clipOffset, overlapLength);
    }
}
