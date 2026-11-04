cmake_minimum_required(VERSION 3.20)
project(spectrum_visualizer VERSION 1.0.0 LANGUAGES CXX)

add_library(spectrum_visualizer SHARED
    SpectrumVisualizerPlugin.hpp
    SpectrumVisualizerPlugin.cpp
)

target_compile_definitions(spectrum_visualizer PRIVATE PICAUDIO_PLUGIN_BUILD)
target_link_libraries(spectrum_visualizer PRIVATE picaudio_sdk)

set_target_properties(spectrum_visualizer PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
    PREFIX ""
    OUTPUT_NAME "spectrum_visualizer"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/spectrum_visualizer"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/spectrum_visualizer"
)

# Copy plugin.json into output plugin folder
add_custom_command(TARGET spectrum_visualizer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/plugins/spectrum_visualizer"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/plugin.json"
        "${CMAKE_BINARY_DIR}/plugins/spectrum_visualizer/plugin.json"
)

install(TARGETS spectrum_visualizer
    DESTINATION lib/picaudio/plugins/spectrum_visualizer
)
install(FILES plugin.json
    DESTINATION lib/picaudio/plugins/spectrum_visualizer
)
