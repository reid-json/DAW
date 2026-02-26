#include <juce_gui_basics/juce_gui_basics.h>
#include "Test_Audio_Engine.h"

class TestWindow : public juce::DocumentWindow
{
public:
    TestWindow()
        : DocumentWindow("Audio Engine Test",
                         juce::Colours::black,
                         DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        setResizable(true, true);
        setContentOwned(new Test_Audio_Engine(), true);
        centreWithSize(600, 400);
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplicationBase::quit();
    }
};

class TestApp : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "AudioEngine Test"; }
    const juce::String getApplicationVersion() override { return "1.0"; }

    void initialise(const juce::String&) override
    {
        window.reset(new TestWindow());
    }

    void shutdown() override
    {
        window = nullptr;
    }

private:
    std::unique_ptr<TestWindow> window;
};

START_JUCE_APPLICATION(TestApp)