#This is the actual module's cmake file, and will add more files here later

add_library(Audio_Engine STATIC
    Root/AudioEngine.cpp
    Root/AudioEngine.h
    IO/AudioInputHandler.cpp
    IO/AudioInputHandler.h
    IO/InputRingBuffer.cpp
    IO/InputRingBuffer.h
    Tracks/AudioTrack.cpp
    Tracks/AudioTrack.h
    DSP/DSPProcessor.h
    DSP/GainProcessor.cpp
    DSP/GainProcessor.h
    Graph/DSPChain.cpp
    Graph/DSPChain.h
)

target_link_libraries(Audio_Engine
    PRIVATE
        juce::juce_gui_basics
        juce::juce_audio_devices
        juce::juce_audio_basics
        
)
