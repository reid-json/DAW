cmake_minimum_required(VERSION 3.15)
project(DAW VERSION 1.0)



# Fetching JUCE for the project
include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.0
)
FetchContent_MakeAvailable(juce)

# Fetching gui extension for the project
FetchContent_Declare(
    foleys_gui_magic
    GIT_REPOSITORY https://github.com/ffAudio/foleys_gui_magic.git
    GIT_TAG        main

)
#looked up how to turn off building all the examples, kinda speeding up the build process
#still figuring out all the stuff regarding the build process
set(FOLEYS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(foleys_gui_magic)



# Adding modules here
# within each module is where we can add libraries specific for each
# This is where you have to add an "entry point" if you will so the root cmake will know to build it
add_subdirectory(Utilities)

add_subdirectory(Audio_Engine)
add_subdirectory(Audio_Engine/Test)

add_subdirectory(Piano_Roll)
add_subdirectory(Piano_Roll/Test)

add_subdirectory(GUI)
add_subdirectory(GUI/Test)

add_subdirectory(Plugin)
add_subdirectory(Plugin/Test)

add_subdirectory(Plugin_Hosting)
add_subdirectory(Plugin_Hosting/Test)

add_subdirectory(App)