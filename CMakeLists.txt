cmake_minimum_required(VERSION 3.15)
project(DAW VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Fetching JUCE
include(FetchContent)

# Make FetchContent less opaque + more reliable on unstable networks
set(FETCHCONTENT_QUIET OFF)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.0
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(juce)

# --- IMPORTANT ---
# Ensure stylesheet support is compiled in for anything that needs it (including JIVE sources)
add_compile_definitions(JIVE_GUI_ITEMS_HAVE_STYLE_SHEETS=1)

# Add subdirectories
add_subdirectory(external/JIVE)
add_subdirectory(Utilities)

add_subdirectory(Audio_Engine)
add_subdirectory(Audio_Engine/Test)

add_subdirectory(Piano_Roll)
add_subdirectory(Piano_Roll/Test)

add_subdirectory(Plugin)
add_subdirectory(Plugin/Test)

add_subdirectory(Plugin_Hosting)
add_subdirectory(Plugin_Hosting/Test)

add_subdirectory(GUI)

# copying resources to the build directory for both DAW and GUI_Test targets
add_custom_target(CopyResources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/Resources
            $<TARGET_FILE_DIR:DAW>/Resources
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/Resources
            $<TARGET_FILE_DIR:GUI_Test>/Resources
    COMMENT "Synchronizing Resources for DAW and GUI_Test"
)

add_subdirectory(App)