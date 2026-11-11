/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <core/engine/enginedefs.h>
#include <core/player/playbackqueue.h>
#include <core/playlist/playlist.h>
#include <core/track.h>
#include <utils/id.h>

#include <optional>

namespace PicAudioEngine::Player {
Q_NAMESPACE_EXPORT(PICAUDIO_ENGINE_EXPORT)

enum class PlayState : uint8_t
{
    Playing = 0,
    Paused,
    Stopped,
};
Q_ENUM_NS(PlayState)

enum class AdvanceReason : uint8_t
{
    Unknown = 0,
    StartupRestore,
    ManualSelection,
    ManualNext,
    ManualPrevious,
    NaturalEnd,
    PreparedCrossfade,
    PreparedCommit,
    PreparedCrossfadeCommit,
};
Q_ENUM_NS(AdvanceReason)

struct PICAUDIO_ENGINE_EXPORT ABRepeatRange
{
    std::optional<uint64_t> start;
    std::optional<uint64_t> end;
    bool enabled{false};

    bool operator==(const ABRepeatRange&) const = default;
};

struct PICAUDIO_ENGINE_EXPORT TrackChangeContext
{
    AdvanceReason reason{AdvanceReason::Unknown};
    bool userInitiated{false};
};

struct PICAUDIO_ENGINE_EXPORT TrackChangeRequest
{
    PlaylistTrack track;
    TrackChangeContext context;
    bool isQueueTrack{false};
    PlaybackQueueItemId queueItemId{0};
    uint64_t itemId{0};
};

struct PICAUDIO_ENGINE_EXPORT UpcomingTrack
{
    PlaylistTrack track;
    bool isQueueTrack{false};
    PlaybackQueueItemId queueItemId{0};
    uint64_t itemId{0};
};

struct PICAUDIO_ENGINE_EXPORT PlaybackSnapshot
{
    PlayState playState{PlayState::Stopped};
    Track track;
    UId playlistId;
    int indexInPlaylist{-1};
    uint64_t positionMs{0};
    uint64_t durationMs{0};
    int bitrate{0};
    QString decoder;
    Engine::PlaybackOutputInfo outputInfo;
    bool isQueueTrack{false};
    PlaybackQueueItemId queueItemId{0};
};
} // namespace PicAudioEngine::Player

Q_DECLARE_METATYPE(PicAudioEngine::Player::AdvanceReason)
Q_DECLARE_METATYPE(PicAudioEngine::Player::TrackChangeContext)
Q_DECLARE_METATYPE(PicAudioEngine::Player::TrackChangeRequest)
Q_DECLARE_METATYPE(PicAudioEngine::Player::UpcomingTrack)
Q_DECLARE_METATYPE(PicAudioEngine::Player::PlaybackSnapshot)
