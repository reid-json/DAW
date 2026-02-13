#include <juce_gui_basics/juce_gui_basics.h>

class Test_Plugin : public juce::Component
{
public:
    Test_Plugin()
    {
        // test stuff
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::darkgrey);
        g.setColour(juce::Colours::white);
        g.drawText("Plugin Test", getLocalBounds(), juce::Justification::centred);
    }
};