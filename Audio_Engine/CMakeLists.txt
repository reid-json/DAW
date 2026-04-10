add_library(Audio_Engine STATIC
    Arrangement/ArrangementState.cpp
    Root/AudioEngine.cpp
    IO/DeviceManager.cpp
    IO/InputNode.cpp
    IO/OutputNode.cpp
    Recording/RecordingManager.cpp
)

target_include_directories(Audio_Engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Arrangement
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/IO
    ${CMAKE_CURRENT_SOURCE_DIR}/Recording
    ${CMAKE_CURRENT_SOURCE_DIR}/Root
    ${CMAKE_CURRENT_SOURCE_DIR}/Tracks
)

target_link_libraries(Audio_Engine PUBLIC
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
)

