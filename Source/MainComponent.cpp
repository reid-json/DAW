#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    setSize(600, 400);

    // Register audio formats
    formatManager.registerBasicFormats();

    // Start audio with no input, stereo output
    setAudioChannels(0, 2);

    // Button
    addAndMakeVisible(loadButton);
    loadButton.addListener(this);

    // Label
    addAndMakeVisible(fileNameLabel);
    fileNameLabel.setText("No file loaded", juce::dontSendNotification);
    fileNameLabel.setJustificationType(juce::Justification::centred);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

//==============================================================================
void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
}

void MainComponent::resized()
{
    loadButton.setBounds(getWidth()/2 - 100, getHeight()/2 - 30, 200, 60);
    fileNameLabel.setBounds(0, 20, getWidth(), 30);
}

//==============================================================================
void MainComponent::buttonClicked(juce::Button* button)
{
    if (button == &loadButton)
        loadFile();
}

void MainComponent::loadFile()
{
    auto chooser = std::make_shared<juce::FileChooser>(
        "Select an audio file...",
        juce::File{},
        "*.wav;*.mp3;*.aiff"
    );

    chooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (!file.existsAsFile())
                return;

            transportSource.stop();
            transportSource.setSource(nullptr);
            readerSource.reset();

            if (auto* reader = formatManager.createReaderFor(file))
            {
                readerSource.reset(new juce::AudioFormatReaderSource(reader, true));

                // Set transport source and start playback
                transportSource.setSource(readerSource.get(), 0, nullptr, reader->sampleRate);
                transportSource.start();

                // Update UI on main thread
                juce::MessageManager::callAsync([this, file]()
                {
                    fileNameLabel.setText("Playing: " + file.getFileName(), juce::dontSendNotification);
                });
            }
        });
}

//==============================================================================
void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    currentBlockSize = samplesPerBlockExpected;
    currentSampleRate = sampleRate;
    transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    transportSource.getNextAudioBlock(bufferToFill);
}

void MainComponent::releaseResources()
{
    transportSource.releaseResources();
}