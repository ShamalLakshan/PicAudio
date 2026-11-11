#pragma once

#include "fycore_export.h"
#include <QString>
#include <QStringList>

namespace Fooyin::Core {
FYCORE_EXPORT QString settingsPath();
FYCORE_EXPORT QString statePath();
FYCORE_EXPORT QString playlistsPath();
FYCORE_EXPORT QStringList pluginPaths();
FYCORE_EXPORT QString userPluginsPath();
FYCORE_EXPORT QString translationsPath();
} // namespace Fooyin::Core
