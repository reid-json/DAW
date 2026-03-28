add_library(Plugin_Hosting STATIC
    hosting.cpp
    pluginhostmanager.cpp
    pluginhostmanager.h
)

target_include_directories(Plugin_Hosting PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(Plugin_Hosting
    PUBLIC
        Plugin
        juce::juce_audio_processors
        juce::juce_gui_extra
)
