/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <core/track.h>

namespace PicAudioEngine::TrackPlayback {
constexpr auto ExclusionProperty          = "_PLAYBACK_EXCLUSION";
constexpr auto DisableCrossfadingProperty = "_PLAYBACK_DISABLE_CROSSFADING";

inline bool isExcluded(const Track& track, bool shuffling)
{
    if(!track.hasExtraProperty(QString::fromLatin1(ExclusionProperty))) {
        return false;
    }

    const QString value = track.extraProperties().value(QString::fromLatin1(ExclusionProperty));
    return value == QLatin1StringView{"always"} || (shuffling && value == QLatin1StringView{"shuffle"});
}

inline bool crossfadingDisabled(const Track& track)
{
    return track.hasExtraProperty(QString::fromLatin1(DisableCrossfadingProperty))
        && track.extraProperties().value(QString::fromLatin1(DisableCrossfadingProperty)) == QLatin1StringView{"1"};
}
} // namespace PicAudioEngine::TrackPlayback
