/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include "core/engine/audioformat.h"

#include <limits>

namespace PicAudioEngine {
class Track;

//! True when two track handles refer to the same logical track request.
//! Uses DB id when available, otherwise falls back to segment identity.
PICAUDIO_ENGINE_EXPORT bool sameTrackIdentity(const Track& lhs, const Track& rhs);
//! True when two tracks refer to the exact same logical segment.
PICAUDIO_ENGINE_EXPORT bool sameTrackSegment(const Track& lhs, const Track& rhs);
//! True when @p nextTrack starts exactly at current offset + duration in same file/subsong.
PICAUDIO_ENGINE_EXPORT bool isContiguousSameFileSegment(const Track& currentTrack, const Track& nextTrack);
//! True when both tracks point to same file but represent different segment metadata.
PICAUDIO_ENGINE_EXPORT bool isMultiTrackFileTransition(const Track& currentTrack, const Track& nextTrack);
//! Convert absolute position to position relative to segment offset.
PICAUDIO_ENGINE_EXPORT uint64_t relativeTrackPositionMs(uint64_t absolutePosMs, uint64_t offsetMs);
//! Convert stream-relative position to absolute timeline position.
PICAUDIO_ENGINE_EXPORT uint64_t absoluteTrackPositionMs(uint64_t streamPosMs, uint64_t streamOriginMs);
//! Convert stream-relative position to track-relative logical position.
PICAUDIO_ENGINE_EXPORT uint64_t relativeTrackPositionMs(uint64_t streamPosMs, uint64_t streamOriginMs, uint64_t trackOffsetMs);
//! Latch helper to emit track-end signal only once per logical segment.
PICAUDIO_ENGINE_EXPORT bool shouldEmitTrackEndOnce(Track& lastEndedTrack, const Track& currentTrack);
//! Infer a default channel layout when positions are missing but channel count is known.
PICAUDIO_ENGINE_EXPORT AudioFormat normaliseChannelLayout(const AudioFormat& format);
//! Add two unsigned timeline values without overflow.
[[nodiscard]] constexpr uint64_t saturatingAdd(uint64_t lhs, uint64_t rhs) noexcept
{
    return lhs > (std::numeric_limits<uint64_t>::max() - rhs) ? std::numeric_limits<uint64_t>::max() : lhs + rhs;
}
} // namespace PicAudioEngine
