#include "AudioEngine.h"

AudioEngine::AudioEngine()
{
}

AudioEngine::~AudioEngine()
{
    shutdown();
}

void AudioEngine::initialise(int inputChannels, int outputChannels)
{
    
    juce::String error = deviceManager.initialise(inputChannels, outputChannels, nullptr, true, "ASIO", nullptr);
    if (error.isNotEmpty())
    {
        
        deviceManager.initialiseWithDefaultDevices(inputChannels, outputChannels);
    }
    ioDeviceManager.initialise(deviceManager, inputChannels, outputChannels);
    
    std::cout << "[AUDIO] Device: " << deviceManager.getCurrentAudioDevice()->getName().toStdString() << std::endl;

    
    startTime = juce::Time::getMillisecondCounter();

    // midi input exposing
    activeMidiInputs.clear();
    const auto midiInputs = juce::MidiInput::getAvailableDevices();
    std::cout << "[MIDI] Found " << midiInputs.size() << " inputs." << std::endl;
    
    for (const auto& devInfo : midiInputs)
    {
        std::cout << "  -> Opening Input: " << devInfo.name.toStdString() << std::endl;
        if (auto input = juce::MidiInput::openDevice(devInfo.identifier, this))
        {
            input->start();
            activeMidiInputs.push_back(std::move(input));
        }
    }

    // midi output discovery
    activeMidiOutputs.clear();
    const auto midiOutputs = juce::MidiOutput::getAvailableDevices();
    std::cout << "[MIDI] Found " << midiOutputs.size() << " outputs." << std::endl;

    for (const auto& devInfo : midiOutputs)
    {
        std::cout << "  -> Opening Output: " << devInfo.name.toStdString() << std::endl;
        if (auto output = juce::MidiOutput::openDevice(devInfo.identifier))
        {
            sendDiscoveryHandshake(output.get());
            activeMidiOutputs.push_back(std::move(output));
        }
    }
}

void AudioEngine::shutdown()
{
    for (auto& input : activeMidiInputs)
        input->stop();
    
    activeMidiInputs.clear();
    activeMidiOutputs.clear();
    
    ioDeviceManager.shutdown();
    deviceManager.closeAudioDevice();
}

void AudioEngine::sendDiscoveryHandshake(juce::MidiOutput* output)
{
    if (output == nullptr) return;

    juce::String name = output->getName();
    
    if (name.containsIgnoreCase("Launchkey") || name.containsIgnoreCase("Novation"))
    {
        std::cout << "[HANDSHAKE] Sending DAW-Mode ON to " << name.toStdString() << std::endl;
        juce::MidiMessage handshake = juce::MidiMessage::noteOn(16, 12, (juce::uint8)127);
        output->sendMessageNow(handshake);
    }
    else
    {
        std::cout << "[HANDSHAKE] Skipping generic device: " << name.toStdString() << std::endl;
    }
}

void AudioEngine::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message)
{
    if (message.getChannel() == 16)
        return;

    if (!message.isNoteOn() && !message.isNoteOff() && !message.isController() && !message.isPitchWheel())
        return;

    bool isStillStarting = (juce::Time::getMillisecondCounter() - startTime < 1000);

    auto msg = message;
    
    if (!isStillStarting)
    {
        const juce::String sourceName = (source != nullptr) ? source->getName() : "INTERNAL";
        std::cout << "[MIDI EVENT] " << sourceName.toStdString() << ": " << msg.getDescription().toStdString() << std::endl;
    }

    msg.setTimeStamp(0);
    ioDeviceManager.getMidiCollector().addMessageToQueue(msg);
}
