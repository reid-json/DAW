add_library(Piano_Roll STATIC
    pianoRoll.cpp
    pianoRoll.h
)

target_link_libraries(Piano_Roll
    PUBLIC
        juce::juce_gui_basics
)

target_include_directories(Piano_Roll PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})