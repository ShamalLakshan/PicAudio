/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include "seekplanner.h"

#include <algorithm>
#include <limits>

namespace PicAudioEngine {
SeekPlan planSeek(const SeekPlanContext& context)
{
    SeekPlan plan;

    if(!context.decoderValid) {
        plan.strategy = SeekStrategy::NoOp;
        return plan;
    }

    if(!context.isPlaying) {
        plan.strategy = SeekStrategy::SimpleSeek;
        return plan;
    }

    if(context.crossfadeEnabled && context.trackDurationMs > 0) {
        const int autoFadeOutMs = std::max(0, context.autoCrossfadeOutMs);
        if(autoFadeOutMs > 0) {
            const auto fadeWindowMs = static_cast<uint64_t>(autoFadeOutMs);
            const uint64_t triggerPosMs
                = context.trackDurationMs > fadeWindowMs ? (context.trackDurationMs - fadeWindowMs) : 0;
            if(context.requestedPositionMs >= triggerPosMs) {
                plan.strategy = SeekStrategy::SimpleSeek;
                return plan;
            }
        }
    }

    if(!context.crossfadeEnabled || (context.seekFadeOutMs <= 0 && context.seekFadeInMs <= 0)) {
        plan.strategy = SeekStrategy::SimpleSeek;
        return plan;
    }

    if(!context.hasCurrentStream) {
        plan.strategy = SeekStrategy::SimpleSeek;
        return plan;
    }

    const int requestedFadeOutMs = std::max(0, context.seekFadeOutMs);
    if(requestedFadeOutMs > 0) {
        const int highWatermarkMs = std::max(0, context.decoderHighWatermarkMs);
        plan.reserveTargetMs      = std::min(std::max(requestedFadeOutMs, highWatermarkMs), context.bufferLengthMs);
    }

    const int availableFadeOutMs = static_cast<int>(
        std::min<uint64_t>(context.bufferedDurationMs, static_cast<uint64_t>(std::numeric_limits<int>::max())));
    plan.fadeOutDurationMs = std::min(requestedFadeOutMs, availableFadeOutMs);
    if(plan.fadeOutDurationMs <= 0) {
        plan.strategy          = SeekStrategy::SimpleSeek;
        plan.fadeOutDurationMs = 0;
        plan.fadeInDurationMs  = 0;
        return plan;
    }

    plan.fadeInDurationMs = std::max(0, context.seekFadeInMs);
    plan.strategy         = SeekStrategy::CrossfadeSeek;

    return plan;
}
} // namespace PicAudioEngine
