/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <QSharedDataPointer>

#if defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

extern "C"
{
#include <libavutil/frame.h>
}

#if defined(__GNUG__)
#pragma GCC diagnostic pop
#elif defined(__clang__)
#pragma clang diagnostic pop
#endif

namespace PicAudioEngine {
class FramePrivate;

class PICAUDIO_ENGINE_EXPORT Frame
{
public:
    Frame();
    explicit Frame(AVRational timeBase);
    ~Frame();

    Frame(const Frame& other);
    Frame& operator=(const Frame& other);
    Frame(Frame&& other) noexcept;

    [[nodiscard]] bool isValid() const;

    [[nodiscard]] AVFrame* avFrame() const;

    [[nodiscard]] int channelCount() const;
    [[nodiscard]] int sampleRate() const;
    [[nodiscard]] AVSampleFormat format() const;

    [[nodiscard]] int sampleCount() const;

    [[nodiscard]] uint64_t ptsMs() const;
    [[nodiscard]] uint64_t durationMs() const;

    [[nodiscard]] uint64_t end() const;

private:
    QSharedDataPointer<FramePrivate> p;
};
} // namespace PicAudioEngine
