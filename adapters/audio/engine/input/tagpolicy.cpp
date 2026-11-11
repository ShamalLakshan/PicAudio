/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include "tagpolicy.h"

#include "internalcoresettings.h"

#include <core/coresettings.h>

namespace PicAudioEngine {
namespace {
template <typename Enum>
Enum enumSetting(const PicAudioSettings& settings, auto key, Enum defaultValue, Enum min, Enum max)
{
    const int value = settings.value(key, static_cast<int>(defaultValue)).toInt();

    if(value < static_cast<int>(min) || value > static_cast<int>(max)) {
        return defaultValue;
    }

    return static_cast<Enum>(value);
}
} // namespace

TagPolicy tagPolicy()
{
    const PicAudioSettings settings;

    const auto id3v2Version = enumSetting(settings, Settings::Core::Internal::Id3v2WriteVersion, Id3v2WriteVersion::V4,
                                          Id3v2WriteVersion::V3, Id3v2WriteVersion::V4);

    const auto mp3TagWritingScheme
        = enumSetting(settings, Settings::Core::Internal::Mp3TagWritingScheme, Mp3TagWritingScheme::Id3v2AndId3v1,
                      Mp3TagWritingScheme::Id3v2AndId3v1, Mp3TagWritingScheme::Ape);

    return {
        .rating    = ratingTagPolicy(),
        .playcount = playcountTagPolicy(),
        .splitId3v23SemicolonSeparatedTags
        = settings.value(Settings::Core::Internal::SplitId3v23SemicolonSeparatedTags, true).toBool(),
        .id3v2WriteVersion   = id3v2Version,
        .mp3TagWritingScheme = mp3TagWritingScheme,
    };
}
} // namespace PicAudioEngine
