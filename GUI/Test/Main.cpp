#include <juce_gui_basics/juce_gui_basics.h>
#include "../guicomponent.h"

class GUI_TestApplication : public juce::JUCEApplication
{
public:
    GUI_TestApplication() {}

    const juce::String getApplicationName() override { return "GUI Test"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String& commandLine) override
    {
        mainWindow.reset(new MainWindow(getApplicationName()));
    }

    void shutdown() override { mainWindow.reset(); }

    void systemRequestedQuit() override { quit(); }

    void anotherInstanceStarted(const juce::String& commandLine) override {}

    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow(juce::String name) : DocumentWindow(name, 
            juce::Colours::black, 
            allButtons)
        {
            setUsingNativeTitleBar(true);
            setResizable(true, true);
            
            auto* contentComp = new GUIComponent();
            setContentOwned(contentComp, true);

    
            centreWithSize(1000, 700);

            setVisible(true);
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(GUI_TestApplication)
