/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include "orphanstreamtracker.h"

#include "audiostream.h"

#include <algorithm>
#include <cstdint>
#include <limits>

namespace PicAudioEngine {
OrphanStreamTracker::OrphanStreamTracker()
    : m_streamId{InvalidStreamId}
{ }

bool OrphanStreamTracker::hasOrphan() const
{
    return m_streamId != InvalidStreamId;
}

StreamId OrphanStreamTracker::streamId() const
{
    return m_streamId;
}

StreamId OrphanStreamTracker::replace(const StreamId streamId)
{
    if(streamId == InvalidStreamId) {
        clear();
        return InvalidStreamId;
    }

    const StreamId displacedStreamId
        = (m_streamId != InvalidStreamId && m_streamId != streamId) ? m_streamId : InvalidStreamId;

    m_streamId = streamId;
    m_drainDeadline.reset();

    return displacedStreamId;
}

void OrphanStreamTracker::clear()
{
    m_streamId = InvalidStreamId;
    m_drainDeadline.reset();
}

void OrphanStreamTracker::armDrainDeadline(uint64_t bufferedDurationMs, uint64_t playbackDelayMs,
                                           const std::chrono::milliseconds minTimeout,
                                           const std::chrono::milliseconds maxTimeout)
{
    if(!hasOrphan()) {
        return;
    }

    uint64_t baseTimeoutMs = bufferedDurationMs;
    if(baseTimeoutMs > std::numeric_limits<uint64_t>::max() - playbackDelayMs) {
        baseTimeoutMs = std::numeric_limits<uint64_t>::max();
    }
    else {
        baseTimeoutMs += playbackDelayMs;
    }

    static constexpr uint64_t CleanupGraceMs = 1000;
    if(baseTimeoutMs > std::numeric_limits<uint64_t>::max() - CleanupGraceMs) {
        baseTimeoutMs = std::numeric_limits<uint64_t>::max();
    }
    else {
        baseTimeoutMs += CleanupGraceMs;
    }

    const uint64_t minTimeoutMs     = static_cast<uint64_t>(std::max<int64_t>(0, minTimeout.count()));
    const uint64_t maxTimeoutMs     = static_cast<uint64_t>(std::max<int64_t>(0, maxTimeout.count()));
    const uint64_t clampedTimeoutMs = std::clamp(baseTimeoutMs, minTimeoutMs, std::max(minTimeoutMs, maxTimeoutMs));

    m_drainDeadline = std::chrono::steady_clock::now() + std::chrono::milliseconds{clampedTimeoutMs};
}

OrphanStreamTracker::Evaluation OrphanStreamTracker::evaluate(const AudioStream* orphanStream) const
{
    Evaluation evaluation;

    if(!hasOrphan()) {
        return evaluation;
    }

    if(!orphanStream) {
        evaluation.shouldCleanup = true;
        return evaluation;
    }

    const bool orphanFinished = orphanStream->state() == AudioStream::State::Stopped
                             || (orphanStream->bufferEmpty() && orphanStream->fadeGain() < 0.01);
    const bool orphanTimedOut = m_drainDeadline.has_value() && std::chrono::steady_clock::now() >= *m_drainDeadline;

    evaluation.shouldCleanup = orphanFinished || orphanTimedOut;
    evaluation.timedOut      = orphanTimedOut && !orphanFinished;
    return evaluation;
}
} // namespace PicAudioEngine
