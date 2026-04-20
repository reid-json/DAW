add_library(Plugin STATIC
    LowHighFilter/LowpassHighpassFilter.cpp
    LowHighFilter/LowpassHighpassFilter.h
    LowHighFilter/PluginProcessor.cpp
    LowHighFilter/PluginProcessor.h
    LowHighFilter/PluginEditor.cpp
    LowHighFilter/PluginEditor.h

    Compressor/Compressor.cpp
    Compressor/Compressor.h
    Compressor/PluginProcessor.cpp
    Compressor/PluginProcessor.h
    Compressor/PluginEditor.cpp
    Compressor/PluginEditor.h

    SevenBandEQ/SevenBandEQ.cpp
    SevenBandEQ/SevenBandEQ.h
    SevenBandEQ/PluginProcessor.cpp
    SevenBandEQ/PluginProcessor.h
    SevenBandEQ/PluginEditor.cpp
    SevenBandEQ/PluginEditor.h

    GainPanFilter/GainPanFilter.cpp
    GainPanFilter/GainPanFilter.h
    GainPanFilter/PluginProcessor.cpp
    GainPanFilter/PluginProcessor.h
    GainPanFilter/PluginEditor.cpp
    GainPanFilter/PluginEditor.h
)

target_include_directories(Plugin PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Plugin
    PUBLIC
        juce::juce_audio_processors
        juce::juce_audio_basics
        juce::juce_gui_basics
    PRIVATE
        juce::juce_gui_extra
        juce::juce_dsp
)

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
