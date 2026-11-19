# PicAudio Plugin Packaging Helper
# Creates a self-contained .paplugin ZIP archive containing multi-platform binaries,
# manifest, and companion resources.

function(picaudio_create_plugin_bundle TARGET_NAME)
    cmake_parse_arguments(BUNDLE "" "NAME;OUTPUT_DIR" "RESOURCES;RUNTIME_TARGETS" ${ARGN})

    if(NOT BUNDLE_NAME)
        set(BUNDLE_NAME "${TARGET_NAME}")
    endif()

    if(NOT BUNDLE_OUTPUT_DIR)
        set(BUNDLE_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    # Determine standard platform directory name
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|aarch64")
            set(PLATFORM_NAME "windows-arm64")
        else()
            set(PLATFORM_NAME "windows-x64")
        endif()
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|arm64")
            set(PLATFORM_NAME "macos-arm64")
        else()
            set(PLATFORM_NAME "macos-x64")
        endif()
    else()
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|aarch64")
            set(PLATFORM_NAME "linux-arm64")
        else()
            set(PLATFORM_NAME "linux-x64")
        endif()
    endif()

    set(STAGE_DIR "${CMAKE_CURRENT_BINARY_DIR}/bundle_stage")
    set(ARCHIVE_FILE "${BUNDLE_OUTPUT_DIR}/${BUNDLE_NAME}.paplugin")

    add_custom_target(${TARGET_NAME}_bundle
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${STAGE_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${STAGE_DIR}/${PLATFORM_NAME}"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${TARGET_NAME}>" "${STAGE_DIR}/${PLATFORM_NAME}/"
        COMMENT "Staging PicAudio plugin bundle for ${TARGET_NAME} (${PLATFORM_NAME})..."
        VERBATIM
    )
    add_dependencies(${TARGET_NAME}_bundle ${TARGET_NAME})

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugin.json")
        add_custom_command(TARGET ${TARGET_NAME}_bundle POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/plugin.json" "${STAGE_DIR}/plugin.json"
            VERBATIM
        )
    endif()

    if(BUNDLE_RESOURCES)
        foreach(RES_FILE IN LISTS BUNDLE_RESOURCES)
            add_custom_command(TARGET ${TARGET_NAME}_bundle POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${RES_FILE}" "${STAGE_DIR}/"
                VERBATIM
            )
        endforeach()
    endif()

    add_custom_command(TARGET ${TARGET_NAME}_bundle POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E tar cfv "${ARCHIVE_FILE}" --format=zip -- .
        WORKING_DIRECTORY "${STAGE_DIR}"
        COMMENT "Generated PicAudio plugin bundle: ${ARCHIVE_FILE}"
        VERBATIM
    )
endfunction()
