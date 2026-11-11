/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include "timedaudiofifo.h"

#include <core/engine/dsp/processingbufferlist.h>

#include <span>
#include <utility>
#include <vector>

namespace PicAudioEngine {
/*!
 * Shared helper for buffered DSP output stages:
 * - coalesces processed chunks into an output-format buffer,
 * - stores queued output and timeline metadata,
 * - provides write/compact operations for downstream pumps.
 */
class PICAUDIO_ENGINE_EXPORT BufferedDspStage
{
public:
    using TimelineChunk = TimedAudioFifo::TimelineChunk;

    struct QueueResult
    {
        int appendedFrames{0};
        bool coalesceFailed{false};
        bool zeroFrameCoalesce{false};
        bool formatReset{false};
    };

    [[nodiscard]] bool hasPendingOutput() const;
    [[nodiscard]] int pendingOutputFrames() const;
    [[nodiscard]] int pendingWriteOffsetFrames() const;
    [[nodiscard]] StreamId pendingStreamId() const;
    [[nodiscard]] uint64_t pendingEpoch() const;
    [[nodiscard]] const std::vector<TimelineChunk>& pendingTimeline() const;

    void clearPendingOutput();
    void compactPendingOutput();
    void dropPendingFrontFrames(int frames);

    TimedAudioFifo::AppendResult appendPendingOutput(const AudioBuffer& buffer, StreamId streamId);
    TimedAudioFifo::AppendResult appendPendingOutput(const AudioBuffer& buffer,
                                                     std::span<const TimelineChunk> timelineChunks, StreamId streamId);
    TimedAudioFifo::AppendResult appendPendingOutput(const AudioBuffer& buffer,
                                                     std::span<const TimelineChunk> timelineChunks, StreamId streamId,
                                                     uint64_t epoch);
    TimedAudioFifo::AppendResult appendPendingOutputOrReplace(const AudioBuffer& buffer,
                                                              std::span<const TimelineChunk> timelineChunks,
                                                              StreamId streamId);
    TimedAudioFifo::AppendResult appendPendingOutputOrReplace(const AudioBuffer& buffer,
                                                              std::span<const TimelineChunk> timelineChunks,
                                                              StreamId streamId, uint64_t epoch);
    void setPendingOutput(const AudioBuffer& buffer, std::span<const TimelineChunk> timelineChunks, StreamId streamId);
    void setPendingOutput(const AudioBuffer& buffer, std::span<const TimelineChunk> timelineChunks, StreamId streamId,
                          uint64_t epoch);

    template <typename Fn>
    TimedAudioFifo::WriteResult writePendingOutput(Fn&& writer)
    {
        return m_pendingOutput.write(std::forward<Fn>(writer));
    }

    QueueResult queueProcessedOutput(const ProcessingBufferList& chunks, const AudioFormat& outputFormat, bool dither,
                                     StreamId streamId, uint64_t timelineEpoch = 0);

private:
    AudioBuffer m_coalescedOutputBuffer;
    TimedAudioFifo m_pendingOutput;
    std::vector<TimelineChunk> m_timelineScratch;
};
} // namespace PicAudioEngine
