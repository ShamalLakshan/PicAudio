find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(PicAudio
    main.cpp
    ${CMAKE_SOURCE_DIR}/deploy/icons.qrc
)

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
