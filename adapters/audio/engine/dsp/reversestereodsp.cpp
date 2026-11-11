/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include "reversestereodsp.h"

namespace PicAudioEngine {
QString ReverseStereoDsp::name() const
{
    return QStringLiteral("Reverse stereo channels");
}

QString ReverseStereoDsp::id() const
{
    return QStringLiteral("picaudio.engine.dsp.reversestereo");
}

void ReverseStereoDsp::prepare(const AudioFormat& format)
{
    m_format = format;
}

void ReverseStereoDsp::process(ProcessingBufferList& chunks)
{
    const size_t count = chunks.count();
    for(size_t i{0}; i < count; ++i) {
        auto* buffer = chunks.item(i);
        if(!buffer || !buffer->isValid()) {
            continue;
        }

        const int channels = buffer->format().channelCount();
        if(channels != 2) {
            continue;
        }

        const int frames = buffer->frameCount();
        if(frames <= 0) {
            continue;
        }

        auto dataSpan            = buffer->data();
        const size_t sampleCount = static_cast<size_t>(frames) * static_cast<size_t>(channels);
        if(dataSpan.size() < sampleCount) {
            continue;
        }

        double* data = dataSpan.data();
        for(size_t base{0}; base + 1 < sampleCount; base += 2) {
            std::swap(data[base], data[base + 1]);
        }
    }
}
} // namespace PicAudioEngine
