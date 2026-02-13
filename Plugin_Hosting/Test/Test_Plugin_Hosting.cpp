#include <juce_gui_basics/juce_gui_basics.h>

class Test_Plugin_Hosting : public juce::Component
{
public:
    Test_Plugin_Hosting()
    {
        // test stuff
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::darkgrey);
        g.setColour(juce::Colours::white);
        g.drawText("Plugin Hosting Test", getLocalBounds(), juce::Justification::centred);
    }
};