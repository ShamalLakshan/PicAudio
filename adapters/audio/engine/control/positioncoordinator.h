/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include "core/engine/pipeline/audiopipeline.h"

#include <core/engine/enginedefs.h>

namespace PicAudioEngine {
class PICAUDIO_ENGINE_EXPORT PositionCoordinator
{
public:
    struct PreparedCrossfadeContext
    {
        bool armed{false};
        StreamId streamId{InvalidStreamId};
        uint64_t bufferedAtArmMs{0};
        uint64_t boundaryLeadMs{0};
        bool boundarySignalled{false};
    };

    struct Input
    {
        Engine::PlaybackState playbackState{Engine::PlaybackState::Stopped};
        bool seekInProgress{false};
        bool decoderTimerActive{false};
        int decoderLowWatermarkMs{0};

        StreamId streamId{InvalidStreamId};
        AudioStream::State streamState{AudioStream::State::Stopped};
        bool streamEndOfInput{false};
        uint64_t streamBufferedDurationMs{0};
        uint64_t streamPositionMs{0};

        uint64_t streamToTrackOriginMs{0};
        uint64_t trackOffsetMs{0};

        uint64_t pipelineDelayMs{0};
        double delayToSourceScale{1.0};
        PipelineStatus pipelineStatus;

        PreparedCrossfadeContext preparedCrossfade;
    };

    struct Output
    {
        bool shouldEnsureDecodeTimer{false};
        bool positionAvailable{false};
        bool discontinuity{false};
        bool emitNow{false};
        uint64_t relativePosMs{0};
        bool pendingWithoutMappedAudio{false};
        bool hasRenderedSegment{false};
        bool preparedCrossfadeArmed{false};
        uint64_t trackEndingPosMs{0};
        bool boundaryFallbackReached{false};
        bool evaluateTrackEnding{false};
    };

    PositionCoordinator();

    void reset();
    void resetContinuity();
    void clearGaplessHold();
    void setGaplessHold(StreamId streamId);
    void notePublishedDiscontinuity(uint64_t sourcePositionMs);

    [[nodiscard]] Output evaluate(const Input& input);

private:
    bool m_lastSourcePositionValid;
    uint64_t m_lastSourcePositionMs;
    StreamId m_lastSourceStreamId;
    StreamId m_gaplessHoldStreamId;
};
} // namespace PicAudioEngine
