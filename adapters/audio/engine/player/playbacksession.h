/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <core/player/playerdefs.h>

#include <optional>

namespace PicAudioEngine {
class PICAUDIO_ENGINE_EXPORT PlaybackSession
{
public:
    enum class ScheduledTrackKind
    {
        Normal = 0,
        StopAfterCurrentResume,
    };

    PlaybackSession();

    PlaylistTrack* currentTrackPtr();
    PlaylistTrack* scheduledTrackPtr();
    bool* isQueueTrackPtr();

    [[nodiscard]] const PlaylistTrack& currentTrack() const;
    PlaylistTrack& currentTrack();
    [[nodiscard]] const PlaylistTrack& scheduledTrack() const;
    [[nodiscard]] ScheduledTrackKind scheduledTrackKind() const;
    [[nodiscard]] const std::optional<PlaylistTrack>& detachedCurrentPlaylistTrack() const;
    [[nodiscard]] bool hasCurrentTrack() const;
    [[nodiscard]] bool isQueueTrack() const;
    [[nodiscard]] PlaybackQueueItemId currentQueueItemId() const;
    [[nodiscard]] uint64_t currentItemId() const;
    [[nodiscard]] bool isIdle() const;

    [[nodiscard]] const Player::TrackChangeContext& pendingChangeContext() const;
    Player::TrackChangeContext& pendingChangeContext();
    [[nodiscard]] const Player::TrackChangeContext& lastChangeContext() const;
    [[nodiscard]] bool hasPendingRequest() const;
    [[nodiscard]] bool pendingRequestMatchesCurrentTrack() const;
    [[nodiscard]] bool canAcceptRequest() const;

    [[nodiscard]] Player::TrackChangeRequest withPlaybackItemId(Player::TrackChangeRequest request,
                                                                uint64_t nextItemId) const;
    Player::TrackChangeRequest requestTrackChange(Player::TrackChangeRequest request);
    bool rejectPendingRequest(uint64_t itemId);

    struct CommitResult
    {
        bool isQueueTrack{false};
        PlaybackQueueItemId queueItemId{0};
        bool matchedPendingRequest{false};
    };
    CommitResult commitRequest(const Player::TrackChangeRequest& request);

    void clearPendingRequest();
    void clearCurrentTrack();
    void resetCurrentTrackState();
    void scheduleTrack(const PlaylistTrack& track, ScheduledTrackKind kind = ScheduledTrackKind::Normal);
    void clearScheduledTrack();
    void setDetachedCurrentPlaylistTrack(const PlaylistTrack& track);
    void clearDetachedCurrentPlaylistTrack();

    bool updateCurrentTrack(const Track& track);
    bool updateCurrentTrackPlaylist(const UId& playlistId);
    bool updateCurrentTrackEntry(const UId& entryId);
    bool updateCurrentTrackIndex(int index);

private:
    PlaylistTrack m_currentTrack;
    PlaylistTrack m_scheduledTrack;
    ScheduledTrackKind m_scheduledTrackKind;
    bool m_isQueueTrack;
    PlaybackQueueItemId m_currentQueueItemId;
    uint64_t m_currentItemId;
    Player::TrackChangeContext m_pendingChangeContext;
    Player::TrackChangeContext m_lastChangeContext;
    std::optional<Player::TrackChangeRequest> m_pendingRequest;
    std::optional<PlaylistTrack> m_detachedCurrentPlaylistTrack;
};
} // namespace PicAudioEngine
