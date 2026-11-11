/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fyutils_export.h"

#include <QDir>
#include <QFile>
#include <QStringList>
#include <QUrl>

#include <stop_token>

namespace PicAudioEngine::Utils::File {
PICAUDIO_UTILS_EXPORT bool moveToTrash(const QString& path);
PICAUDIO_UTILS_EXPORT QString cleanPath(const QString& path);
PICAUDIO_UTILS_EXPORT bool isSamePath(const QString& filename1, const QString& filename2);
PICAUDIO_UTILS_EXPORT bool isSubdir(const QString& dir, const QString& parentDir);
PICAUDIO_UTILS_EXPORT QString getParentDirectory(const QString& filename);

PICAUDIO_UTILS_EXPORT bool createDirectories(const QString& path);
PICAUDIO_UTILS_EXPORT void openDirectory(const QString& dir);
PICAUDIO_UTILS_EXPORT uint64_t directorySize(const QString& dir);

PICAUDIO_UTILS_EXPORT QStringList getFilesInDir(const QDir& baseDirectory, const QStringList& fileExtensions = {});
PICAUDIO_UTILS_EXPORT QStringList getFilesInDirRecursive(const QDir& baseDirectory, const QStringList& fileExtensions = {});
PICAUDIO_UTILS_EXPORT QList<QUrl> getUrlsInDir(const QDir& baseDirectory, const QStringList& fileExtensions = {});
PICAUDIO_UTILS_EXPORT QList<QUrl> getUrlsInDirRecursive(const QDir& baseDirectory, const QStringList& fileExtensions = {});
PICAUDIO_UTILS_EXPORT QStringList getFiles(const QStringList& paths, const QStringList& fileExtensions = {});
PICAUDIO_UTILS_EXPORT QStringList getFiles(const QList<QUrl>& urls, const QStringList& fileExtensions = {});
PICAUDIO_UTILS_EXPORT QStringList getAllSubdirectories(const QDir& dir);
PICAUDIO_UTILS_EXPORT QStringList getAllSubdirectories(const QDir& dir, std::stop_token stopToken);

PICAUDIO_UTILS_EXPORT bool pathContainsWildcard(const QString& path);
PICAUDIO_UTILS_EXPORT QStringList directoriesFromWildcardPath(const QString& pathPattern);
PICAUDIO_UTILS_EXPORT QStringList filesFromWildcardPath(const QString& pathPattern, QDir::Filters filters = QDir::Files,
                                                 QDir::SortFlags sort = QDir::Name);
} // namespace PicAudioEngine::Utils::File
