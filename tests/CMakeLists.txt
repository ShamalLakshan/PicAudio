find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick)

find_package(GTest QUIET)
if(GTest_FOUND)
    set(GTEST_LIBRARIES GTest::gtest GTest::gtest_main)
else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(GTEST QUIET gtest gtest_main)
    endif()
endif()

if(NOT GTEST_FOUND AND NOT GTest_FOUND)
    message(WARNING "GoogleTest not found; skipping unit tests.")
    return()
endif()

add_executable(picaudio_tests
    test_queue_service.cpp
    test_theme_loader.cpp
    test_settings.cpp
    test_metadata_art.cpp
    test_desktop_integration.cpp
)

target_include_directories(picaudio_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GTEST_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core/include
    ${CMAKE_SOURCE_DIR}/adapters
    ${CMAKE_SOURCE_DIR}/adapters/ui_qt
    ${CMAKE_SOURCE_DIR}/adapters/ui_qt/theming
)

target_link_libraries(picaudio_tests PRIVATE
    player_core
    ui_adapter
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    ${GTEST_LIBRARIES}
)

include(CTest)
add_test(NAME PicAudioUnitTests COMMAND picaudio_tests)
