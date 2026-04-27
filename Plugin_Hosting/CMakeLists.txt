add_library(Plugin_Hosting STATIC
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

target_compile_definitions(Plugin_Hosting
    PUBLIC
        JUCE_PLUGINHOST_VST=$<BOOL:${DAW_ENABLE_VST2_HOSTING}>
        JUCE_PLUGINHOST_VST3=1
)
