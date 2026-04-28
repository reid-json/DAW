#include "AudioEngine.h"
#include <cmath>

namespace
{
std::vector<float> resampleChannel(const float* sourceData,
                                   int sourceSamples,
                                   double sourceRate,
                                   double targetRate)
{
    if (sourceData == nullptr || sourceSamples <= 0)
        return {};

    if (sourceRate <= 0.0 || targetRate <= 0.0 || sourceRate == targetRate)
        return std::vector<float>(sourceData, sourceData + sourceSamples);

    const auto ratio = targetRate / sourceRate;
    const auto targetSamples = juce::jmax(1, static_cast<int>(std::llround(static_cast<double>(sourceSamples) * ratio)));

    std::vector<float> resampled(static_cast<size_t>(targetSamples), 0.0f);
    for (int index = 0; index < targetSamples; ++index)
    {
        const auto sourceIndex = static_cast<double>(index) / ratio;
        const auto baseIndex = juce::jlimit(0, sourceSamples - 1, static_cast<int>(std::floor(sourceIndex)));
        const auto nextIndex = juce::jmin(sourceSamples - 1, baseIndex + 1);
        const auto fraction = static_cast<float>(sourceIndex - static_cast<double>(baseIndex));
        const auto first = sourceData[baseIndex];
        const auto second = sourceData[nextIndex];
        resampled[static_cast<size_t>(index)] = first + ((second - first) * fraction);
    }

    return resampled;
}
}

AudioEngine::AudioEngine() = default;

AudioEngine::~AudioEngine()
{
    shutdown();
}

void AudioEngine::initialise(int inputChannels, int outputChannels)
{
    arrangementState.initialiseDefaults();

    juce::String error = deviceManager.initialise(inputChannels, outputChannels, nullptr, true, "ASIO", nullptr);
    if (error.isNotEmpty())
        deviceManager.initialiseWithDefaultDevices(inputChannels, outputChannels);

    ioDeviceManager.setArrangementState(&arrangementState);
    ioDeviceManager.setRecordingManager(&recordingManager);
    ioDeviceManager.setPlaybackManager(&playbackManager);
    ioDeviceManager.setInputMonitoringEnabled(inputMonitoringEnabled);
    ioDeviceManager.initialise(deviceManager, inputChannels, outputChannels);

    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        recordingManager.prepare(device->getCurrentSampleRate());
        playbackManager.prepare(device->getCurrentSampleRate());
    }

}

void AudioEngine::shutdown()
{
    stopRecording();
    stopPlayback();

    ioDeviceManager.shutdown();
    deviceManager.closeAudioDevice();
}

bool AudioEngine::startRecording()
{
    juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
    playbackManager.stop();
    return recordingManager.startRecording();
}

bool AudioEngine::stopRecording()
{
    juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
    std::vector<RecordedClip> finishedClips;
    const bool stopped = recordingManager.stopRecording(finishedClips);

    for (const auto& clip : finishedClips)
        arrangementState.addRecentRecording(clip);

    return stopped;
}

double AudioEngine::getCurrentRecordingLengthSeconds() const
{
    const auto sampleRate = recordingManager.getSampleRate();
    return sampleRate > 0.0
        ? static_cast<double>(recordingManager.getRecordedSampleCount()) / sampleRate
        : 0.0;
}

double AudioEngine::getCurrentTransportPositionSeconds() const
{
    if (recordingManager.isRecording())
        return getCurrentRecordingLengthSeconds();

    return playbackManager.getCurrentPositionSeconds();
}

double AudioEngine::getTimelineSampleRate() const
{
    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        const auto currentRate = device->getCurrentSampleRate();
        if (currentRate > 0.0)
            return currentRate;
    }

    const auto playbackRate = playbackManager.getSampleRate();
    if (playbackRate > 0.0)
        return playbackRate;

    const auto recordingRate = recordingManager.getSampleRate();
    return recordingRate > 0.0 ? recordingRate : 44100.0;
}

bool AudioEngine::startPlayback()
{
    juce::ScopedLock sl(deviceManager.getAudioCallbackLock());

    if (recordingManager.isRecording())
        return false;

    if (playbackManager.isPaused())
    {
        playbackManager.resume();
        return true;
    }

    if (arrangementState.getTimelinePlacements().empty())
        return false;

    playbackManager.start();
    return true;
}

bool AudioEngine::togglePlaybackPause()
{
    juce::ScopedLock sl(deviceManager.getAudioCallbackLock());

    if (recordingManager.isRecording())
        return false;

    if (playbackManager.isPlaying())
    {
        playbackManager.pause();
        return true;
    }

    if (playbackManager.isPaused())
    {
        playbackManager.resume();
        return true;
    }

    return false;
}

