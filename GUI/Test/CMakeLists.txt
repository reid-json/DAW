juce_add_gui_app(GUITest
    PRODUCT_NAME "GUI Test"
)

target_sources(GUITest PRIVATE
    Main.cpp
    Test_GUI.cpp
)

target_link_libraries(GUITest PRIVATE
    juce::juce_gui_basics
    juce::juce_gui_extra
    GUI
)