/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "ffmpegutils.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace PicAudioEngine {
class Codec
{
public:
    Codec() = default;
    Codec(CodecContextPtr context, AVStream* stream);

    Codec(Codec&& other) noexcept;
    Codec& operator=(Codec&& other) noexcept;

    Codec(const Codec& other)            = delete;
    Codec& operator=(const Codec& other) = delete;

    [[nodiscard]] bool isValid() const;

    [[nodiscard]] AVCodecContext* context() const;
    [[nodiscard]] AVStream* stream() const;
    [[nodiscard]] int streamIndex() const;
    [[nodiscard]] bool isPlanar() const;

private:
    CodecContextPtr m_context;
    AVStream* m_stream;
};
} // namespace PicAudioEngine
