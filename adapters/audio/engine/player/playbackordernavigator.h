/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <core/player/playbackqueue.h>
#include <core/playlist/playlist.h>

#include <optional>

namespace PicAudioEngine {
class PlaybackQueue;
class PlaylistHandler;
class SettingsManager;

class PlaybackOrderNavigator
{
public:
    struct PlaybackOrderState
    {
        PlaylistTrack* currentTrack{nullptr};
        Playlist::PlayModes* playMode{nullptr};
        bool* isQueueTrack{nullptr};
        PlaylistTrack* scheduledTrack{nullptr};
    };

    enum class RestartTarget : uint8_t
    {
        None = 0,
        Beginning,
        End,
    };

    struct RequestedTrack
    {
        PlaylistTrack track;
        bool isQueueTrack{false};
        PlaybackQueueItemId queueItemId{0};
    };

    PlaybackOrderNavigator(SettingsManager* settings, PlaylistHandler* playlistHandler, PlaybackQueue* queue,
                           PlaybackOrderState state);

    [[nodiscard]] Playlist* playbackPlaylist() const;
    [[nodiscard]] const PlaybackQueueItem* sequenceRelativeItem(int delta) const;
    [[nodiscard]] PlaylistTrack previewPlaybackRelativeTrack(int delta) const;
    std::optional<RequestedTrack> selectScheduledTrack();
    std::optional<RequestedTrack> selectPlaybackOrderTrack(int delta);
    std::optional<RequestedTrack> selectPlayFromIdleState(bool preferScheduledTrack);
    PlaylistTrack restartPlaylist(RestartTarget target);

private:
    [[nodiscard]] bool queueIsPlaybackSource() const;
    PlaylistTrack advancePlaybackRelativeTrack(int delta);
    [[nodiscard]] Playlist* playlistForTrack(const PlaylistTrack& track) const;
    PlaylistTrack restartPlaylistFromBeginning();
    PlaylistTrack restartPlaylistFromEnd();

    void activatePlaylistTrack(const PlaylistTrack& track);
    [[nodiscard]] std::optional<PlaylistTrack> followQueuedTrackIndex(int delta) const;

    SettingsManager* m_settings;
    PlaylistHandler* m_playlistHandler;
    PlaybackQueue* m_queue;
    PlaybackOrderState m_state;
};
} // namespace PicAudioEngine
