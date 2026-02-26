
juce_add_gui_app(AudioEngineTest
    PRODUCT_NAME "AudioEngine Test"
)

target_sources(AudioEngineTest PRIVATE
    Main.cpp
    Test_Audio_Engine.cpp
    Test_Audio_Engine.h
)

target_link_libraries(AudioEngineTest
    PRIVATE
        Audio_Engine
        juce::juce_gui_basics
        juce::juce_audio_devices
        juce::juce_audio_basics
)