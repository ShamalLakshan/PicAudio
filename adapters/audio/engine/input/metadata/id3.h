/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <QByteArrayView>
#include <QString>
#include <QStringList>

#include <optional>

namespace PicAudioEngine::Metadata {
struct TimedId3Metadata
{
    QString title;
    QString artist;
    QString station;
};

[[nodiscard]] PICAUDIO_ENGINE_EXPORT std::optional<TimedId3Metadata> parseTimedId3Metadata(QByteArrayView data);
[[nodiscard]] PICAUDIO_ENGINE_EXPORT QStringList splitId3StandardField(const QString& field, const QStringList& values,
                                                              bool splitSemicolonSeparated);
[[nodiscard]] PICAUDIO_ENGINE_EXPORT QStringList splitId3ExtraField(const QString& field, const QStringList& values,
                                                           bool splitSemicolonSeparated);
} // namespace PicAudioEngine::Metadata
