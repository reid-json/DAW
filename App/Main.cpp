#include <juce_gui_basics/juce_gui_basics.h>
#include "MainComponent.h"

class DAWApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override       { return "DAW"; }
    const juce::String getApplicationVersion() override    { return "1.0"; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    void initialise(const juce::String&) override
    {
        mainWindow.reset(new MainWindow("DAW"));
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow(juce::String name)
            : DocumentWindow(name,
                             juce::Colours::black,
                             DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setResizable(true, true);
            setResizeLimits(800, 600, 4096, 4096);

 
            centreWithSize(1000, 700);

            setContentOwned(new MainComponent(), true);

            setVisible(true);
        }

        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(DAWApplication)