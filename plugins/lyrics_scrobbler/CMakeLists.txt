cmake_minimum_required(VERSION 3.20)
project(lyrics_scrobbler VERSION 1.0.0 LANGUAGES CXX)

add_library(lyrics_scrobbler SHARED
    LyricsScrobblerPlugin.hpp
    LyricsScrobblerPlugin.cpp
)

target_compile_definitions(lyrics_scrobbler PRIVATE PICAUDIO_PLUGIN_BUILD)
target_link_libraries(lyrics_scrobbler PRIVATE picaudio_sdk)

set_target_properties(lyrics_scrobbler PROPERTIES
    PREFIX ""
    OUTPUT_NAME "lyrics_scrobbler"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/lyrics_scrobbler"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/lyrics_scrobbler"
)

# Copy plugin.json into output plugin folder
add_custom_command(TARGET lyrics_scrobbler POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/plugins/lyrics_scrobbler"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/plugin.json"
        "${CMAKE_BINARY_DIR}/plugins/lyrics_scrobbler/plugin.json"
)

install(TARGETS lyrics_scrobbler
    DESTINATION lib/picaudio/plugins/lyrics_scrobbler
)
install(FILES plugin.json
    DESTINATION lib/picaudio/plugins/lyrics_scrobbler
)
