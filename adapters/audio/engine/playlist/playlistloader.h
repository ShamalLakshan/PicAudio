/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <core/playlist/playlistparser.h>

#include <memory>
#include <shared_mutex>

#include <QStringList>

class QUrl;

namespace PicAudioEngine {
class PICAUDIO_ENGINE_EXPORT PlaylistLoader
{
public:
    PlaylistParser* addParser(std::unique_ptr<PlaylistParser> parser);

    [[nodiscard]] QStringList supportedExtensions() const;
    [[nodiscard]] QStringList supportedSaveExtensions() const;
    [[nodiscard]] PlaylistParser* parserForExtension(const QString& extension) const;
    [[nodiscard]] PlaylistParser* parserForData(const QByteArray& data, const QString& contentType,
                                                const QUrl& url) const;
    [[nodiscard]] QStringList resolveUrls(const QUrl& baseUrl, const QByteArray& data,
                                          const QString& contentType = {}) const;

private:
    std::unordered_map<QString, std::unique_ptr<PlaylistParser>> m_parsers;
    mutable std::shared_mutex m_parserMutex;
};
} // namespace PicAudioEngine
