add_library(Piano_Roll STATIC
    piano.cpp
)

target_include_directories(Piano_Roll PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})