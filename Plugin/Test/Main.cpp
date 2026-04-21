#include <juce_gui_basics/juce_gui_basics.h>
#include "Test_Plugin.h"

class ModuleTestApplication : public juce::JUCEApplication
{
public:
    ModuleTestApplication() {}

    const juce::String getApplicationName() override       { return "Plugin Test"; }
    const juce::String getApplicationVersion() override    { return "1.0"; }
    bool moreThanOneInstanceAllowed() override             { return true; }

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
        MainWindow(const juce::String& name)
            : DocumentWindow(name,
                             juce::Colours::darkgrey,
                             DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new FilterTestComponent(), true);
            centreWithSize(getContentComponent()->getWidth(),
                           getContentComponent()->getHeight());
            setResizable(false, false);
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

START_JUCE_APPLICATION(ModuleTestApplication)
