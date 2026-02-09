juce_add_gui_app(PianoRollTest
    PRODUCT_NAME "Piano Roll Test"
)

target_sources(PianoRollTest PRIVATE
    Main.cpp
    Test_Piano_Roll.cpp
)

target_link_libraries(PianoRollTest PRIVATE
    juce::juce_gui_basics
    juce::juce_gui_extra
    Piano_Roll
)