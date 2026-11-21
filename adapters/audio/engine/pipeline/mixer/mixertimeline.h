/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once
#ifndef PICAUDIO_ENGINE_MIXERTIMELINE_H
#define PICAUDIO_ENGINE_MIXERTIMELINE_H

#include "audiomixer.h"

#include <cstdint>
#include <span>
#include <vector>

namespace PicAudioEngine::Timeline {
/*!
 * Timeline utilities for mixer output segments.
 *
 * Provides helpers for appending/merging contiguous segments and consuming
 * timeline ranges aligned to drained frame windows.
 */
//! Append a segment, merging with tail where continuity allows.
PICAUDIO_ENGINE_EXPORT void appendSegment(std::vector<AudioMixer::TimelineSegment>& segments,
                                 const AudioMixer::TimelineSegment& segment);
//! Append unknown-timestamp segment for `frames`.
PICAUDIO_ENGINE_EXPORT void appendUnknown(std::vector<AudioMixer::TimelineSegment>& segments, int frames,
                                 StreamId streamId = InvalidStreamId);
//! Convert chunk timeline range into mixer segments for drained output window.
PICAUDIO_ENGINE_EXPORT void consumeRange(std::span<const TimedAudioFifo::TimelineChunk> chunks, int offsetFrames, int frames,
                                StreamId fallbackStreamId, std::vector<AudioMixer::TimelineSegment>& out);
//! First valid start timestamp across segments (if present).
[[nodiscard]] PICAUDIO_ENGINE_EXPORT bool startNs(const std::vector<AudioMixer::TimelineSegment>& segments,
                                         uint64_t& startNsOut);
} // namespace PicAudioEngine::Timeline
#endif // PICAUDIO_ENGINE_MIXERTIMELINE_H
