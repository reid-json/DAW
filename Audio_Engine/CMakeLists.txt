#This is the actual module's cmake file, and will add more files here later

add_library(Audio_Engine STATIC
    engine.cpp
)

target_include_directories(Audio_Engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})