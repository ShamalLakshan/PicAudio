/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "core/database/playbackqueuedatabase.h"

#include <core/player/playbackqueue.h>
#include <utils/database/dbconnectionpool.h>

namespace PicAudioEngine {
class MusicLibrary;
class PlaylistHandler;

class PlaybackQueueStore
{
public:
    PlaybackQueueStore(DbConnectionPoolPtr dbPool, MusicLibrary* library, PlaylistHandler* playlistHandler);

    void save(const PlaybackQueue& queue) const;
    [[nodiscard]] PlaybackQueueSnapshot load() const;

private:
    MusicLibrary* m_library;
    PlaylistHandler* m_playlistHandler;
    PlaybackQueueDatabase m_database;
};
} // namespace PicAudioEngine
