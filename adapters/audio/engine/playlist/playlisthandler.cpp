#include <core/playlist/playlisthandler.h>
#include <core/playlist/playlist.h>

namespace PicAudioEngine {

class PlaylistHandlerPrivate {
public:
    Playlist* activePlaylist{nullptr};
};

PlaylistHandler::PlaylistHandler(DbConnectionPoolPtr, std::shared_ptr<AudioLoader>,
                                 MusicLibrary*, SettingsManager*, QObject* parent)
    : QObject{parent}
    , p{std::make_unique<PlaylistHandlerPrivate>()}
{
}

PlaylistHandler::~PlaylistHandler() = default;

Playlist* PlaylistHandler::playlistById(const UId&) const
{
    return p->activePlaylist;
}

Playlist* PlaylistHandler::playlistByDbId(int) const
{
    return nullptr;
}

Playlist* PlaylistHandler::playlistByIndex(int) const
{
    return p->activePlaylist;
}

Playlist* PlaylistHandler::playlistByName(const QString&) const
{
    return p->activePlaylist;
}

PlaylistList PlaylistHandler::playlists() const
{
    if (p->activePlaylist) {
        return {p->activePlaylist};
    }
    return {};
}

PlaylistList PlaylistHandler::removedPlaylists() const
{
    return {};
}

PlaylistList PlaylistHandler::pendingRemovedPlaylists() const
{
    return {};
}

Playlist* PlaylistHandler::createEmptyPlaylist()
{
    return nullptr;
}

Playlist* PlaylistHandler::createTempEmptyPlaylist()
{
    return nullptr;
}

Playlist* PlaylistHandler::createPlaylist(const QString&)
{
    return nullptr;
}

Playlist* PlaylistHandler::createTempPlaylist(const QString&)
{
    return nullptr;
}

Playlist* PlaylistHandler::createPlaylist(const QString&, const TrackList&)
{
    return nullptr;
}

Playlist* PlaylistHandler::createTempPlaylist(const QString&, const TrackList&)
{
    return nullptr;
}

Playlist* PlaylistHandler::createNewPlaylist(const QString&)
{
    return nullptr;
}

Playlist* PlaylistHandler::createNewTempPlaylist(const QString&)
{
    return nullptr;
}

Playlist* PlaylistHandler::createNewPlaylist(const QString&, const TrackList&)
{
    return nullptr;
}

Playlist* PlaylistHandler::createNewTempPlaylist(const QString&, const TrackList&)
{
    return nullptr;
}

Playlist* PlaylistHandler::createAutoPlaylist(const QString&, const QString&, const QString&, bool)
{
    return nullptr;
}

Playlist* PlaylistHandler::createNewAutoPlaylist(const QString&, const QString&, const QString&, bool)
{
    return nullptr;
}

void PlaylistHandler::appendToPlaylist(const UId&, const TrackList&)
{
}

void PlaylistHandler::replacePlaylistTracks(const UId&, const TrackList&, PlaylistTrackChangeSource)
{
}

void PlaylistHandler::replacePlaylistTracks(const UId&, const PlaylistTrackList&, PlaylistTrackChangeSource)
{
}

void PlaylistHandler::movePlaylistTracks(const UId&, const UId&)
{
}

void PlaylistHandler::removePlaylistTracks(const UId&, const std::vector<int>&)
{
}

void PlaylistHandler::clearPlaylistTracks(const UId&)
{
}

std::vector<int> PlaylistHandler::duplicateTrackIndexes(const UId&) const
{
    return {};
}

std::vector<int> PlaylistHandler::deadTrackIndexes(const UId&) const
{
    return {};
}

void PlaylistHandler::handleTracksDeleted(const TrackList&)
{
}

void PlaylistHandler::changePlaylistIndex(const UId&, int)
{
}

void PlaylistHandler::changeActivePlaylist(const UId&)
{
}

void PlaylistHandler::changeActivePlaylist(Playlist* playlist)
{
    p->activePlaylist = playlist;
}

bool PlaylistHandler::sortPlaylistsByName(Qt::SortOrder)
{
    return true;
}

PlaylistTrack PlaylistHandler::currentTrack() const
{
    if (p->activePlaylist) {
        if (auto t = p->activePlaylist->playlistTrack(p->activePlaylist->currentTrackIndex())) {
            return *t;
        }
    }
    return {};
}

PlaylistTrack PlaylistHandler::peekRelativeTrack(Playlist::PlayModes, int delta) const
{
    if (p->activePlaylist) {
        int idx = p->activePlaylist->currentTrackIndex() + delta;
        if (auto t = p->activePlaylist->playlistTrack(idx)) {
            return *t;
        }
    }
    return {};
}

PlaylistTrack PlaylistHandler::advanceRelativeTrack(Playlist::PlayModes mode, int delta)
{
    auto tr = peekRelativeTrack(mode, delta);
    if (p->activePlaylist) {
        p->activePlaylist->changeCurrentIndex(p->activePlaylist->currentTrackIndex() + delta);
    }
    return tr;
}

void PlaylistHandler::renamePlaylist(const UId&, const QString&)
{
}

void PlaylistHandler::setPlaylistLocked(const UId&, bool)
{
}

void PlaylistHandler::removePlaylist(const UId&)
{
}

Playlist* PlaylistHandler::restorePlaylist(const UId&)
{
    return nullptr;
}

void PlaylistHandler::purgeRemovedPlaylists(const std::vector<UId>&)
{
}

void PlaylistHandler::ensurePlaylistItemVisible(const UId&, int)
{
}

Playlist* PlaylistHandler::activePlaylist() const
{
    return p->activePlaylist;
}

int PlaylistHandler::playlistCount() const
{
    return p->activePlaylist ? 1 : 0;
}

void PlaylistHandler::prepareUpcomingTrack()
{
}

void PlaylistHandler::savePlaylists()
{
}

void PlaylistHandler::savePlaylist(const UId&)
{
}

void PlaylistHandler::finishPendingUpdates()
{
}

} // namespace PicAudioEngine
