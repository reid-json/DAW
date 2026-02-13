#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <foleys_gui_magic/foleys_gui_magic.h>

class GuiComponent : public juce::Component
{
public:
    GuiComponent();
    ~GuiComponent() override = default;

    void resized() override;

private:
    std::unique_ptr<foleys::MagicGUIState> magicState;
    std::unique_ptr<foleys::MagicGUIBuilder> magicBuilder;

    juce::Label testLabel;
};