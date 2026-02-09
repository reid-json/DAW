add_library(Plugin STATIC
    plugin.cpp
)

target_include_directories(Plugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})