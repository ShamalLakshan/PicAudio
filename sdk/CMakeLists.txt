cmake_minimum_required(VERSION 3.20)

add_library(picaudio_sdk INTERFACE)

target_include_directories(picaudio_sdk INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_library(parakeet_sdk ALIAS picaudio_sdk)

install(TARGETS picaudio_sdk EXPORT PicAudioSdkConfig)
install(DIRECTORY include/ DESTINATION include)
