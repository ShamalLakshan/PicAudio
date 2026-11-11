/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include "ffmpegcodec.h"

namespace PicAudioEngine {
Codec::Codec(CodecContextPtr context, AVStream* stream)
    : m_context{std::move(context)}
    , m_stream{stream}
{ }

Codec::Codec(Codec&& other) noexcept
    : m_context{std::move(other.m_context)}
    , m_stream{other.m_stream}
{ }

Codec& Codec::operator=(Codec&& other) noexcept
{
    if(other.m_context) {
        m_context = std::move(other.m_context);
    }
    if(other.m_stream) {
        m_stream = other.m_stream;
    }
    return *this;
}

bool Codec::isValid() const
{
    return m_context && m_stream;
}

AVCodecContext* Codec::context() const
{
    return m_context.get();
}

AVStream* Codec::stream() const
{
    return m_stream;
}

int Codec::streamIndex() const
{
    return m_stream ? m_stream->index : -1;
}

bool Codec::isPlanar() const
{
    return av_sample_fmt_is_planar(m_context->sample_fmt);
}
} // namespace PicAudioEngine
