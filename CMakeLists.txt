cmake_minimum_required(VERSION 3.15)
project(DAW VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Fetching JUCE
include(FetchContent)


set(FETCHCONTENT_QUIET OFF)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.0
)
FetchContent_MakeAvailable(juce)


# Ensure stylesheet support is compiled in for anything that needs it (including JIVE sources)
add_compile_definitions(JIVE_GUI_ITEMS_HAVE_STYLE_SHEETS=1)

# Add subdirectories
add_subdirectory(external/JIVE)

add_subdirectory(Audio_Engine)

add_subdirectory(Piano_Roll)

add_subdirectory(Plugin)

add_subdirectory(Plugin_Hosting)

add_subdirectory(GUI)

# copying resources to the build directory
add_custom_target(CopyResources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/Resources
            $<TARGET_FILE_DIR:DAW>/Resources
    COMMENT "Synchronizing Resources for DAW"
)

add_subdirectory(App)
