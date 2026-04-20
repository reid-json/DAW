add_library(Plugin STATIC
<<<<<<< HEAD
    LowpassHighpassFilter.cpp
    LowpassHighpassFilter.h
    PluginProcessor.cpp
    PluginProcessor.h
    PluginEditor.cpp
    PluginEditor.h
)

target_include_directories(Plugin PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
=======
    plugin.cpp
)

# LowpassHighpassFilter
add_library(LowpassHighpassFilterPlugin
    Plugins/LowpassHighpassFilter/PluginProcessor.cpp
    Plugins/LowpassHighpassFilter/PluginEditor.cpp
    Plugins/LowpassHighpassFilter/LowpassHighpassFilter.cpp
)

target_include_directories(LowpassHighpassFilterPlugin PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/LowpassHighpassFilter
>>>>>>> 075fdc811b6aa9e1c2cfa300111279a0c69bafad
)

# GainPanFilter
add_library(GainPanFilterPlugin
    Plugins/GainPanFilter/PluginProcessor.cpp
    Plugins/GainPanFilter/PluginEditor.cpp
    Plugins/GainPanFilter/GainPanFilter.cpp
)

target_include_directories(GainPanFilterPlugin PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/GainPanFilter
)

# Link everything together
target_link_libraries(Plugin
    PUBLIC
        LowpassHighpassFilterPlugin
        GainPanFilterPlugin
        juce::juce_audio_processors
        juce::juce_audio_basics
        juce::juce_gui_basics
    PRIVATE
        juce::juce_gui_extra
        juce::juce_dsp
)

# Include root
target_include_directories(Plugin PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# JUCE plugin settings (KEEP THESE)
target_compile_definitions(Plugin PUBLIC
    JUCE_STANDALONE_APPLICATION=1
    JucePlugin_Name="LowpassHighpassFilter"
    JucePlugin_Desc="LowpassHighpassFilter"
    JucePlugin_Manufacturer="DAW"
    JucePlugin_ManufacturerCode=0x4d616e75
    JucePlugin_PluginCode=0x4d616e75
    JucePlugin_IsSynth=0
    JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0
    JucePlugin_IsMidiEffect=0
    JucePlugin_EditorRequiresKeyboardFocus=0
    JucePlugin_Version=1.0.0
    JucePlugin_VersionCode=0x10000
    JucePlugin_VersionString="1.0.0"
    "JucePlugin_PreferredChannelConfigurations={(1,1),(2,2)}"
)

