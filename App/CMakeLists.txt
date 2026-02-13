#moved from root cmake to here to decouple and create a module for the executable
# This is creating the JUCE app and is the main entry point for the DAW.
juce_add_gui_app(DAW
    PRODUCT_NAME "DAW"
)

# This is targeting the files for the exe 
target_sources(DAW PRIVATE
    Main.cpp
    MainComponent.cpp
    MainComponent.h
    App.cpp
    App.h
)



# This is adding libraries or modules and we can add more later
# also since it is setup modular now, for any reason we add more sections then we need to connect the module here
# this is all the sections that'll end up being added to the exe, so if we add more then we need to add the module name here as well
target_link_libraries(DAW PRIVATE
    Utilities
    Audio_Engine
    Piano_Roll
    GUI
    Plugin
    Plugin_Hosting

    foleys_gui_magic

    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
)





