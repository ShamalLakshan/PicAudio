/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include "timedaudiofifo.h"

#include <core/engine/dsp/processingbufferlist.h>

#include <cstdint>
#include <vector>

namespace PicAudioEngine {
/*!
 * Shared helper functions for coalescing processed chunks and building timeline metadata.
 */
namespace Timeline {
using TimelineChunk = TimedAudioFifo::TimelineChunk;

[[nodiscard]] PICAUDIO_ENGINE_EXPORT bool coalesceChunks(const ProcessingBufferList& chunks, const AudioFormat& outputFormat,
                                                AudioBuffer& combined, bool dither);
[[nodiscard]] PICAUDIO_ENGINE_EXPORT bool buildTimelineChunks(const ProcessingBufferList& chunks,
                                                     std::vector<TimelineChunk>& timelineChunks,
                                                     StreamId streamId = InvalidStreamId, uint64_t timelineEpoch = 0);
} // namespace Timeline
} // namespace PicAudioEngine
