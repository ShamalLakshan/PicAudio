/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <core/engine/conversion/conversiondefs.h>

#include <QCoreApplication>
#include <QString>

#include <vector>

namespace PicAudioEngine {
enum class ConversionPathStatus : uint8_t
{
    Ready = 0,
    Skipped,
    NeedsOverwriteDecision,
    Error,
};

struct ConversionPathResult
{
    Track track;
    TrackList tracks;
    QString outputPath;
    ConversionPathStatus status{ConversionPathStatus::Error};
    QString error;
};

class PICAUDIO_ENGINE_EXPORT ConversionPathResolver
{
    Q_DECLARE_TR_FUNCTIONS(PicAudioEngine::ConversionPathResolver)

public:
    struct Request
    {
        TrackList tracks;
        ConversionDestination destination;
        QString extension;
        QString askFolder;
    };

    static std::vector<ConversionPathResult> resolve(const Request& request);
    static QString previewPath(const Track& track, const ConversionDestination& destination, const QString& extension,
                               const QString& askFolder = {});
};
} // namespace PicAudioEngine
