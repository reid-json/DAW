#include "Test_Plugin.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginEditor.h"
#include "PluginProcessor.h"

class FilterTestComponent::Impl
{
public:
    std::unique_ptr<LowpassHighpassFilterAudioProcessor> processor;
    std::unique_ptr<LowpassHighpassFilterAudioProcessorEditor> editor;
};

FilterTestComponent::FilterTestComponent()
    : impl(std::make_unique<Impl>())
{
    impl->processor = std::make_unique<LowpassHighpassFilterAudioProcessor>();
    impl->processor->prepareToPlay(44100.0, 512);

    impl->editor.reset(static_cast<LowpassHighpassFilterAudioProcessorEditor*>(impl->processor->createEditor()));
    addAndMakeVisible(impl->editor.get());

    setSize(impl->editor->getWidth(), impl->editor->getHeight());
}

FilterTestComponent::~FilterTestComponent()
{
    impl->editor.reset();
    impl->processor->releaseResources();
}

void FilterTestComponent::resized()
{
    if (impl->editor != nullptr)
        impl->editor->setBounds(getLocalBounds());
}
