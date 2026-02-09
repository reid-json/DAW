#This is wat is creating the test exe for the module so we can have an enviornemnt for each one
juce_add_gui_app(Audio_Engine_Test
    PRODUCT_NAME "Audio Engine Test"
)

target_sources(Audio_Engine_Test PRIVATE
    Main.cpp
    Test_Audio_Engine.cpp
)

#adding Audio_Engine here is what allows the test section to use the code from the module
#without this it would not be able to see any of the code in the module
target_link_libraries(Audio_Engine_Test PRIVATE
    juce::juce_gui_basics
    juce::juce_gui_extra
    Audio_Engine
)