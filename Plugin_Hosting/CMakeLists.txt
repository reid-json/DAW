add_library(Plugin_Hosting STATIC
    hosting.cpp
)

target_include_directories(Plugin_Hosting PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})