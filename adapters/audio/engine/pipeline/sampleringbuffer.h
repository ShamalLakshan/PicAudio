/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <cstddef>
#include <vector>

namespace PicAudioEngine {
/*!
 * Lightweight single-thread sample FIFO used in mixer scratch paths.
 *
 * Unlike `LockFreeRingBuffer`, this container is not thread-safe and is only
 * used from the audio thread for temporary buffering.
 */
class PICAUDIO_ENGINE_EXPORT SampleRingBuffer
{
public:
    SampleRingBuffer();

    //! Remove all buffered samples.
    void clear();

    //! Current number of buffered samples.
    [[nodiscard]] size_t size() const;
    //! Ensure capacity can hold at least `minCapacity` samples.
    void ensureCapacity(size_t minCapacity);

    //! Append `count` samples from `src`.
    void push(const double* src, size_t count);
    //! Append `count` zero-valued samples.
    void pushZeros(size_t count);
    //! Pop up to `count` samples into `dst`, returning actual popped count.
    size_t pop(double* dst, size_t count);

private:
    std::vector<double> m_buffer;
    size_t m_head;
    size_t m_size;
};
} // namespace PicAudioEngine
