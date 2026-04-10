add_library(GUI STATIC
    guicomponent.cpp
    guicomponent.h
    dawstate.h
    timelinecomponent.cpp
    timelinecomponent.h
    arrangementcomponent.cpp
    arrangementcomponent.h
    tracklistcomponent.cpp
    tracklistcomponent.h
    recentclipscomponent.cpp
    recentclipscomponent.h
    tempocontrolcomponent.cpp
    tempocontrolcomponent.h
    settingswindow.h
)

target_link_libraries(GUI
    PUBLIC
        juce::juce_gui_basics
        juce::juce_audio_devices
        juce::juce_audio_utils
        Plugin_Hosting
        jive::jive_core
        jive::jive_components
        jive::jive_layouts
        jive::jive_style_sheets
        Piano_Roll
)

target_include_directories(GUI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(GUI PUBLIC
    JIVE_GUI_ITEMS_HAVE_STYLE_SHEETS=1
)

