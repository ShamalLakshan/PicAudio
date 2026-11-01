find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Multimedia Widgets)
find_package(Qt6 OPTIONAL_COMPONENTS DBus)

# SQLite3 discovery
find_package(SQLite3 QUIET)
if(SQLite3_FOUND)
    if(TARGET SQLite3::SQLite3)
        set(SQLITE_LIBRARIES SQLite3::SQLite3)
    else()
        set(SQLITE_LIBRARIES SQLite::SQLite3)
    endif()
    set(SQLITE_INCLUDE_DIRS ${SQLite3_INCLUDE_DIRS})
else()
    find_package(unofficial-sqlite3 CONFIG QUIET)
    if(unofficial-sqlite3_FOUND)
        set(SQLITE_LIBRARIES unofficial-sqlite3::sqlite3)
    else()
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(SQLITE REQUIRED sqlite3)
        else()
            message(FATAL_ERROR "SQLite3 could not be found.")
        endif()
    endif()
endif()

# TagLib discovery
find_package(TagLib CONFIG QUIET)
if(TagLib_FOUND OR TAGLIB_FOUND)
    if(TARGET TagLib::tag)
        set(TAGLIB_LIBRARIES TagLib::tag)
    elseif(TARGET TagLib::TagLib)
        set(TAGLIB_LIBRARIES TagLib::TagLib)
    endif()
    set(TAGLIB_INCLUDE_DIRS ${TagLib_INCLUDE_DIRS})
else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(TAGLIB QUIET taglib)
    endif()
    if(NOT TAGLIB_FOUND)
        find_path(TAGLIB_INCLUDE_DIRS NAMES taglib/tag.h)
        find_library(TAGLIB_LIBRARIES NAMES tag taglib)
        if(TAGLIB_INCLUDE_DIRS AND TAGLIB_LIBRARIES)
            set(TAGLIB_FOUND TRUE)
        else()
            message(FATAL_ERROR "TagLib could not be found.")
        endif()
    endif()
endif()

if(COMMAND qt_policy)
    if(QT_KNOWN_POLICY_QTP0001)
        qt_policy(SET QTP0001 NEW)
    endif()
    if(QT_KNOWN_POLICY_QTP0004)
        qt_policy(SET QTP0004 NEW)
    endif()
endif()

if(Qt6DBus_FOUND)
    add_compile_definitions(PICAUDIO_ENABLE_DBUS)
endif()

# UI, Theming, Data, Audio & Desktop Adapters
qt_add_library(ui_adapter STATIC
    ui_qt/QtBridge.hpp
    ui_qt/QtBridge.cpp
    ui_qt/theming/ThemeLoader.hpp
    ui_qt/theming/ThemeLoader.cpp
    ui_qt/models/AlbumListModel.hpp
    ui_qt/models/AlbumListModel.cpp
    ui_qt/models/TrackListModel.hpp
    ui_qt/models/TrackListModel.cpp
    ui_qt/providers/AlbumArtImageProvider.hpp
    ui_qt/providers/AlbumArtImageProvider.cpp
    metadata/TagLibMetadataAdapter.hpp
    metadata/TagLibMetadataAdapter.cpp
    storage/SqliteDatabaseAdapter.hpp
    storage/SqliteDatabaseAdapter.cpp
    audio/BitPerfectAudioAdapter.hpp
    audio/BitPerfectAudioAdapter.cpp
    desktop/PlatformMediaControls.hpp
    desktop/MprisAdapter.hpp
    desktop/MprisAdapter.cpp
    desktop/WindowsMediaControls.hpp
    desktop/WindowsMediaControls.cpp
    desktop/MacMediaControls.hpp
    desktop/MacMediaControls.cpp
    desktop/DesktopTrayAdapter.hpp
    desktop/DesktopTrayAdapter.cpp
    desktop/DesktopNotificationAdapter.hpp
    desktop/DesktopNotificationAdapter.cpp
    desktop/SingleInstanceManager.hpp
    desktop/SingleInstanceManager.cpp
    desktop/GlobalMediaKeysHook.hpp
    desktop/GlobalMediaKeysHook.cpp
    desktop/DesktopIntegrationManager.hpp
    desktop/DesktopIntegrationManager.cpp
    plugin/PluginManager.hpp
    plugin/PluginManager.cpp
    ${CMAKE_SOURCE_DIR}/themes/themes.qrc
    ${CMAKE_SOURCE_DIR}/deploy/icons.qrc
    ${CMAKE_SOURCE_DIR}/i18n/i18n.qrc
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/themes/
    DESTINATION share/picaudio/themes
    FILES_MATCHING PATTERN "*.json"
)

