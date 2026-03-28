add_library(Utilities STATIC
    util.cpp
)

target_include_directories(Utilities PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})