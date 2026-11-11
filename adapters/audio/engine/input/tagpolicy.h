/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "playcounttagpolicy.h"
#include "ratingtagpolicy.h"

#include "fycore_export.h"
#include "internalcoresettings.h"

namespace PicAudioEngine {
struct PICAUDIO_ENGINE_EXPORT TagPolicy
{
    RatingTagPolicy rating;
    PlaycountTagPolicy playcount;
    bool splitId3v23SemicolonSeparatedTags{false};
    Id3v2WriteVersion id3v2WriteVersion{Id3v2WriteVersion::V4};
    Mp3TagWritingScheme mp3TagWritingScheme{Mp3TagWritingScheme::Id3v2AndId3v1};
};

[[nodiscard]] PICAUDIO_ENGINE_EXPORT TagPolicy tagPolicy();
} // namespace PicAudioEngine
