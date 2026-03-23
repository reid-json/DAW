#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include "../IO/DeviceManager.h"
#include "../Tracks/TrackInputRouter.h"
#include <memory>

#include <vector>

class AudioEngine : public juce::MidiInputCallback
{
public:
    AudioEngine();
    ~AudioEngine();

    void initialise(int inputChannels, int outputChannels);
    void shutdown();

    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;

    juce::MidiMessageCollector& getMidiCollector() { return ioDeviceManager.getMidiCollector(); }
    
    // Track Management
    TrackInputRouter* addTrack() 
    {
        juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
        tracks.push_back(std::make_unique<TrackInputRouter>());
        
        if (auto* device = deviceManager.getCurrentAudioDevice())
        {
            tracks.back()->prepare(device->getCurrentSampleRate(), device->getCurrentBufferSizeSamples());
        }
        tracks.back()->setGain(masterVolumeDb);
        
        ioDeviceManager.setTracks(&tracks);
        return tracks.back().get();
    }
    
    void removeTrack(TrackInputRouter* trackToRemove)
    {
        juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
        auto it = std::remove_if(tracks.begin(), tracks.end(), 
            [trackToRemove](const std::unique_ptr<TrackInputRouter>& t) { return t.get() == trackToRemove; });
        if (it != tracks.end())
        {
            tracks.erase(it, tracks.end());
            ioDeviceManager.setTracks(&tracks);
        }
    }
    
    size_t getNumTracks() const { return tracks.size(); }
    TrackInputRouter* getTrack(size_t index) 
    { 
        if (index < tracks.size()) return tracks[index].get(); 
        return nullptr; 
    }

    juce::AudioDeviceManager& getDeviceManager() { return deviceManager; }

    void sendDiscoveryHandshake(juce::MidiOutput* output);

    void setMasterVolume(float volumeDb)
    {
        juce::ScopedLock sl(deviceManager.getAudioCallbackLock());
        masterVolumeDb = volumeDb;
        for (auto& track : tracks)
            track->setGain(masterVolumeDb);
    }

private:
    float masterVolumeDb = -6.0f;
    juce::AudioDeviceManager deviceManager;
    DeviceManager ioDeviceManager;
    std::vector<std::unique_ptr<TrackInputRouter>> tracks;
    
    std::vector<std::unique_ptr<juce::MidiInput>> activeMidiInputs;
    std::vector<std::unique_ptr<juce::MidiOutput>> activeMidiOutputs;
    juce::uint32 startTime = 0;
};