if(UNIX AND NOT APPLE)
    install(FILES ${CMAKE_SOURCE_DIR}/deploy/linux/picaudio.desktop
        DESTINATION share/applications
    )
    install(FILES ${CMAKE_SOURCE_DIR}/deploy/linux/icons/picaudio.svg
        DESTINATION share/icons/hicolor/scalable/apps
    )
endif()

set(QT_QML_NO_CACHEGEN ON)

qt_add_qml_module(ui_adapter
    URI PlayerUI
    VERSION 1.0
    RESOURCE_PREFIX "/"
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/PlayerUI
    NO_CACHEGEN
    NO_LINT
    QML_FILES
        ui_qt/qml/Main.qml
        ui_qt/qml/VectorIcon.qml
        ui_qt/qml/components/Badge.qml
        ui_qt/qml/components/ContextMenu.qml
        ui_qt/qml/components/EqualizerOverlay.qml
        ui_qt/qml/components/MovableHeaderArea.qml
        ui_qt/qml/components/ResizeGrip.qml
        ui_qt/qml/components/WaveVisualizer.qml
        ui_qt/qml/components/ToastBanner.qml
        ui_qt/qml/dialogs/PreferencesDialog.qml
        ui_qt/qml/dialogs/TagEditorDialog.qml
        ui_qt/qml/dialogs/OpenUrlDialog.qml
        ui_qt/qml/dialogs/ShortcutsDialog.qml
        ui_qt/qml/dialogs/DiagnosticsDialog.qml
        ui_qt/qml/dialogs/AboutDialog.qml
        ui_qt/qml/dialogs/SleepTimerDialog.qml
        ui_qt/qml/dialogs/NewPlaylistDialog.qml
        ui_qt/qml/dialogs/RenamePlaylistDialog.qml
        ui_qt/qml/dialogs/DeleteConfirmDialog.qml
        ui_qt/qml/views/QueueView.qml
        ui_qt/qml/views/NowPlayingView.qml
        ui_qt/qml/views/HistoryView.qml
        ui_qt/qml/views/AlbumGridView.qml
        ui_qt/qml/views/AlbumDetailView.qml
        ui_qt/qml/views/ColumnBrowserView.qml
        ui_qt/qml/views/TrackTableView.qml
        ui_qt/qml/views/ViewSubToolbar.qml
        ui_qt/qml/menubar/AppMenuBar.qml
        ui_qt/qml/toolbar/TopToolbar.qml
        ui_qt/qml/navigation/LeftSidebar.qml
        ui_qt/qml/inspector/RightInspectorPanel.qml
        ui_qt/qml/player/PlayerDeck.qml
        ui_qt/qml/player/MiniPlayer.qml
        ui_qt/qml/statusbar/StatusBar.qml
)

target_link_libraries(ui_adapter PUBLIC
    player_core
    picaudio_sdk
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Multimedia
    Qt6::Widgets
    $<$<BOOL:${Qt6DBus_FOUND}>:Qt6::DBus>
    ${SQLITE_LIBRARIES}
    ${TAGLIB_LIBRARIES}
)

if(WIN32)
    target_link_libraries(ui_adapter PUBLIC User32 Shell32 Ole32)
endif()

target_include_directories(ui_adapter PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ui_qt
    ${CMAKE_CURRENT_SOURCE_DIR}/ui_qt/theming
    ${CMAKE_CURRENT_SOURCE_DIR}/ui_qt/models
    ${CMAKE_CURRENT_SOURCE_DIR}/ui_qt/providers
    ${CMAKE_CURRENT_SOURCE_DIR}/metadata
    ${CMAKE_CURRENT_SOURCE_DIR}/storage
    ${CMAKE_CURRENT_SOURCE_DIR}/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/desktop
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin
    ${SQLITE_INCLUDE_DIRS}
    ${TAGLIB_INCLUDE_DIRS}
)
