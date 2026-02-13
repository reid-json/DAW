#include <juce_gui_basics/juce_gui_basics.h>

class Test_Piano_Roll : public juce::Component
{
public:
    Test_Piano_Roll()
    {
        // test stuff
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::darkgrey);
        g.setColour(juce::Colours::white);
        g.drawText("test for Piano Roll", getLocalBounds(), juce::Justification::centred);
    }
};