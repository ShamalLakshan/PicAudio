cmake_minimum_required(VERSION 3.20)
project(PicAudioSamplePlugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find or include PicAudio SDK
# If built within the PicAudio tree, picaudio_sdk is already available.
if(NOT TARGET picaudio_sdk)
    find_package(PicAudioSdk REQUIRED)
endif()

add_library(sample_plugin SHARED
    sample_plugin.cpp
)

target_compile_definitions(sample_plugin PRIVATE PICAUDIO_PLUGIN_BUILD)
target_link_libraries(sample_plugin PRIVATE picaudio_sdk)

set_target_properties(sample_plugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "sample_plugin"
)
