juce_add_gui_app(GUI_Test
    PRODUCT_NAME "GUI Test"
)

target_link_libraries(GUI_Test
    PRIVATE
        GUI
        juce::juce_gui_basics
)

target_sources(GUI_Test
    PRIVATE
        Main.cpp
)

add_dependencies(GUI_Test CopyResources)
