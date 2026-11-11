/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include "playbackstatestore.h"

#include <core/coresettings.h>
#include <utils/enum.h>

using namespace Qt::StringLiterals;

constexpr auto ActivePlaylistDbIdKey    = "Playlist/ActiveId"_L1;
constexpr auto ActiveTrackIndexKey      = "Playlist/ActiveTrackIndex"_L1;
constexpr auto LastPlaybackPosition     = "Player/LastPosition"_L1;
constexpr auto LastPlaybackTimeListened = "Player/LastTimeListened"_L1;
constexpr auto LastPlaybackState        = "Player/LastState"_L1;

namespace PicAudioEngine::PlaybackState {
void saveActivePlaylistDbId(int dbId)
{
    PicAudioStateSettings stateSettings;
    stateSettings.setValue(ActivePlaylistDbIdKey, dbId);
}

void clearActivePlaylistDbId()
{
    PicAudioStateSettings stateSettings;
    stateSettings.remove(ActivePlaylistDbIdKey);
}

std::optional<int> activePlaylistDbId()
{
    const PicAudioStateSettings stateSettings;
    if(stateSettings.contains(ActivePlaylistDbIdKey)) {
        return stateSettings.value(ActivePlaylistDbIdKey).toInt();
    }

    return {};
}

void saveActiveTrackIndex(int index)
{
    PicAudioStateSettings stateSettings;
    stateSettings.setValue(ActiveTrackIndexKey, index);
}

void clearActiveTrackIndex()
{
    PicAudioStateSettings stateSettings;
    stateSettings.remove(ActiveTrackIndexKey);
}

std::optional<int> activeTrackIndex()
{
    const PicAudioStateSettings stateSettings;
    if(stateSettings.contains(ActiveTrackIndexKey)) {
        return stateSettings.value(ActiveTrackIndexKey).toInt();
    }

    return {};
}

void savePlaybackPosition(uint64_t position)
{
    PicAudioStateSettings stateSettings;
    stateSettings.setValue(LastPlaybackPosition, QVariant::fromValue(position));
}

void clearPlaybackPosition()
{
    PicAudioStateSettings stateSettings;
    stateSettings.remove(LastPlaybackPosition);
}

std::optional<uint64_t> playbackPosition()
{
    const PicAudioStateSettings stateSettings;
    if(stateSettings.contains(LastPlaybackPosition)) {
        return stateSettings.value(LastPlaybackPosition).value<uint64_t>();
    }

    return {};
}

void savePlaybackTimeListened(uint64_t timeListenedMs)
{
    PicAudioStateSettings stateSettings;
    stateSettings.setValue(LastPlaybackTimeListened, QVariant::fromValue(timeListenedMs));
}

void clearPlaybackTimeListened()
{
    PicAudioStateSettings stateSettings;
    stateSettings.remove(LastPlaybackTimeListened);
}

std::optional<uint64_t> playbackTimeListened()
{
    const PicAudioStateSettings stateSettings;
    if(stateSettings.contains(LastPlaybackTimeListened)) {
        return stateSettings.value(LastPlaybackTimeListened).value<uint64_t>();
    }

    return {};
}

void savePlaybackState(Player::PlayState state)
{
    PicAudioStateSettings stateSettings;
    stateSettings.setValue(LastPlaybackState, Utils::Enum::toString(state));
}

void clearPlaybackState()
{
    PicAudioStateSettings stateSettings;
    stateSettings.remove(LastPlaybackState);
}

std::optional<Player::PlayState> playbackState()
{
    const PicAudioStateSettings stateSettings;
    if(stateSettings.contains(LastPlaybackState)) {
        return Utils::Enum::fromString<Player::PlayState>(stateSettings.value(LastPlaybackState).toString());
    }

    return {};
}
} // namespace PicAudioEngine::PlaybackState
