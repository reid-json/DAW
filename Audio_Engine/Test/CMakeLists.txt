add_executable(Audio_Engine_Test WIN32
    Main.cpp
)

target_link_libraries(Audio_Engine_Test PRIVATE
    Audio_Engine
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_utils
    juce::juce_gui_basics
    juce::juce_gui_extra
)
