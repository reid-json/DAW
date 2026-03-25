#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "../MIDI/MidiSynth.h"
#include <memory>
#include <vector>

struct RecordedClip
{
    int clipId = 0;
    juce::String name;
    juce::uint32 takeId = 0;
    double sampleRate = 44100.0;
    juce::int64 startSample = 0;
    std::vector<float> leftChannel;
    std::vector<float> rightChannel;

    int getNumSamples() const { return static_cast<int>(leftChannel.size()); }
    double getLengthSeconds() const { return sampleRate > 0.0 ? static_cast<double>(getNumSamples()) / sampleRate : 0.0; }
};

class TrackInputRouter
{
public:
    enum class OutputTarget
    {
        master,
        track
    };

    explicit TrackInputRouter(int trackId);
    ~TrackInputRouter() = default;

    void prepare(double sampleRate, int blockSize);
    void processBlock(juce::AudioBuffer<float>& buffer,
                      const juce::MidiBuffer& midi,
                      bool shouldRenderRecordedAudio = false,
                      juce::int64 playbackStartSample = 0);

    void setInputEnabled(bool enabled) { inputEnabled = enabled; }
    void setAudioInputEnabled(bool enabled) { audioEnabled = enabled; }
    void setMidiEnabled(bool enabled) { midiEnabled = enabled; }

    void setGain(float gainDb);

    int getTrackId() const { return trackId; }
    const juce::String& getTrackName() const { return trackName; }
    void setTrackName(const juce::String& newName) { trackName = newName; }

    void addRecordedClip(RecordedClip clip);
    const std::vector<RecordedClip>& getRecordedClips() const { return recordedClips; }
    void clearRecordedClips() { recordedClips.clear(); }

    void setMixerTrackId(int id) { mixerTrackId = id; }
    int getMixerTrackId() const { return mixerTrackId; }

    void setOutputTarget(OutputTarget target) { outputTarget = target; }
    OutputTarget getOutputTarget() const { return outputTarget; }

    void setOutputTrackId(int id) { outputTrackId = id; }
    int getOutputTrackId() const { return outputTrackId; }

    void setTimelineTrackId(int id) { timelineTrackId = id; }
    int getTimelineTrackId() const { return timelineTrackId; }

private:
    void renderRecordedClips(juce::AudioBuffer<float>& buffer, juce::int64 playbackStartSample) const;

    std::unique_ptr<MidiSynth> synth;
    std::vector<RecordedClip> recordedClips;
    juce::String trackName;
    float gain = 1.0f;
    bool inputEnabled = true;
    bool audioEnabled = true;
    bool midiEnabled = true;
    int trackId = 0;
    int mixerTrackId = -1;
    int outputTrackId = -1;
    int timelineTrackId = 0;
    OutputTarget outputTarget = OutputTarget::master;
    juce::uint32 startTime = 0;
};
