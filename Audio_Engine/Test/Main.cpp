#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Root/AudioEngine.h"

class RecentAssetsModel : public juce::ListBoxModel
{
public:
    explicit RecentAssetsModel(AudioEngine& engineRef) : engine(engineRef) {}

    int getNumRows() override
    {
        return static_cast<int>(engine.getArrangementState().getRecentAssetIds().size());
    }

    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override
    {
        if (rowIsSelected)
            g.fillAll(juce::Colour(0xff2b4955));

        const auto& arrangement = engine.getArrangementState();
        const auto& recentIds = arrangement.getRecentAssetIds();
        if (rowNumber < 0 || rowNumber >= static_cast<int>(recentIds.size()))
            return;

        if (const auto* asset = arrangement.findAsset(recentIds[static_cast<size_t>(rowNumber)]))
        {
            g.setColour(juce::Colours::white);
            g.drawText(asset->name, 8, 4, width - 16, 22, juce::Justification::centredLeft, true);
            g.setColour(juce::Colours::white.withAlpha(0.6f));
            g.drawText(juce::String(asset->clip.getLengthSeconds(), 2) + " s", 8, 24, width - 16, 18, juce::Justification::centredLeft, false);
        }
    }

private:
    AudioEngine& engine;
};

class MainComponent : public juce::Component, private juce::Timer
{
public:
    MainComponent()
        : recentAssetsModel(engine)
    {
        engine.initialise(2, 2);

        recordButton.setButtonText("Start Recording");
        recordButton.onClick = [this]
        {
            if (engine.isRecording())
                engine.stopRecording();
            else
                engine.startRecording();

            refreshUi();
        };
        addAndMakeVisible(recordButton);

        uploadButton.setButtonText("Upload Audio");
        uploadButton.onClick = [this] { importAudio(); };
        addAndMakeVisible(uploadButton);

        statusLabel.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(statusLabel);

        recentLabel.setText("Recent Recordings / Uploaded Files", juce::dontSendNotification);
        addAndMakeVisible(recentLabel);

        recentList.setModel(&recentAssetsModel);
        addAndMakeVisible(recentList);

        setSize(520, 420);
        startTimerHz(8);
        refreshUi();
    }

    ~MainComponent() override
    {
        engine.shutdown();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff10181d));
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(16);
        auto buttons = area.removeFromTop(30);
        recordButton.setBounds(buttons.removeFromLeft(160));
        buttons.removeFromLeft(10);
        uploadButton.setBounds(buttons.removeFromLeft(140));
        area.removeFromTop(10);
        statusLabel.setBounds(area.removeFromTop(24));
        area.removeFromTop(12);
        recentLabel.setBounds(area.removeFromTop(20));
        area.removeFromTop(8);
        recentList.setBounds(area);
    }

private:
    void timerCallback() override
    {
        refreshUi();
    }

    void refreshUi()
    {
        recentList.updateContent();
        recentList.repaint();

        if (engine.isRecording())
        {
            recordButton.setButtonText("Stop Recording");
            statusLabel.setText("Recording: " + juce::String(engine.getCurrentRecordingLengthSeconds(), 2) + " s",
                                juce::dontSendNotification);
        }
        else
        {
            recordButton.setButtonText("Start Recording");
            statusLabel.setText("Idle", juce::dontSendNotification);
        }
    }

    void importAudio()
    {
        chooser = std::make_unique<juce::FileChooser>("Choose an audio file", juce::File(), "*.wav;*.mp3;*.aif;*.aiff;*.flac");
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fileChooser)
            {
                const auto file = fileChooser.getResult();
                if (file.existsAsFile())
                    engine.importAudioFileAsRecentAsset(file);

                chooser.reset();
                refreshUi();
            });
    }

    AudioEngine engine;
    RecentAssetsModel recentAssetsModel;
    juce::TextButton recordButton;
    juce::TextButton uploadButton;
    juce::Label statusLabel;
    juce::Label recentLabel;
    juce::ListBox recentList;
    std::unique_ptr<juce::FileChooser> chooser;
};

class AudioEngineTestApp : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "Audio Engine Test"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String&) override
    {
        mainWindow.reset(new MainWindow(getApplicationName()));
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

private:
    class MainWindow : public juce::DocumentWindow
    {
    public:
        explicit MainWindow(juce::String name)
            : DocumentWindow(name, juce::Colours::darkgrey, DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new MainComponent(), true);
            setResizable(true, true);
            centreWithSize(getWidth(), getHeight());
            setVisible(true);
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };

    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(AudioEngineTestApp)
