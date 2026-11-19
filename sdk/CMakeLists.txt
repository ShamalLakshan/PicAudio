cmake_minimum_required(VERSION 3.20)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(DEFAULT_VERSION "0.1.0")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../CMakeLists.txt")
        file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/../CMakeLists.txt" ROOT_VERSION_LINE REGEX "project\\(PicAudio VERSION [0-9.]+")
        if(ROOT_VERSION_LINE MATCHES "project\\(PicAudio VERSION ([0-9.]+)")
            set(DEFAULT_VERSION "${CMAKE_MATCH_1}")
        endif()
    endif()
    project(PicAudioSdk VERSION ${DEFAULT_VERSION} LANGUAGES CXX)
endif()

add_library(picaudio_sdk INTERFACE)

target_include_directories(picaudio_sdk INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_library(parakeet_sdk ALIAS picaudio_sdk)

install(TARGETS picaudio_sdk EXPORT PicAudioSdkConfig)
install(DIRECTORY include/ DESTINATION include)
install(DIRECTORY cmake/ DESTINATION share/picaudio/cmake)

# Package SDK target for developer distribution
find_package(Git QUIET)
set(PICAUDIO_GIT_COMMIT "unknown")
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --short HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE PICAUDIO_GIT_COMMIT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()

set(SDK_VERSION "${PROJECT_VERSION}")
set(SDK_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
if(NOT SDK_ARCH)
    set(SDK_ARCH "x64")
endif()
string(TOLOWER "${SDK_ARCH}" SDK_ARCH)
if(SDK_ARCH MATCHES "amd64|x86_64")
    set(SDK_ARCH "x64")
endif()

string(TOLOWER "${CMAKE_SYSTEM_NAME}" SDK_OS)
set(SDK_NAME "picaudio-sdk-${SDK_VERSION}-${SDK_OS}-${SDK_ARCH}")
set(SDK_OUTPUT_DIR "${CMAKE_BINARY_DIR}/sdk")
set(SDK_STAGE_DIR "${SDK_OUTPUT_DIR}/${SDK_NAME}")
file(MAKE_DIRECTORY "${SDK_OUTPUT_DIR}")

set(SDK_LICENSE_FILE "${CMAKE_SOURCE_DIR}/LICENSE")
if(NOT EXISTS "${SDK_LICENSE_FILE}" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE")
    set(SDK_LICENSE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE")
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/manifest.json.in" "${CMAKE_CURRENT_BINARY_DIR}/sdk-manifest.json" @ONLY)

add_custom_target(package_sdk
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SDK_OUTPUT_DIR}"
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${SDK_STAGE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SDK_STAGE_DIR}/include"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SDK_STAGE_DIR}/cmake"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SDK_STAGE_DIR}/templates"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/include" "${SDK_STAGE_DIR}/include"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/cmake" "${SDK_STAGE_DIR}/cmake"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/templates" "${SDK_STAGE_DIR}/templates"
    COMMAND ${CMAKE_COMMAND} -E copy "${SDK_LICENSE_FILE}" "${SDK_STAGE_DIR}/LICENSE"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/sdk-manifest.json" "${SDK_STAGE_DIR}/sdk-manifest.json"
    COMMAND ${CMAKE_COMMAND} -E tar cf "${SDK_OUTPUT_DIR}/${SDK_NAME}.zip" --format=zip "${SDK_NAME}"
    WORKING_DIRECTORY "${SDK_OUTPUT_DIR}"
    COMMENT "Packaging PicAudio SDK into ${SDK_OUTPUT_DIR}/${SDK_NAME}.zip"
    VERBATIM
)
