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
#This probably needs to go into the top level cmakeLists file but I havent added it: target_link_libraries(DAW PRIVATE LowpassHighpassFilterPlugin)

target_include_directories(Plugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
