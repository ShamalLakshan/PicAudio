#pragma once

#include "fycore_export.h"
#include <QString>
#include <QStringList>

namespace PicAudioEngine::Core {
PICAUDIO_ENGINE_EXPORT QString settingsPath();
PICAUDIO_ENGINE_EXPORT QString statePath();
PICAUDIO_ENGINE_EXPORT QString playlistsPath();
PICAUDIO_ENGINE_EXPORT QStringList pluginPaths();
PICAUDIO_ENGINE_EXPORT QString userPluginsPath();
PICAUDIO_ENGINE_EXPORT QString translationsPath();
} // namespace PicAudioEngine::Core
