/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <core/engine/audioloader.h>
#include <core/playlist/playlist.h>
#include <core/playlist/playlistchangeset.h>
#include <core/track.h>
#include <memory>

#include <QObject>

namespace PicAudioEngine {
class DbConnectionPool;
using DbConnectionPoolPtr = std::shared_ptr<DbConnectionPool>;
class MusicLibrary;
class PlaylistHandlerPrivate;
class SettingsManager;

enum class PlaylistTrackChangeSource
{
    External = 0,
    History,
};

class PICAUDIO_ENGINE_EXPORT PlaylistHandler : public QObject
{
    Q_OBJECT

public:
    explicit PlaylistHandler(DbConnectionPoolPtr dbPool, std::shared_ptr<AudioLoader> audioLoader,
                             MusicLibrary* library, SettingsManager* settings, QObject* parent = nullptr);
    ~PlaylistHandler() override;

    /** Returns the playlist with the @p id if it exists, otherwise nullptr. */
    [[nodiscard]] Playlist* playlistById(const UId& id) const;
    /** Returns the playlist with the db @p id if it exists, otherwise nullptr. */
    [[nodiscard]] Playlist* playlistByDbId(int id) const;
    /** Returns the playlist with the @p index if it exists, otherwise nullptr. */
    [[nodiscard]] Playlist* playlistByIndex(int index) const;
    /** Returns the playlist with the @p name if it exists, otherwise nullptr. */
    [[nodiscard]] Playlist* playlistByName(const QString& name) const;

    [[nodiscard]] PlaylistList playlists() const;
    /** Returns removed playlists retained for restoration until purged. */
    [[nodiscard]] PlaylistList removedPlaylists() const;
    /** Returns removed playlists still pending export or deletion at shutdown. */
    [[nodiscard]] PlaylistList pendingRemovedPlaylists() const;

    /** Creates and returns an empty playlist with a default name. */
    Playlist* createEmptyPlaylist();
    /** Creates and returns an empty hidden playlist with a default name. */
    Playlist* createTempEmptyPlaylist();
    /** Returns the playlist called @p name if it exists, otherwise creates it. */
    Playlist* createPlaylist(const QString& name);
    /** Returns the temporary playlist called @p name if it exists, otherwise creates it. */
    Playlist* createTempPlaylist(const QString& name);
    /** Returns the playlist called @p name and replaces it's tracks if it exists, otherwise creates it. */
    Playlist* createPlaylist(const QString& name, const TrackList& tracks);
    /** Returns the temporary playlist called @p name and replaces it's tracks if it exists, otherwise creates it. */
    Playlist* createTempPlaylist(const QString& name, const TrackList& tracks);
    /** Creates and returns a playlist called @p name. If it already exists, the name will be changed. */
    Playlist* createNewPlaylist(const QString& name);
    /** Creates and returns a temporary playlist called @p name. If it already exists, the name will be changed. */
    Playlist* createNewTempPlaylist(const QString& name);
    /** Creates and returns a playlist called @p name with the provided tracks. If it already exists, the name will be
     * changed. */
    Playlist* createNewPlaylist(const QString& name, const TrackList& tracks);
    /** Creates and returns a temporary playlist called @p name with the provided tracks. If it already exists, the name
     * will be changed. */
    Playlist* createNewTempPlaylist(const QString& name, const TrackList& tracks);
    /** Returns the autoplaylist called @p name if it exists, otherwise creates it. */
    Playlist* createAutoPlaylist(const QString& name, const QString& query, const QString& sortQuery = {},
                                 bool forceSorted = true);
    /** Creates and returns the autoplaylist called @p name. If it already exists, the name will be changed. */
    Playlist* createNewAutoPlaylist(const QString& name, const QString& query, const QString& sortQuery = {},
                                    bool forceSorted = true);

