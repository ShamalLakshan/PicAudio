/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <core/engine/audioloader.h>
#include <core/track.h>

#include <QDir>
#include <QString>

class QUrl;

namespace PicAudioEngine {
class PICAUDIO_ENGINE_EXPORT PlaylistParser
{
public:
    struct ReadPlaylistEntry
    {
        std::function<Track(const Track& filepath)> readTrack;
        std::function<bool(const Track& track)> canLoadTrack;
        bool cancel{false};
    };

    enum class PathType : uint8_t
    {
        Auto = 0,
        Absolute,
        Relative
    };

    virtual ~PlaylistParser() = default;

    [[nodiscard]] virtual QString name() const                    = 0;
    [[nodiscard]] virtual QStringList supportedExtensions() const = 0;
    [[nodiscard]] virtual bool saveIsSupported() const            = 0;
    [[nodiscard]] virtual bool canParse(const QByteArray& data, const QString& contentType, const QUrl& url) const;
    [[nodiscard]] virtual size_t countEntries(QIODevice* device, const QString& filepath, const QDir& dir) const;

    virtual TrackList readPlaylist(QIODevice* device, const QString& filepath, const QDir& dir,
                                   const ReadPlaylistEntry& readEntry, bool skipNotFound) = 0;
    virtual void savePlaylist(QIODevice* device, const QString& extension, const TrackList& tracks, const QDir& dir,
                              PathType type, bool writeMetdata);

    static QByteArray toUtf8(QIODevice* file);
    static QString determineTrackPath(const QUrl& url, const QDir& dir, PathType type);
};
} // namespace PicAudioEngine
