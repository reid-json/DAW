add_library(Plugin STATIC
    plugin.cpp
)

#added for LowpassHighPassFilter
add_library(LowpassHighpassFilterPlugin
    Plugins/LowpassHighpassFilter/PluginProcessor.cpp
    Plugins/LowpassHighpassFilter/PluginEditor.cpp
    Plugins/LowpassHighpassFilter/LowpassHighpassFilter.cpp
)
target_include_directories(LowpassHighpassFilterPlugin PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/LowpassHighpassFilter
)
target_include_directories(GainPanFilterPlugin PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/GainPanFilter
)

#added for GainPanFilter (not working)
add_library(GainPanFilterPlugin
    Plugins/GainPanFilter/PluginProcessor.cpp
    Plugins/GainPanFilter/PluginEditor.cpp
    Plugins/GainPanFilter/GainPanFilter.cpp
)

target_link_libraries(Plugin PRIVATE
    LowpassHighpassFilterPlugin
    GainPanFilterPlugin
)

target_include_directories(Plugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
