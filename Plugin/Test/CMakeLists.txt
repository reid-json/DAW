juce_add_gui_app(PluginTest
    PRODUCT_NAME "Plugin Test"
)

target_sources(PluginTest PRIVATE
    Main.cpp
    Test_Plugin.cpp
)

target_link_libraries(PluginTest PRIVATE
    Plugin
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_audio_processors
    juce::juce_audio_basics
    juce::juce_audio_devices
)