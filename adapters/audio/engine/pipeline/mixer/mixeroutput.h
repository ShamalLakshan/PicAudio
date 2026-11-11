/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "audiomixer.h"

#include <core/engine/dsp/processingbuffer.h>
#include <core/engine/pipeline/timedaudiofifo.h>

#include <array>
#include <span>
#include <vector>

namespace PicAudioEngine {
/*!
 * Mixer output queue that preserves source timeline metadata.
 *
 * `AudioMixer` commits rendered interleaved F64 samples and associated timeline
 * segments here, while pipeline consumers drain frame windows from it on demand.
 */
class MixerOutput
{
public:
    MixerOutput();

    struct CommitPlan
    {
        int outRate{0};
        int outChannels{0};
        int requestedFrames{0};
        StreamId pendingStreamToStartId{InvalidStreamId};
    };

    struct CycleState
    {
        bool startedPendingStream{false};
        bool hasActiveStream{false};
        bool dspBuffering{false};
        int producedFrames{0};
        StreamId trackedPrimaryStreamId{InvalidStreamId};
    };

    void clear();
    [[nodiscard]] int queuedFrames() const;

    void commitMixed(const CommitPlan& plan, const CycleState& state, std::span<const double> mixedInterleaved,
                     std::span<const AudioMixer::TimelineSegment> outputTimeline);

    int drain(ProcessingBuffer& output, int requestedFrames, int outChannels,
              std::vector<AudioMixer::TimelineSegment>& consumedTimeline);

private:
    TimedAudioFifo m_queue;
    std::vector<TimedAudioFifo::TimelineChunk> m_timelineScratch;
    AudioBuffer m_silenceScratch;
    std::array<TimedAudioFifo::TimelineChunk, 1> m_silenceTimeline;
};
} // namespace PicAudioEngine
