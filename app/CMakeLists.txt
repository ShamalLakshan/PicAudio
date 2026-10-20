find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(PICAUDIO_SOURCES
    main.cpp
    ${CMAKE_SOURCE_DIR}/deploy/icons.qrc
)

if(WIN32)
    list(APPEND PICAUDIO_SOURCES ${CMAKE_SOURCE_DIR}/deploy/windows/picaudio.rc)
endif()

add_executable(PicAudio ${PICAUDIO_SOURCES})

target_link_libraries(PicAudio PRIVATE
    player_core
    ui_adapter
    Qt6::Widgets
    ${SQLITE_LIBRARIES}
    ${TAGLIB_LIBRARIES}
)

install(TARGETS PicAudio
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)
