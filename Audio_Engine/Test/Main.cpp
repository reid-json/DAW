/*boiler plate code, very similar to the default JUCE code, just setup to see the structure working*/
#include <juce_gui_basics/juce_gui_basics.h>
#include "Test_Audio_Engine.cpp"  

class ModuleTestApplication : public juce::JUCEApplication
{
public:
    ModuleTestApplication() {}

    const juce::String getApplicationName() override       { return "Audio Engine Test"; }
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

            
            setContentOwned(new Test_Audio_Engine(), true);

            centreWithSize(800, 600);
            setResizable(true, true);
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