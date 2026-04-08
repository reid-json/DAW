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

    activeMidiInputs.clear();
    for (const auto& devInfo : juce::MidiInput::getAvailableDevices())
    {
        if (auto input = juce::MidiInput::openDevice(devInfo.identifier, this))
        {
            input->start();
            activeMidiInputs.push_back(std::move(input));
        }
    }

    activeMidiOutputs.clear();
    for (const auto& devInfo : juce::MidiOutput::getAvailableDevices())
    {
        if (auto output = juce::MidiOutput::openDevice(devInfo.identifier))
        {
            sendDiscoveryHandshake(output.get());
            activeMidiOutputs.push_back(std::move(output));
        }
    }
}

void AudioEngine::shutdown()
{
    stopRecording();
    stopPlayback();

    for (auto& input : activeMidiInputs)
        input->stop();

    activeMidiInputs.clear();
    activeMidiOutputs.clear();
    ioDeviceManager.shutdown();
    deviceManager.closeAudioDevice();
}

void AudioEngine::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message)
{
    juce::ignoreUnused(source);

    if (message.getChannel() == 16)
        return;

    if (!message.isNoteOn() && !message.isNoteOff() && !message.isController() && !message.isPitchWheel())
        return;

    auto msg = message;

    msg.setTimeStamp(0);
    ioDeviceManager.getMidiCollector().addMessageToQueue(msg);
}

void AudioEngine::sendDiscoveryHandshake(juce::MidiOutput* output)
{
    if (output == nullptr)
        return;

    const auto name = output->getName();
    if (name.containsIgnoreCase("Launchkey") || name.containsIgnoreCase("Novation"))
    {
        juce::MidiMessage handshake = juce::MidiMessage::noteOn(16, 12, static_cast<juce::uint8>(127));
        output->sendMessageNow(handshake);
    }
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

CentralTrackSlot* AudioEngine::addCentralTrackSlot()
{
    juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
    return arrangementState.createCentralTrackSlot();
}

bool AudioEngine::assignRecentAssetToCentralTrack(int assetId, int slotId)
{
    juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
    return arrangementState.assignRecentAssetToCentralTrack(assetId, slotId);
}

bool AudioEngine::placeCentralTrackOnTimeline(int slotId, int timelineTrackId)
{
    juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
    return arrangementState.placeCentralTrackOnTimeline(slotId, timelineTrackId);
}

bool AudioEngine::placeRecentAssetDirectToTimeline(int assetId, int timelineTrackId)
{
    juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
    return arrangementState.placeRecentAssetDirectToTimeline(assetId, timelineTrackId);
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

bool AudioEngine::setCentralTrackMixerTrack(int slotId, int mixerTrackId)
{
    juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
    return arrangementState.setCentralTrackMixerTrack(slotId, mixerTrackId);
}

bool AudioEngine::setCentralTrackTimelineTrack(int slotId, int timelineTrackId)
{
    juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
    return arrangementState.setCentralTrackTimelineTrack(slotId, timelineTrackId);
}

bool AudioEngine::setCentralTrackOutputToMaster(int slotId)
{
    juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
    return arrangementState.setCentralTrackOutputToMaster(slotId);
}

bool AudioEngine::setCentralTrackOutputToTrack(int slotId, int destinationSlotId)
{
    juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
    return arrangementState.setCentralTrackOutputToTrack(slotId, destinationSlotId);
}

bool AudioEngine::setCentralTrackLiveInputArmed(int slotId, bool shouldArm)
{
    juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
    return arrangementState.setCentralTrackLiveInputArmed(slotId, shouldArm);
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

int AudioEngine::createMidiPatternAsset()
{
    RecordedClip clip;
    clip.name = "Pattern " + juce::String(juce::Time::getCurrentTime().toMilliseconds() % 10000);
    clip.sampleRate = 44100.0;

    const int samples = static_cast<int>(clip.sampleRate * 2.0);
    clip.leftChannel.resize(static_cast<size_t>(samples), 0.0f);
    clip.rightChannel.resize(static_cast<size_t>(samples), 0.0f);

    for (int i = 0; i < samples; ++i)
    {
        const double time = static_cast<double>(i) / clip.sampleRate;
        const bool gate = (static_cast<int>(time * 4.0) % 2) == 0;
        const float sample = gate ? static_cast<float>(0.18 * std::sin(2.0 * juce::MathConstants<double>::pi * 220.0 * time)) : 0.0f;
        clip.leftChannel[static_cast<size_t>(i)] = sample;
        clip.rightChannel[static_cast<size_t>(i)] = sample;
    }

    juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
    auto* asset = arrangementState.addAsset(clip.name, AssetKind::pianoRollPattern, clip);
    return asset != nullptr ? asset->assetId : -1;
}

int AudioEngine::createPatternAsset(const juce::String& name, double lengthSeconds, std::vector<PatternNote> patternNotes)
{
    RecordedClip clip;
    clip.name = name;
    clip.sampleRate = getTimelineSampleRate();

    const auto clampedLengthSeconds = juce::jmax(0.1, lengthSeconds);
    const int samples = static_cast<int>(std::round(clip.sampleRate * clampedLengthSeconds));
    clip.leftChannel.resize(static_cast<size_t>(samples), 0.0f);
    clip.rightChannel.resize(static_cast<size_t>(samples), 0.0f);

    juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
    auto* asset = arrangementState.addAsset(name, AssetKind::pianoRollPattern, clip);
    if (asset != nullptr)
        asset->patternNotes = std::move(patternNotes);
    return asset != nullptr ? asset->assetId : -1;
}

bool AudioEngine::updatePatternAsset(int assetId, const juce::String& name, double lengthSeconds, std::vector<PatternNote> patternNotes)
{
    RecordedClip clip;
    clip.name = name;
    clip.sampleRate = getTimelineSampleRate();

    const auto clampedLengthSeconds = juce::jmax(0.1, lengthSeconds);
    const int samples = static_cast<int>(std::round(clip.sampleRate * clampedLengthSeconds));
    clip.leftChannel.resize(static_cast<size_t>(samples), 0.0f);
    clip.rightChannel.resize(static_cast<size_t>(samples), 0.0f);

    juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
    const bool renamed = arrangementState.renameAsset(assetId, name);
    const bool updated = arrangementState.updateAssetClip(assetId, clip);
    if (auto* asset = arrangementState.findAsset(assetId))
        asset->patternNotes = std::move(patternNotes);

    return renamed && updated;
}

bool AudioEngine::renameAsset(int assetId, const juce::String& newName)
{
    juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
    return arrangementState.renameAsset(assetId, newName);
}

int AudioEngine::createLiveInputAsset(const juce::String& name)
{
    juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
    auto* asset = arrangementState.addAsset(name, AssetKind::liveInput, {});
    return asset != nullptr ? asset->assetId : -1;
}

void AudioEngine::setPatternTrackRenderer(ArrangementState::PatternTrackRenderer renderer)
{
    juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
    arrangementState.setPatternTrackRenderer(std::move(renderer));
}
