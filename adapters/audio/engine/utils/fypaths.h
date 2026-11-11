/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fyutils_export.h"

#include <QString>

namespace PicAudioEngine::Utils {
PICAUDIO_UTILS_EXPORT bool isPortable();
PICAUDIO_UTILS_EXPORT bool isFlatpak();
PICAUDIO_UTILS_EXPORT QString createPath(const QString& path, const QString& appendPath = {});
PICAUDIO_UTILS_EXPORT QString configPath(const QString& appendPath = {});
PICAUDIO_UTILS_EXPORT QString statePath(const QString& appendPath = {});
PICAUDIO_UTILS_EXPORT QString sharePath(const QString& appendPath = {});
PICAUDIO_UTILS_EXPORT QString cachePath(const QString& appendPath = {});
} // namespace PicAudioEngine::Utils
