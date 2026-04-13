juce_add_gui_app(PluginHostingTest
    PRODUCT_NAME "Plugin Hosting Test"
)

target_sources(PluginHostingTest PRIVATE
    Main.cpp
    Test_Plugin_Hosting.cpp
)

target_link_libraries(PluginHostingTest PRIVATE
    juce::juce_gui_basics
    juce::juce_gui_extra
    Plugin_Hosting
)