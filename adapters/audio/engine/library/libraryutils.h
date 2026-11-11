/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <core/track.h>

#include <QString>

#include <cstdint>
#include <optional>
#include <vector>

namespace PicAudioEngine::Utils {
enum class PICAUDIO_ENGINE_EXPORT CommonOperation : uint8_t
{
    Update = 0,
    Remove,
    UpdateChanged
};

PICAUDIO_ENGINE_EXPORT std::vector<int> updateCommonTracks(TrackList& tracks, const TrackList& updatedTracks,
                                                  CommonOperation operation);
PICAUDIO_ENGINE_EXPORT std::optional<QString> physicalSourceKey(const Track& track);
} // namespace PicAudioEngine::Utils
