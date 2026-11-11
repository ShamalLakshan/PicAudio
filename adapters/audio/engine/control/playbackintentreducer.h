/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include "core/engine/output/fadecontroller.h"

#include <core/engine/enginedefs.h>

namespace PicAudioEngine {
enum class PlaybackIntent : uint8_t
{
    Play = 0,
    Pause,
    Stop
};

enum class PlaybackAction : uint8_t
{
    NoOp = 0,
    Continue,
    BeginFade,
    Immediate
};

struct PlaybackIntentReducerState
{
    Engine::PlaybackState playbackState{Engine::PlaybackState::Stopped};
    FadeState fadeState{FadeState::Idle};
    bool fadingEnabled{false};
    int pauseFadeInMs{0};
    int pauseFadeOutMs{0};
    int stopFadeOutMs{0};
};

PICAUDIO_ENGINE_EXPORT PlaybackAction reducePlaybackIntent(const PlaybackIntentReducerState& state, PlaybackIntent intent);
} // namespace PicAudioEngine
