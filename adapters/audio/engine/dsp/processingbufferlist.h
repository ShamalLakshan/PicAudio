/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <core/engine/dsp/processingbuffer.h>

#include <vector>

namespace PicAudioEngine {
/*!
 * Mutable list of ProcessingBuffer chunks used across engine processing stages.
 */
class PICAUDIO_ENGINE_EXPORT ProcessingBufferList
{
public:
    [[nodiscard]] size_t count() const;

    ProcessingBuffer* item(size_t index);
    [[nodiscard]] const ProcessingBuffer* item(size_t index) const;

    void reserve(size_t count);
    void clear();
    void removeByIdx(size_t index);

    ProcessingBuffer* insertItem(size_t index, const AudioFormat& format, uint64_t startTimeNs, size_t sampleCount);
    ProcessingBuffer* addItem(const AudioFormat& format, uint64_t startTimeNs, size_t sampleCount);
    void addChunk(const ProcessingBuffer& buffer);
    void addChunk(ProcessingBuffer&& buffer);
    void setToSingle(const ProcessingBuffer& buffer);

private:
    std::vector<ProcessingBuffer> m_chunks;
};
} // namespace PicAudioEngine
