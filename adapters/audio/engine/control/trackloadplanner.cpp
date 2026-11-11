/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include "trackloadplanner.h"

namespace PicAudioEngine {
LoadPlan planTrackLoad(const TrackLoadContext& context)
{
    LoadPlan plan;

    plan.trySegmentSwitch   = !context.involvesRemoteStream && !context.manualChange && context.isPlaying
                           && context.decoderValid && context.isContiguousSameFileSegment;
    plan.tryManualFadeDefer = context.manualChange && !context.hasPendingTransportFade;
    plan.tryAutoTransition = !context.involvesRemoteStream && !context.manualChange && !context.hasPendingTransportFade;

    if(plan.trySegmentSwitch) {
        plan.firstStrategy = LoadStrategy::SegmentSwitch;
    }
    else if(plan.tryManualFadeDefer) {
        plan.firstStrategy = LoadStrategy::ManualFadeDefer;
    }
    else if(plan.tryAutoTransition) {
        plan.firstStrategy = LoadStrategy::AutoTransition;
    }
    else {
        plan.firstStrategy = LoadStrategy::FullReinit;
    }

    return plan;
}
} // namespace PicAudioEngine
