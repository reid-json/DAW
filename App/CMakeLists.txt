juce_add_gui_app(DAW
    PRODUCT_NAME "DAW"
)



target_sources(DAW PRIVATE
    Main.cpp
    MainComponent.cpp
    MainComponent.h
    App.cpp
    App.h
)

target_link_libraries(DAW PRIVATE
    GUI
    Audio_Engine
    Piano_Roll
    Plugin_Hosting

    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors

    jive::jive_core
    jive::jive_components
    jive::jive_layouts
    jive::jive_style_sheets
)


add_dependencies(DAW CopyResources)
