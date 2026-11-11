find_package(Qt6 REQUIRED COMPONENTS Core Network Concurrent Multimedia)

pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libswresample libavutil)
pkg_check_modules(ICU REQUIRED icu-i18n icu-uc)

set(ENGINE_SOURCES
    audioanalysisbus.cpp
    audiobuffer.cpp
    audioclock.cpp
    audioconverter.cpp
    audioencoderregistry.cpp
    audioengine.cpp
    audioformat.cpp
    audioinput.cpp
    audioloader.cpp
    audioutils.cpp
    corepaths.cpp
    enginehandler.cpp
    enginehelpers.cpp
    fadingdefs.cpp
    internalcoresettings.cpp
    stringpool.cpp
    track.cpp
    visualisationbackend.cpp
    visualisationservice.cpp

    control/enginetaskqueue.cpp
    control/playbackintentreducer.cpp
    control/playbacktransitioncoordinator.cpp
    control/positioncoordinator.cpp
    control/seekplanner.cpp
    control/trackloadplanner.cpp

    decode/decodercontext.cpp
    decode/decodingcontroller.cpp
    decode/nexttrackpreparer.cpp

    dsp/downmixtomonodsp.cpp
    dsp/downmixtostereodsp.cpp
    dsp/dspchain.cpp
    dsp/dspchainstore.cpp
    dsp/dspnode.cpp
    dsp/dspregistry.cpp
    dsp/monotostereodsp.cpp
    dsp/processingbuffer.cpp
    dsp/processingbufferlist.cpp
    dsp/realfft.cpp
    dsp/resamplerdsp.cpp
    dsp/reversestereodsp.cpp
    dsp/skipsilencedsp.cpp

    input/carrierdecoder.cpp
    input/archiveinput.cpp
    input/ffmpeg/ffmpegcodec.cpp
    input/ffmpeg/ffmpegencoder.cpp
    input/ffmpeg/ffmpegframe.cpp
    input/ffmpeg/ffmpeginput.cpp
    input/ffmpeg/ffmpegstream.cpp
    input/ffmpeg/ffmpegutils.cpp
    input/metadata/id3.cpp
    input/metadata/mpegproperties.cpp
    input/playcounttagpolicy.cpp
    input/ratingtagpolicy.cpp
    input/taglibparser.cpp
    input/tagpolicy.cpp

    output/fadecontroller.cpp
    output/outputcontroller.cpp
    output/outputfader.cpp
    output/postprocessor/replaygainprocessor.cpp

    pipeline/audiopipeline.cpp
    pipeline/audiostream.cpp
    pipeline/buffereddspstage.cpp
    pipeline/chunktimelineassembler.cpp
    pipeline/orphanstreamtracker.cpp
    pipeline/outputpump.cpp
    pipeline/pipelineoutput.cpp
    pipeline/pipelinerenderer.cpp
    pipeline/pipelinesignalmailbox.cpp
    pipeline/pipelinethreadhost.cpp
    pipeline/pipelinetimeline.cpp
    pipeline/sampleringbuffer.cpp
    pipeline/timedaudiofifo.cpp
    pipeline/mixer/audiomixer.cpp
    pipeline/mixer/mixeroutput.cpp
    pipeline/mixer/mixertimeline.cpp

    outputs/qtaudio/qtaudiooutput.cpp

    player/playbackcursor.cpp
    player/playbackorder.cpp
    player/playbackordernavigator.cpp
    player/playbackprogresstracker.cpp
    player/playbackqueue.cpp
    player/playbacksession.cpp
    player/playbackstatestore.cpp
    player/playercontroller.cpp
    player/playlistnavigator.cpp
    playlist/playlistchangeset.cpp
    playlist/playlist.cpp
    playlist/playlisthandler.cpp
    library/libraryutils.cpp

    include/core/coresettings.h
    include/core/engine/audiooutput.h
    include/core/engine/enginecontroller.h
    include/core/engine/enginedefs.h
    include/core/player/playerdefs.h
    control/playbackphase.h

    pffft/pffft.c
    pffft/pffft_common.c
    utils/crypto.cpp
    utils/datastream.cpp
    utils/id.cpp
    utils/stringutils.cpp
    utils/settings/settingsentry.cpp
    utils/settings/settingsmanager.cpp
    network/remotestreamdevice.cpp
)

if(ALSA_FOUND)
    list(APPEND ENGINE_SOURCES outputs/alsa/alsaoutput.cpp)
endif()

if(PIPEWIRE_FOUND)
    list(APPEND ENGINE_SOURCES
        outputs/pipewire/pipewirecontext.cpp
        outputs/pipewire/pipewirecore.cpp
        outputs/pipewire/pipewireoutput.cpp
        outputs/pipewire/pipewireregistry.cpp
        outputs/pipewire/pipewirestream.cpp
        outputs/pipewire/pipewirethreadloop.cpp
    )
endif()

if(WIN32)
    list(APPEND ENGINE_SOURCES outputs/wasapi/wasapioutput.cpp)
endif()

qt_add_library(picaudio_engine STATIC ${ENGINE_SOURCES})

target_include_directories(picaudio_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/control
    ${CMAKE_CURRENT_SOURCE_DIR}/decode
    ${CMAKE_CURRENT_SOURCE_DIR}/dsp
    ${CMAKE_CURRENT_SOURCE_DIR}/output
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/mixer
    ${CMAKE_CURRENT_SOURCE_DIR}/outputs
    ${CMAKE_CURRENT_SOURCE_DIR}/outputs/qtaudio
    ${CMAKE_CURRENT_SOURCE_DIR}/outputs/alsa
    ${CMAKE_CURRENT_SOURCE_DIR}/outputs/pipewire
    ${CMAKE_CURRENT_SOURCE_DIR}/outputs/wasapi
    ${CMAKE_CURRENT_SOURCE_DIR}/player
    ${CMAKE_CURRENT_SOURCE_DIR}/playlist
    ${CMAKE_CURRENT_SOURCE_DIR}/pffft
    ${CMAKE_CURRENT_SOURCE_DIR}/input/ffmpeg
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/settings
    ${FFMPEG_INCLUDE_DIRS}
    ${TAGLIB_INCLUDE_DIRS}
    ${ICU_INCLUDE_DIRS}
)

if(ALSA_FOUND)
    target_include_directories(picaudio_engine PUBLIC ${ALSA_INCLUDE_DIRS})
endif()

if(PIPEWIRE_FOUND)
    target_include_directories(picaudio_engine PUBLIC ${PIPEWIRE_INCLUDE_DIRS})
endif()

target_link_libraries(picaudio_engine PUBLIC
    Qt6::Core
    Qt6::Network
    Qt6::Concurrent
    Qt6::Multimedia
    ${FFMPEG_LIBRARIES}
    ${TAGLIB_LIBRARIES}
    ${ICU_LIBRARIES}
    $<$<BOOL:${ALSA_FOUND}>:${ALSA_LIBRARIES}>
    $<$<BOOL:${PIPEWIRE_FOUND}>:${PIPEWIRE_LIBRARIES}>
)

if(WIN32)
    target_link_libraries(picaudio_engine PUBLIC Avrt Ole32)
endif()
