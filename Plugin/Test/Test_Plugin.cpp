#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

class FilterTestComponent : public juce::Component
{
public:
    FilterTestComponent()
    {
        processor = std::make_unique<LowpassHighpassFilterAudioProcessor>();
        processor->prepareToPlay(44100.0, 512);

        editor.reset(static_cast<LowpassHighpassFilterAudioProcessorEditor*>(processor->createEditor()));
        addAndMakeVisible(editor.get());

        setSize(editor->getWidth(), editor->getHeight());
    }

    ~FilterTestComponent() override
    {
        editor.reset();
        processor->releaseResources();
    }

    void resized() override
    {
        if (editor != nullptr)
            editor->setBounds(getLocalBounds());
    }

private:
    std::unique_ptr<LowpassHighpassFilterAudioProcessor> processor;
    std::unique_ptr<LowpassHighpassFilterAudioProcessorEditor> editor;
};