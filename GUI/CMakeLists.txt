add_library(GUI STATIC
    guicomponent.cpp
    guicomponent.h
)
# here can see that the target has foleys_gui_magic
# this is the only place that needs it so makes it not dependent on other stuff
target_link_libraries(GUI
    PRIVATE
        juce::juce_gui_basics
        foleys_gui_magic
)