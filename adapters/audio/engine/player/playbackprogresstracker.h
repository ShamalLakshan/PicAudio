/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <cstdint>
#include <optional>

namespace PicAudioEngine {
class PICAUDIO_ENGINE_EXPORT PlaybackProgressTracker
{
public:
    struct PositionUpdate
    {
        uint64_t positionMs{0};
        std::optional<uint64_t> positionSeconds;
        bool reachedPlayedThreshold{false};
    };

    PlaybackProgressTracker();

    void reset();
    void onTrackCommitted(uint64_t totalDurationMs, double playedThresholdFraction, uint64_t playedThresholdTimeMs);
    void updateDuration(uint64_t totalDurationMs, double playedThresholdFraction, uint64_t playedThresholdTimeMs);
    bool restartTracking();

    [[nodiscard]] PositionUpdate restorePosition(uint64_t posMs);
    [[nodiscard]] PositionUpdate restoreProgress(uint64_t posMs, uint64_t timeListenedMs);
    [[nodiscard]] PositionUpdate updatePosition(uint64_t posMs);
    [[nodiscard]] PositionUpdate resetPosition();

    bool markSeekPosition(uint64_t posMs);
    [[nodiscard]] bool updateBitrate(int bitrate);

    [[nodiscard]] uint64_t totalDuration() const;
    [[nodiscard]] uint64_t position() const;
    [[nodiscard]] uint64_t timeListened() const;
    [[nodiscard]] uint64_t playedThreshold() const;
    [[nodiscard]] bool playedThresholdReached() const;
    [[nodiscard]] int bitrate() const;

private:
    [[nodiscard]] PositionUpdate currentPositionUpdate(bool reachedPlayedThreshold = false);

    uint64_t m_totalDuration;
    uint64_t m_position;
    uint64_t m_timeListened;
    uint64_t m_playedThreshold;
    int m_bitrate;
    uint64_t m_lastPositionSecond;
    bool m_hasLastPositionSecond;
    bool m_trackingActive;
    bool m_seeking;
    bool m_counted;
};
} // namespace PicAudioEngine
