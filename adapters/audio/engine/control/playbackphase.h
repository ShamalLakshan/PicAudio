/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <QObject>

#include <cstdint>

namespace PicAudioEngine::Playback {
Q_NAMESPACE

enum class Phase : uint8_t
{
    Stopped = 0,
    Loading,
    Playing,
    Paused,
    Seeking,
    SeekCrossfading,
    TrackCrossfading,
    FadingToPause,
    FadingToStop,
    FadingToManualChange,
    Error
};
Q_ENUM_NS(Phase)

[[nodiscard]] constexpr bool isPlaybackActive(Phase phase)
{
    switch(phase) {
        case Phase::Playing:
        case Phase::Seeking:
        case Phase::SeekCrossfading:
        case Phase::TrackCrossfading:
        case Phase::FadingToPause:
        case Phase::FadingToStop:
        case Phase::FadingToManualChange:
            return true;
        default:
            return false;
    }
}

[[nodiscard]] constexpr bool isCrossfading(Phase phase)
{
    return phase == Phase::SeekCrossfading || phase == Phase::TrackCrossfading;
}

[[nodiscard]] constexpr bool isFadingTransport(Phase phase)
{
    return phase == Phase::FadingToPause || phase == Phase::FadingToStop || phase == Phase::FadingToManualChange;
}
} // namespace PicAudioEngine::Playback
