/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include "ffmpegstream.h"

#include <core/engine/audioformat.h>

#include <QLoggingCategory>

extern "C"
{
#include <libavcodec/avcodec.h>
}

#include <memory>

class QString;
struct AVCodecParameters;
struct AVCodecContext;
struct AVAudioFifo;
struct AVFormatContext;
struct AVIOContext;
struct AVPacket;
struct SwrContext;

#define OLD_CHANNEL_LAYOUT (LIBAVCODEC_VERSION_INT < AV_VERSION_INT(59, 24, 100))
#define OLD_CODEC_SUPPORTED_CONFIG (LIBAVCODEC_VERSION_MAJOR < 61)
#define OLD_FRAME (LIBAVUTIL_VERSION_MAJOR < 58)

PICAUDIO_ENGINE_EXPORT Q_DECLARE_LOGGING_CATEGORY(FFMPEG)

struct IOContextDeleter
{
    void operator()(AVIOContext* context) const;
};
using IOContextPtr = std::unique_ptr<AVIOContext, IOContextDeleter>;

struct FormatContextDeleter
{
    void operator()(AVFormatContext* context) const;
};
using FormatContextPtr = std::unique_ptr<AVFormatContext, FormatContextDeleter>;

struct OutputFormatContextDeleter
{
    void operator()(AVFormatContext* context) const;
};
using OutputFormatContextPtr = std::unique_ptr<AVFormatContext, OutputFormatContextDeleter>;

struct CodecContextDeleter
{
    void operator()(AVCodecContext* context) const;
};
using CodecContextPtr = std::unique_ptr<AVCodecContext, CodecContextDeleter>;

struct FrameDeleter
{
    void operator()(AVFrame* frame) const;
};
using FramePtr = std::unique_ptr<AVFrame, FrameDeleter>;

struct PacketDeleter
{
    void operator()(AVPacket* packet) const;
};
using PacketPtr = std::unique_ptr<AVPacket, PacketDeleter>;

struct SwrContextDeleter
{
    void operator()(SwrContext* context) const;
};
using SwrContextPtr = std::unique_ptr<SwrContext, SwrContextDeleter>;

struct AudioFifoDeleter
{
    void operator()(AVAudioFifo* fifo) const;
};
using AudioFifoPtr = std::unique_ptr<AVAudioFifo, AudioFifoDeleter>;

namespace PicAudioEngine::Utils {
PICAUDIO_ENGINE_EXPORT QString ffmpegErrorString(int error);
PICAUDIO_ENGINE_EXPORT void printError(int error);
PICAUDIO_ENGINE_EXPORT void printError(const QString& error);

PICAUDIO_ENGINE_EXPORT SampleFormat sampleFormat(AVSampleFormat format, int bps);
PICAUDIO_ENGINE_EXPORT AVSampleFormat sampleFormat(SampleFormat format, bool planar = false);
PICAUDIO_ENGINE_EXPORT AudioFormat audioFormatFromCodec(AVCodecParameters* codec, AVSampleFormat ctxFormat);
PICAUDIO_ENGINE_EXPORT Stream findAudioStream(AVFormatContext* context);
} // namespace PicAudioEngine::Utils
