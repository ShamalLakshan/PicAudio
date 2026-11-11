/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "audiostream.h"

#include <chrono>
#include <cstdint>
#include <optional>

namespace PicAudioEngine {
/*!
 * Tracks a single fading-out orphan stream and its drain deadline.
 *
 * Used by `AudioPipeline` during crossfade/seek transitions where the previous
 * stream is allowed to drain briefly while a new stream becomes primary.
 */
class OrphanStreamTracker
{
public:
    struct Evaluation
    {
        bool shouldCleanup{false};
        bool timedOut{false};
    };

    OrphanStreamTracker();

    [[nodiscard]] bool hasOrphan() const;
    [[nodiscard]] StreamId streamId() const;
    [[nodiscard]] StreamId replace(StreamId streamId);

    void clear();

    //! Arm deadline using current buffered + delay estimate clamped to timeout bounds.
    void armDrainDeadline(uint64_t bufferedDurationMs, uint64_t playbackDelayMs, std::chrono::milliseconds minTimeout,
                          std::chrono::milliseconds maxTimeout);
    //! Evaluate whether orphan can remain active or should be cleaned now.
    [[nodiscard]] Evaluation evaluate(const AudioStream* orphanStream) const;

private:
    StreamId m_streamId;
    std::optional<std::chrono::steady_clock::time_point> m_drainDeadline;
};
} // namespace PicAudioEngine
