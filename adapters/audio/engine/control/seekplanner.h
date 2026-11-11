/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <cstdint>

namespace PicAudioEngine {
// Pure seek decision planner.
// Consumes a full context snapshot and returns a deterministic plan.
// Execution details (decoder/pipeline mutation) stay in AudioEngine.
enum class SeekStrategy : uint8_t
{
    NoOp = 0,
    SimpleSeek,
    CrossfadeSeek
};

struct SeekPlanContext
{
    bool decoderValid{false};
    bool isPlaying{false};
    bool crossfadeEnabled{false};
    uint64_t trackDurationMs{0};
    int autoCrossfadeOutMs{0};

    int seekFadeOutMs{0};
    int seekFadeInMs{0};

    bool hasCurrentStream{false};
    uint64_t requestedPositionMs{0};
    uint64_t bufferedDurationMs{0};

    int decoderHighWatermarkMs{0};
    int bufferLengthMs{0};
};

struct SeekPlan
{
    SeekStrategy strategy{SeekStrategy::NoOp};
    int reserveTargetMs{0};
    int fadeOutDurationMs{0};
    int fadeInDurationMs{0};
};

PICAUDIO_ENGINE_EXPORT SeekPlan planSeek(const SeekPlanContext& context);
} // namespace PicAudioEngine
