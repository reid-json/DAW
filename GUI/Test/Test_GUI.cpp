#include <juce_gui_basics/juce_gui_basics.h>

class Test_GUI : public juce::Component
{
public:
    Test_GUI()
    {
        // test stuff 
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::darkgrey);
        g.setColour(juce::Colours::white);
        g.drawText("GUI Module Test", getLocalBounds(), juce::Justification::centred);
    }
};