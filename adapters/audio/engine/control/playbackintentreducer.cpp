/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include "playbackintentreducer.h"

namespace PicAudioEngine {
PlaybackAction reducePlaybackIntent(const PlaybackIntentReducerState& state, const PlaybackIntent intent)
{
    const bool needsAudiblePause = state.fadingEnabled && (state.pauseFadeInMs > 0 || state.pauseFadeOutMs > 0);
    const bool canFadeStop       = state.fadingEnabled && state.stopFadeOutMs > 0;

    switch(intent) {
        case PlaybackIntent::Play:
            return state.playbackState == Engine::PlaybackState::Playing ? PlaybackAction::Continue
                                                                         : PlaybackAction::Immediate;
        case PlaybackIntent::Pause:
            if(state.playbackState == Engine::PlaybackState::Stopped
               || state.playbackState == Engine::PlaybackState::Paused) {
                return PlaybackAction::NoOp;
            }
            if(state.fadeState == FadeState::FadingToPause) {
                return PlaybackAction::Continue;
            }
            if(needsAudiblePause) {
                return PlaybackAction::BeginFade;
            }
            return PlaybackAction::Immediate;
        case PlaybackIntent::Stop:
            if(state.playbackState == Engine::PlaybackState::Stopped) {
                return PlaybackAction::NoOp;
            }
            if(state.fadeState == FadeState::FadingToStop) {
                return PlaybackAction::Continue;
            }
            if(canFadeStop && state.playbackState == Engine::PlaybackState::Playing) {
                return PlaybackAction::BeginFade;
            }
            return PlaybackAction::Immediate;
    }

    std::unreachable();
}
} // namespace PicAudioEngine
