#include <juce_gui_basics/juce_gui_basics.h>
// you'll eventually add the header files of the module here the same way we'll add them to the main app section
// then you can add tests cuase itll be pulling the code from the module
// you dont have to rewrite or remake code here
// just a small test enviornment that you can make a simple ui and test parts or your code without needing the whole project
class Test_Audio_Engine : public juce::Component
{
public:
    Test_Audio_Engine()
    {
        //test stuff here
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::darkgrey);
        g.setColour(juce::Colours::white);
        g.drawText("Audio Module Test", getLocalBounds(), juce::Justification::centred);
    }
};