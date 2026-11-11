/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <QString>

namespace PicAudioEngine {
namespace PlaycountSettings {
constexpr auto ReadTag      = "Playcount/ReadTag";
constexpr auto WriteTag     = "Playcount/WriteTag";
constexpr auto ReadId3Popm  = "Playcount/ReadId3Popm";
constexpr auto WriteId3Popm = "Playcount/WriteId3Popm";

constexpr auto DefaultAutomatic    = "Automatic";
constexpr auto DefaultFmpsTag      = "FMPS_PLAYCOUNT";
constexpr auto DefaultReadId3Popm  = true;
constexpr auto DefaultWriteId3Popm = true;
} // namespace PlaycountSettings

struct PICAUDIO_ENGINE_EXPORT PlaycountTagPolicy
{
    QString readTag;
    QString writeTag;
    bool readId3Popm{true};
    bool writeId3Popm{true};

    [[nodiscard]] bool automaticRead() const;
    [[nodiscard]] bool shouldReadTag(const QString& tag, bool currentPlaycountSet) const;
    [[nodiscard]] QString effectiveWriteTag() const;
};

[[nodiscard]] PICAUDIO_ENGINE_EXPORT PlaycountTagPolicy playcountTagPolicy();
} // namespace PicAudioEngine
