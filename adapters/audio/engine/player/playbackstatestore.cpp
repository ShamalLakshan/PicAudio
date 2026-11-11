/*
 * Fooyin
 * Copyright © 2026, Luke Taylor <luket@pm.me>
 *
 * Fooyin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Fooyin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Fooyin.  If not, see <http://www.gnu.org/licenses/>.
 *
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

namespace Fooyin::PlaybackState {
void saveActivePlaylistDbId(int dbId)
{
    FyStateSettings stateSettings;
    stateSettings.setValue(ActivePlaylistDbIdKey, dbId);
}

void clearActivePlaylistDbId()
{
    FyStateSettings stateSettings;
    stateSettings.remove(ActivePlaylistDbIdKey);
}

std::optional<int> activePlaylistDbId()
{
    const FyStateSettings stateSettings;
    if(stateSettings.contains(ActivePlaylistDbIdKey)) {
        return stateSettings.value(ActivePlaylistDbIdKey).toInt();
    }

    return {};
}

void saveActiveTrackIndex(int index)
{
    FyStateSettings stateSettings;
    stateSettings.setValue(ActiveTrackIndexKey, index);
}

void clearActiveTrackIndex()
{
    FyStateSettings stateSettings;
    stateSettings.remove(ActiveTrackIndexKey);
}

std::optional<int> activeTrackIndex()
{
    const FyStateSettings stateSettings;
    if(stateSettings.contains(ActiveTrackIndexKey)) {
        return stateSettings.value(ActiveTrackIndexKey).toInt();
    }

    return {};
}

void savePlaybackPosition(uint64_t position)
{
    FyStateSettings stateSettings;
    stateSettings.setValue(LastPlaybackPosition, QVariant::fromValue(position));
}

void clearPlaybackPosition()
{
    FyStateSettings stateSettings;
    stateSettings.remove(LastPlaybackPosition);
}

std::optional<uint64_t> playbackPosition()
{
    const FyStateSettings stateSettings;
    if(stateSettings.contains(LastPlaybackPosition)) {
        return stateSettings.value(LastPlaybackPosition).value<uint64_t>();
    }

    return {};
}

void savePlaybackTimeListened(uint64_t timeListenedMs)
{
    FyStateSettings stateSettings;
    stateSettings.setValue(LastPlaybackTimeListened, QVariant::fromValue(timeListenedMs));
}

void clearPlaybackTimeListened()
{
    FyStateSettings stateSettings;
    stateSettings.remove(LastPlaybackTimeListened);
}

std::optional<uint64_t> playbackTimeListened()
{
    const FyStateSettings stateSettings;
    if(stateSettings.contains(LastPlaybackTimeListened)) {
        return stateSettings.value(LastPlaybackTimeListened).value<uint64_t>();
    }

    return {};
}

void savePlaybackState(Player::PlayState state)
{
    FyStateSettings stateSettings;
    stateSettings.setValue(LastPlaybackState, Utils::Enum::toString(state));
}

void clearPlaybackState()
{
    FyStateSettings stateSettings;
    stateSettings.remove(LastPlaybackState);
}

std::optional<Player::PlayState> playbackState()
{
    const FyStateSettings stateSettings;
    if(stateSettings.contains(LastPlaybackState)) {
        return Utils::Enum::fromString<Player::PlayState>(stateSettings.value(LastPlaybackState).toString());
    }

    return {};
}
} // namespace Fooyin::PlaybackState
