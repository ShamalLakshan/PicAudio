/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include "playbackqueuestore.h"

#include <core/library/musiclibrary.h>
#include <core/playlist/playlisthandler.h>
#include <utils/database/dbconnectionprovider.h>

#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace PicAudioEngine {
namespace {
std::optional<PlaylistTrack> findPlaylistTrack(const Playlist& playlist, int trackId, int index)
{
    if(index >= 0) {
        if(const auto playlistTrack = playlist.playlistTrack(index);
           playlistTrack && playlistTrack->track.id() == trackId) {
            return playlistTrack;
        }
    }

    std::optional<PlaylistTrack> matchingTrack;

    const auto playlistTracks = playlist.playlistTracks();
    for(const PlaylistTrack& playlistTrack : playlistTracks) {
        if(playlistTrack.track.id() != trackId) {
            continue;
        }

        // If there are multiple matching tracks, leave entry detached rather than picking one arbitrarily
        if(matchingTrack.has_value()) {
            return {};
        }

        matchingTrack = playlistTrack;
    }

    return matchingTrack;
}
} // namespace

PlaybackQueueStore::PlaybackQueueStore(DbConnectionPoolPtr dbPool, MusicLibrary* library,
                                       PlaylistHandler* playlistHandler)
    : m_library{library}
    , m_playlistHandler{playlistHandler}
{
    const DbConnectionProvider dbProvider{std::move(dbPool)};
    m_database.initialise(dbProvider);
}

void PlaybackQueueStore::save(const PlaybackQueue& queue) const
{
    const auto& queueItems = queue.items();
    const int currentIndex = queue.currentIndex();

    std::vector<PlaybackQueueInfo> items;
    items.reserve(queueItems.size());

    for(int queueIndex{0}; const auto& queueItem : queueItems) {
        const auto& track = queueItem.track;
        if(!track.track.isValid() || !track.track.isInDatabase()) {
            ++queueIndex;
            continue;
        }

        PlaybackQueueInfo item;
        item.trackId     = track.track.id();
        item.origin      = static_cast<int>(queueItem.origin);
        item.sourceOrder = queueItem.sourceOrder;
        item.isCurrent   = queueIndex == currentIndex;

        if(track.playlistId.isValid()) {
            if(auto* playlist = m_playlistHandler->playlistById(track.playlistId);
               playlist && !playlist->isTemporary()) {
                item.playlistDbId       = playlist->dbId();
                item.playlistTrackIndex = track.indexInPlaylist;
            }
        }

        items.emplace_back(item);
        ++queueIndex;
    }

    m_database.replaceQueue(items);
}

PlaybackQueueSnapshot PlaybackQueueStore::load() const
{
    const auto savedQueue = m_database.queue();

    TrackIds trackIds;
    trackIds.reserve(savedQueue.size());

    std::unordered_set<int> uniqueTrackIds;
    uniqueTrackIds.reserve(savedQueue.size());

    for(const auto& item : savedQueue) {
        if(uniqueTrackIds.emplace(item.trackId).second) {
            trackIds.push_back(item.trackId);
        }
    }

    const TrackList tracks = m_library->tracksForIds(trackIds);

    std::unordered_map<int, const Track*> tracksById;
    tracksById.reserve(tracks.size());
    for(const Track& track : tracks) {
        tracksById.emplace(track.id(), &track);
    }

    PlaybackQueueSnapshot snapshot;
    snapshot.items.reserve(savedQueue.size());

    for(const auto& item : savedQueue) {
        const auto trackIt = tracksById.find(item.trackId);
        if(trackIt == tracksById.cend()) {
            continue;
        }

        PlaylistTrack queueTrack{.track = *trackIt->second, .playlistId = {}, .entryId = {}, .indexInPlaylist = -1};

        if(item.playlistDbId >= 0 && item.playlistTrackIndex >= 0) {
            if(auto* playlist = m_playlistHandler->playlistByDbId(item.playlistDbId)) {
                if(const auto playlistTrack = findPlaylistTrack(*playlist, item.trackId, item.playlistTrackIndex)) {
                    queueTrack.playlistId      = playlist->id();
                    queueTrack.entryId         = playlistTrack->entryId;
                    queueTrack.indexInPlaylist = playlistTrack->indexInPlaylist;
                }
            }
        }

        if(item.isCurrent) {
            snapshot.currentIndex = static_cast<int>(snapshot.items.size());
        }

        snapshot.items.push_back({.id          = 0,
                                  .track       = std::move(queueTrack),
                                  .origin      = static_cast<PlaybackQueueItemOrigin>(item.origin),
                                  .sourceOrder = item.sourceOrder});
    }
    return snapshot;
}
} // namespace PicAudioEngine
