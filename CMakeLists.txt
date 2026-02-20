cmake_minimum_required(VERSION 3.15)
project(DAW VERSION 1.0)

# This is downloading JUCE
include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.0
)
FetchContent_MakeAvailable(juce)


# This is creating the JUCE app
juce_add_gui_app(DAW
    PRODUCT_NAME "DAW"
)

# This is finding the code from our app
target_sources(DAW PRIVATE
    source/Main.cpp
    source/MainComponent.cpp
    source/MainComponent.h
)


# This is adding libraries or modules and we can add more later
target_link_libraries(DAW PRIVATE
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_audio_utils
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors

)

# https://cmake.org/cmake/help/latest/manual/cmake-commands.7.html used this documentation and videos to setup this