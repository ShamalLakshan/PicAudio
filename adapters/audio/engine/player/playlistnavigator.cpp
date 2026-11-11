/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include "playlistnavigator.h"

namespace PicAudioEngine {
PlaylistNavigator::PlaylistNavigator(std::shared_ptr<AudioLoader> audioLoader)
    : m_audioLoader{std::move(audioLoader)}
{ }

PlaylistTrack PlaylistNavigator::currentTrack(Playlist* playlist) const
{
    if(!playlist) {
        return {};
    }

    return populateTrackMetadata(playlist, playlist->currentTrack(), playlist->currentTrackIndex());
}

PlaylistTrack PlaylistNavigator::previewRelativeTrack(Playlist* playlist, Playlist::PlayModes mode, int delta) const
{
    if(!playlist) {
        return {};
    }

    const int index = playlist->nextIndex(delta, mode);
    if(const auto track = playlist->track(index)) {
        return populateTrackMetadata(playlist, track.value(), index);
    }

    return {};
}

PlaylistTrack PlaylistNavigator::previewRelativeTrackFrom(Playlist* playlist, int currentIndex,
                                                          Playlist::PlayModes mode, int delta) const
{
    if(!playlist || currentIndex < 0) {
        return {};
    }

    const int index = playlist->nextIndexFrom(currentIndex, delta, mode);
    if(const auto track = playlist->track(index)) {
        return populateTrackMetadata(playlist, track.value(), index);
    }

    return {};
}

PlaylistTrack PlaylistNavigator::advanceRelativeTrack(Playlist* playlist, Playlist::PlayModes mode, int delta) const
{
    if(!playlist) {
        return {};
    }

    Track track = playlist->nextTrackChange(delta, mode);
    if(!track.isValid()) {
        return {};
    }

    return populateTrackMetadata(playlist, std::move(track), playlist->currentTrackIndex());
}

PlaylistTrack PlaylistNavigator::populateTrackMetadata(Playlist* playlist, Track track, int index) const
{
    if(!playlist || !track.isValid() || index < 0) {
        return {};
    }

    if(!track.isRemote() && !track.metadataWasRead() && m_audioLoader && m_audioLoader->readTrackMetadata(track)) {
        track.generateHash();
        playlist->updateTrackAtIndex(index, track);
    }

    if(const auto playlistTrack = playlist->playlistTrack(index)) {
        auto result{*playlistTrack};
        result.track = track;
        return result;
    }

    return {.track = track, .playlistId = playlist->id(), .entryId = {}, .indexInPlaylist = index};
}
} // namespace PicAudioEngine
