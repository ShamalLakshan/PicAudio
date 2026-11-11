/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <core/player/playerdefs.h>

#include <cstdint>
#include <optional>

namespace PicAudioEngine::PlaybackState {
void saveActivePlaylistDbId(int dbId);
void clearActivePlaylistDbId();
std::optional<int> activePlaylistDbId();

void saveActiveTrackIndex(int index);
void clearActiveTrackIndex();
std::optional<int> activeTrackIndex();

void savePlaybackPosition(uint64_t position);
void clearPlaybackPosition();
std::optional<uint64_t> playbackPosition();

void savePlaybackTimeListened(uint64_t timeListenedMs);
void clearPlaybackTimeListened();
std::optional<uint64_t> playbackTimeListened();

void savePlaybackState(Player::PlayState state);
void clearPlaybackState();
std::optional<Player::PlayState> playbackState();
} // namespace PicAudioEngine::PlaybackState
