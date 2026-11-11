/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include "ffmpegstream.h"

extern "C"
{
#include <libavformat/avformat.h>
}

namespace PicAudioEngine {
Stream::Stream()
    : Stream{nullptr}
{ }

Stream::Stream(AVStream* stream)
    : m_stream{stream}
{ }

bool Stream::isValid() const
{
    return !!m_stream;
}

int Stream::index() const
{
    return m_stream->index;
}

AVStream* Stream::avStream() const
{
    return m_stream;
}
} // namespace PicAudioEngine