void AudioEngine::stopPlayback()
{
    juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
    playbackManager.stop();
}

void AudioEngine::setInputMonitoringEnabled(bool shouldMonitor)
{
    juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
    inputMonitoringEnabled = shouldMonitor;
    ioDeviceManager.setInputMonitoringEnabled(shouldMonitor);
}

bool AudioEngine::placeRecentAssetDirectToTimeline(int assetId, int timelineTrackId, juce::int64 startSample)
{
    juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
    return arrangementState.placeRecentAssetDirectToTimeline(assetId, timelineTrackId, startSample);
}

bool AudioEngine::moveTimelinePlacement(int placementId, int timelineTrackId, juce::int64 startSample)
{
    juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
    return arrangementState.moveTimelinePlacement(placementId, timelineTrackId, startSample);
}

bool AudioEngine::removeTimelinePlacement(int placementId)
{
    juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
    return arrangementState.removeTimelinePlacement(placementId);
}

int AudioEngine::importAudioFileAsRecentAsset(const juce::File& file)
{
    if (!file.existsAsFile())
        return -1;

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr)
        return -1;

    juce::AudioBuffer<float> fileBuffer(static_cast<int>(juce::jmin<juce::uint32>(2, reader->numChannels)),
                                        static_cast<int>(reader->lengthInSamples));
    reader->read(&fileBuffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);

    const auto targetSampleRate = getTimelineSampleRate();

    RecordedClip clip;
    clip.name = file.getFileName();
    clip.sampleRate = targetSampleRate;
    clip.startSample = 0;

    if (fileBuffer.getNumSamples() > 0)
    {
        const auto* left = fileBuffer.getReadPointer(0);
        const auto* right = fileBuffer.getNumChannels() > 1 ? fileBuffer.getReadPointer(1) : left;
        clip.leftChannel = resampleChannel(left, fileBuffer.getNumSamples(), reader->sampleRate, targetSampleRate);
        clip.rightChannel = resampleChannel(right, fileBuffer.getNumSamples(), reader->sampleRate, targetSampleRate);
    }

    juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
    auto* asset = arrangementState.addAsset(file.getFileName(), AssetKind::audioFile, clip);
    return asset != nullptr ? asset->assetId : -1;
}

int AudioEngine::createPatternAsset(const juce::String& name, double lengthSeconds, std::vector<PatternNote> patternNotes, const juce::String& instrumentName)
{
    auto clip = makePatternClip(name, lengthSeconds);
    juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
    auto* asset = arrangementState.addAsset(name, AssetKind::pianoRollPattern, clip);
    if (asset != nullptr)
    {
        asset->patternNotes = std::move(patternNotes);
        asset->instrumentName = instrumentName;
    }
    return asset != nullptr ? asset->assetId : -1;
}

bool AudioEngine::updatePatternAsset(int assetId, const juce::String& name, double lengthSeconds, std::vector<PatternNote> patternNotes, const juce::String& instrumentName)
{
    auto clip = makePatternClip(name, lengthSeconds);
    juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
    bool ok = arrangementState.renameAsset(assetId, name) && arrangementState.updateAssetClip(assetId, clip);
    if (auto* asset = arrangementState.findAsset(assetId))
    {
        asset->patternNotes = std::move(patternNotes);
        asset->instrumentName = instrumentName;
    }
    return ok;
}

RecordedClip AudioEngine::makePatternClip(const juce::String& name, double lengthSeconds) const
{
    RecordedClip clip;
    clip.name = name;
    clip.sampleRate = getTimelineSampleRate();
    int samples = static_cast<int>(std::round(clip.sampleRate * juce::jmax(0.1, lengthSeconds)));
    clip.leftChannel.resize(static_cast<size_t>(samples), 0.0f);
    clip.rightChannel.resize(static_cast<size_t>(samples), 0.0f);
    return clip;
}

bool AudioEngine::renameAsset(int assetId, const juce::String& newName)
{
    juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
    return arrangementState.renameAsset(assetId, newName);
}

void AudioEngine::setPatternTrackRenderer(ArrangementState::PatternTrackRenderer renderer)
{
    juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
    arrangementState.setPatternTrackRenderer(std::move(renderer));
}

void AudioEngine::setTrackFxProcessor(ArrangementState::TrackFxProcessor processor)
{
    juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
    arrangementState.setTrackFxProcessor(std::move(processor));
}