    /** Adds @p tracks to the end of the writable playlist with @p id if found. */
    void appendToPlaylist(const UId& id, const TrackList& tracks);
    /** Replaces the @p tracks of the playlist with @p id if found. */
    void replacePlaylistTracks(const UId& id, const TrackList& tracks,
                               PlaylistTrackChangeSource source = PlaylistTrackChangeSource::External);
    /** Replaces the @p tracks of the playlist with @p id if found, preserving playlist entry identity. */
    void replacePlaylistTracks(const UId& id, const PlaylistTrackList& tracks,
                               PlaylistTrackChangeSource source = PlaylistTrackChangeSource::External);
    /** Moves the tracks of the playlist with @p id to the playlist with @p replaceId. */
    void movePlaylistTracks(const UId& id, const UId& replaceId);
    /** Removes the tracks at @p indexes of the playlist with @p id if found. */
    void removePlaylistTracks(const UId& id, const std::vector<int>& indexes);
    /** Clears all tracks of the playlist with @p id if found. */
    void clearPlaylistTracks(const UId& id);
    /** Returns the indexes of all duplicate tracks in the playlist with @p id. */
    [[nodiscard]] std::vector<int> duplicateTrackIndexes(const UId& id) const;
    /** Returns the indexes of all dead tracks in the playlist with @p id. */
    [[nodiscard]] std::vector<int> deadTrackIndexes(const UId& id) const;
    /** Removes any playlist entries that match the deleted @p tracks. */
    void handleTracksDeleted(const TrackList& tracks);

    void changePlaylistIndex(const UId& id, int index);
    void changeActivePlaylist(const UId& id);
    void changeActivePlaylist(Playlist* playlist);

    bool sortPlaylistsByName(Qt::SortOrder order = Qt::AscendingOrder);

    /** Returns the current track in the active playlist with metadata populated if needed. */
    [[nodiscard]] PlaylistTrack currentTrack() const;
    /** Returns a preview track relative to the current index in the active playlist. */
    [[nodiscard]] PlaylistTrack peekRelativeTrack(Playlist::PlayModes mode, int delta = 1) const;
    /** Advances and returns a track relative to the current index in the active playlist. */
    [[nodiscard]] PlaylistTrack advanceRelativeTrack(Playlist::PlayModes mode, int delta = 1);

    void renamePlaylist(const UId& id, const QString& name);
    /** Enables or disables changes to the contents of the playlist with @p id. */
    void setPlaylistLocked(const UId& id, bool locked);
    void removePlaylist(const UId& id);
    Playlist* restorePlaylist(const UId& id);
    /** Discards restoration history while preserving pending export or deletion at shutdown. */
    void purgeRemovedPlaylists(const std::vector<UId>& ids);

    void ensurePlaylistItemVisible(const UId& id, int index);

    /** Returns the playlist currently being played (nullptr if not playing) */
    [[nodiscard]] Playlist* activePlaylist() const;

    [[nodiscard]] int playlistCount() const;
    /** Prime upcoming track state/metadata for seamless transition preparation. */
    void prepareUpcomingTrack();

    void savePlaylists();
    void savePlaylist(const UId& id);
    /** Completes pending autoplaylist updates and stops scheduling workers before shutdown. */
    void finishPendingUpdates();

Q_SIGNALS:
    void playlistItemEnsureVisible(PicAudioEngine::Playlist* playlist, int index);
    void playlistsPopulated();
    void playlistAdded(PicAudioEngine::Playlist* playlist);
    void playlistRemoved(PicAudioEngine::Playlist* playlist);
    void playlistRenamed(PicAudioEngine::Playlist* playlist);
    void playlistUpdated(PicAudioEngine::Playlist* playlist);
    void playlistIndexChanged(PicAudioEngine::Playlist* playlist);
    void activePlaylistChanged(PicAudioEngine::Playlist* playlist);
    void activePlaylistDeleted();
    void playlistReferencesRemapRequested(const PicAudioEngine::UId& fromPlaylistId, const PicAudioEngine::UId& toPlaylistId);

    void tracksAdded(PicAudioEngine::Playlist* playlist, const PicAudioEngine::TrackList& tracks, int index);
    void tracksPatched(PicAudioEngine::Playlist* playlist, const PicAudioEngine::PlaylistChangeset& changeSet,
                       PicAudioEngine::PlaylistTrackChangeSource source);
    void tracksChanged(PicAudioEngine::Playlist* playlist, const std::vector<int>& indexes,
                       PicAudioEngine::PlaylistTrackChangeSource source);
    void tracksUpdated(PicAudioEngine::Playlist* playlist, const std::vector<int>& indexes);
    void tracksRemoved(PicAudioEngine::Playlist* playlist, const std::vector<int>& indexes);

private:
    std::unique_ptr<PlaylistHandlerPrivate> p;
};
} // namespace PicAudioEngine

Q_DECLARE_METATYPE(PicAudioEngine::PlaylistTrackChangeSource)
