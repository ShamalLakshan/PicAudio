#include "corepaths.h"
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>

namespace PicAudioEngine::Core {

QString settingsPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
}

QString statePath()
{
    return settingsPath() + QStringLiteral("/state.ini");
}

QString playlistsPath()
{
    return settingsPath() + QStringLiteral("/playlists");
}

QStringList pluginPaths()
{
    return {userPluginsPath()};
}

QString userPluginsPath()
{
    return settingsPath() + QStringLiteral("/plugins");
}

QString translationsPath()
{
    return QStringLiteral(":/i18n");
}

} // namespace PicAudioEngine::Core
