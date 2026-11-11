/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <core/engine/audioloader.h>
#include <core/playlist/playlist.h>

#include <memory>

namespace PicAudioEngine {
class PICAUDIO_ENGINE_EXPORT PlaylistNavigator
{
public:
    explicit PlaylistNavigator(std::shared_ptr<AudioLoader> audioLoader);

    PlaylistTrack currentTrack(Playlist* playlist) const;
    PlaylistTrack previewRelativeTrack(Playlist* playlist, Playlist::PlayModes mode, int delta) const;
    PlaylistTrack previewRelativeTrackFrom(Playlist* playlist, int currentIndex, Playlist::PlayModes mode,
                                           int delta) const;
    PlaylistTrack advanceRelativeTrack(Playlist* playlist, Playlist::PlayModes mode, int delta) const;

private:
    [[nodiscard]] PlaylistTrack populateTrackMetadata(Playlist* playlist, Track track, int index) const;

    std::shared_ptr<AudioLoader> m_audioLoader;
};
} // namespace PicAudioEngine
