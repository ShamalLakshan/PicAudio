cmake_minimum_required(VERSION 3.20)
project(PicAudio VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include(CTest)
option(BUILD_TESTING "Build unit tests" ON)

add_subdirectory(core)
add_subdirectory(adapters)
add_subdirectory(app)
add_subdirectory(sdk)
add_subdirectory(plugins)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

# Packaging
set(CPACK_PACKAGE_NAME "PicAudio")
set(CPACK_PACKAGE_VENDOR "PicAudio")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Lightweight high-fidelity music player")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
include(CPack)