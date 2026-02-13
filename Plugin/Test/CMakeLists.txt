juce_add_gui_app(PluginTest
    PRODUCT_NAME "Plugin Test"
)

target_sources(PluginTest PRIVATE
    Main.cpp
    Test_Plugin.cpp
)

target_link_libraries(PluginTest PRIVATE
    juce::juce_gui_basics
    juce::juce_gui_extra
    Plugin
)