/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include "playcounttagpolicy.h"

#include "ratingtagpolicy.h"

#include <core/coresettings.h>

using namespace Qt::StringLiterals;

namespace PicAudioEngine {
bool PlaycountTagPolicy::automaticRead() const
{
    return readTag.isEmpty() || readTag == "AUTOMATIC"_L1;
}

bool PlaycountTagPolicy::shouldReadTag(const QString& tag, bool currentPlaycountSet) const
{
    if(automaticRead()) {
        return tag == "FMPS_PLAYCOUNT"_L1 || (tag == "PLAYCOUNT"_L1 && !currentPlaycountSet);
    }
    return tag == readTag;
}

QString PlaycountTagPolicy::effectiveWriteTag() const
{
    return writeTag == "DONOTWRITE"_L1 ? QString{} : writeTag;
}

PlaycountTagPolicy playcountTagPolicy()
{
    const PicAudioSettings settings;
    const bool readId3Popm
        = settings.contains(PlaycountSettings::ReadId3Popm)
            ? settings.value(PlaycountSettings::ReadId3Popm, PlaycountSettings::DefaultReadId3Popm).toBool()
            : settings.value(RatingSettings::ReadId3Popm, RatingSettings::DefaultReadId3Popm).toBool();
    const bool writeId3Popm
        = settings.value(PlaycountSettings::WriteId3Popm, PlaycountSettings::DefaultWriteId3Popm).toBool();

    return {
        .readTag  = settings.value(PlaycountSettings::ReadTag, QLatin1StringView{PlaycountSettings::DefaultAutomatic})
                        .toString()
                        .trimmed()
                        .toUpper(),
        .writeTag = settings.value(PlaycountSettings::WriteTag, QLatin1StringView{PlaycountSettings::DefaultFmpsTag})
                        .toString()
                        .trimmed()
                        .toUpper(),
        .readId3Popm  = readId3Popm,
        .writeId3Popm = writeId3Popm,
    };
}
} // namespace PicAudioEngine
