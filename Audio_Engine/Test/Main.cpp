#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Root/AudioEngine.h"


class MainComponent;

class TrackComponent : public juce::Component
{
public:
    TrackComponent(TrackInputRouter* router, int idx, MainComponent& parent) 
        : trackRouter(router), mainComp(parent)
    {
        trackName.setText("Track " + juce::String(idx + 1), juce::dontSendNotification);
        trackName.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(trackName);
        
        enableAudioBtn.setButtonText("Audio In (Mic/Guitar)");
        enableAudioBtn.setToggleState(true, juce::dontSendNotification);
        enableAudioBtn.onClick = [this] { trackRouter->setAudioInputEnabled(enableAudioBtn.getToggleState()); };
        addAndMakeVisible(enableAudioBtn);
        
        enableMidiBtn.setButtonText("MIDI Synth");
        enableMidiBtn.setToggleState(true, juce::dontSendNotification);
        enableMidiBtn.onClick = [this] { trackRouter->setMidiEnabled(enableMidiBtn.getToggleState()); };
        addAndMakeVisible(enableMidiBtn);
        
        removeBtn.setButtonText("Remove Track");
        removeBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::darkred);
        removeBtn.onClick = [this] { requestRemoval(); };
        addAndMakeVisible(removeBtn);
    }
    
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black.withAlpha(0.2f));
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.drawRect(getLocalBounds(), 1);
    }
    
    void resized() override
    {
        auto area = getLocalBounds().reduced(10);
        trackName.setBounds(area.removeFromTop(30));
        area.removeFromTop(10);
        enableAudioBtn.setBounds(area.removeFromTop(30));
        area.removeFromTop(10);
        enableMidiBtn.setBounds(area.removeFromTop(30));
        
        area.removeFromTop(15);
        removeBtn.setBounds(area.removeFromTop(30));
    }
    
    TrackInputRouter* getRouter() const { return trackRouter; }
    
private:
    TrackInputRouter* trackRouter;
    MainComponent& mainComp;
    juce::Label trackName;
    juce::ToggleButton enableAudioBtn;
    juce::ToggleButton enableMidiBtn;
    juce::TextButton removeBtn;
    
    void requestRemoval();
};

class MainComponent : public juce::Component
{
public:
    MainComponent()
    {
        // Initializing with default system devices
        // need to have a settings page to let users decide which devices to use but rn itll recognize the default ones 
        engine.initialise(2, 2);

        addAndMakeVisible(addTrackButton);
        addTrackButton.setButtonText("Add Track");
        addTrackButton.onClick = [this] { addTrack(); };

        setSize(600, 300);
        
        // so it doesn't start with 0 tracks
        addTrack();
    }

    ~MainComponent() override
    {
        engine.shutdown();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff323e44)); 
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(10);
        
        addTrackButton.setBounds(area.removeFromTop(30));
        area.removeFromTop(10);
        
        if (!trackComps.isEmpty())
        {
            int w = area.getWidth() / trackComps.size();
            for (auto* tc : trackComps)
                tc->setBounds(area.removeFromLeft(w).reduced(5));
        }
    }
    
    void removeTrackUI(TrackComponent* comp)
    {
        engine.removeTrack(comp->getRouter());
        trackComps.removeObject(comp);
        
        
        for (int i = 0; i < trackComps.size(); ++i)
        {
            
        }
        
        resized();
        repaint();
    }

private:
    AudioEngine engine;
    juce::TextButton addTrackButton;
    juce::OwnedArray<TrackComponent> trackComps;

    void addTrack()
    {
        if (trackComps.size() >= 4) return; // up to 4 tracks 
        auto* newTrack = engine.addTrack();
        auto* trackComp = new TrackComponent(newTrack, trackComps.size(), *this);
        trackComps.add(trackComp);
        addAndMakeVisible(trackComp);
        resized();
    }
};

void TrackComponent::requestRemoval()
{
    mainComp.removeTrackUI(this);
}

class AudioEngineTestApp : public juce::JUCEApplication
{
public:
    AudioEngineTestApp() {}
    const juce::String getApplicationName() override { return "Audio_Engine Simple UI Test"; }
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

    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow(juce::String name) : DocumentWindow(name, juce::Colours::darkgrey, DocumentWindow::allButtons)
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

private:
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(AudioEngineTestApp)
