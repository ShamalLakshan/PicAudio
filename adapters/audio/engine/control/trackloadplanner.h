/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <cstdint>

namespace PicAudioEngine {
// Pure strategy planner for loadTrack decisions.
// Output describes strategy ordering only; timing/buffering remains runtime execution logic.
enum class LoadStrategy : uint8_t
{
    SegmentSwitch = 0,
    ManualFadeDefer,
    AutoTransition,
    FullReinit
};

struct TrackLoadContext
{
    bool manualChange{false};
    bool isPlaying{false};
    bool decoderValid{false};
    bool hasPendingTransportFade{false};
    bool involvesRemoteStream{false};
    bool isContiguousSameFileSegment{false};
};

struct LoadPlan
{
    LoadStrategy firstStrategy{LoadStrategy::FullReinit};
    bool trySegmentSwitch{false};
    bool tryManualFadeDefer{false};
    bool tryAutoTransition{false};
};

PICAUDIO_ENGINE_EXPORT LoadPlan planTrackLoad(const TrackLoadContext& context);
} // namespace PicAudioEngine
