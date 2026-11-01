#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QStandardPaths>
#include <iostream>
#include <memory>

#include "QtBridge.hpp"
#include "audio/BitPerfectAudioAdapter.hpp"
#include "core/services/LibraryService.hpp"
#include "core/services/PlayerService.hpp"
#include "desktop/DesktopIntegrationManager.hpp"
#include "metadata/TagLibMetadataAdapter.hpp"
#include "plugin/PluginManager.hpp"
#include "providers/AlbumArtImageProvider.hpp"
#include "storage/SqliteDatabaseAdapter.hpp"
#include "theming/ThemeLoader.hpp"

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  app.setOrganizationName("PicAudio");
  app.setApplicationName("PicAudio");
  app.setWindowIcon(QIcon(":/icons/picaudio.png"));

  if (app.arguments().contains(QStringLiteral("--help")) ||
      app.arguments().contains(QStringLiteral("-h"))) {
    std::cout << "PicAudio - Lightweight high-fidelity music player\n\n"
              << "Usage: picaudio [OPTIONS] [FILES/FOLDERS...]\n\n"
              << "Options:\n"
              << "  --play             Resume or start playback\n"
              << "  --pause            Pause active playback\n"
              << "  --toggle           Toggle between play and pause\n"
              << "  --stop             Stop playback\n"
              << "  --next             Skip to next track in queue\n"
              << "  --prev, --previous Return to previous track\n"
              << "  --volume <0-100>   Set playback volume percentage\n"
              << "  --enqueue, -e      Append files or folders to queue "
                 "without interrupting playback\n"
              << "  --raise, --show    Bring primary window to the foreground\n"
              << "  --portable         Run in portable data mode\n"
              << "  --help, -h         Show this help message\n";
    return 0;
  }

  // Determine application data directory
  QString appDataDir;
#ifdef Q_OS_WIN
  const QString baseDir = QCoreApplication::applicationDirPath();
  const QString portableDir = baseDir + "/data";
  const bool isPortable =
      QFile::exists(baseDir + "/portable.dat") ||
      app.arguments().contains(QStringLiteral("--portable")) ||
      qEnvironmentVariableIsSet("PICAUDIO_PORTABLE");
  if (isPortable) {
    appDataDir = portableDir;
    QDir().mkpath(appDataDir);
    qputenv("PICAUDIO_PORTABLE", "1");
    qputenv("PICAUDIO_DATA_DIR", portableDir.toUtf8());
  } else {
    appDataDir =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataDir);
  }
#else
  // On Linux/POSIX (including AppImage), follow standard XDG directory
  // specifications
  const bool isPortable =
      !qEnvironmentVariableIsSet("APPIMAGE") &&
      (QFile::exists(QCoreApplication::applicationDirPath() +
                     "/portable.dat") ||
       app.arguments().contains(QStringLiteral("--portable")) ||
       qEnvironmentVariableIsSet("PICAUDIO_PORTABLE"));
  if (isPortable) {
    const QString portableDir =
        QCoreApplication::applicationDirPath() + "/data";
    appDataDir = portableDir;
    QDir().mkpath(appDataDir);
  } else {
    appDataDir =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataDir);
  }
#endif
  QString dbPath = appDataDir + "/picaudio_library.db";

  auto dbAdapter = std::make_unique<adapters::SqliteDatabaseAdapter>();
  dbAdapter->initialize(dbPath.toStdString());

  // Metadata and audio adapters
  auto metadataAdapter = std::make_unique<adapters::TagLibMetadataAdapter>();
  auto audioEngine = std::make_shared<adapters::BitPerfectAudioAdapter>();
  audioEngine->initialize(96000, 2);

  // Core services
  core::LibraryService libraryService(*dbAdapter, *metadataAdapter);
  core::PlayerService playerService(audioEngine);

  // Theming engine
  auto themeLoader = std::make_unique<adapters::ThemeLoader>();

  // Bridge connecting core to QML
  QtBridge bridge(playerService, libraryService, themeLoader.get());

  // Dynamic Plugin Manager (Phase 2)
  adapters::PluginManager pluginManager;
  pluginManager.setThemeLoader(themeLoader.get());
  pluginManager.setNavigateCallback([&bridge](int view) {
    QMetaObject::invokeMethod(
        &bridge, [&bridge, view]() { bridge.setActiveWorkspaceViewMode(view); },
        Qt::QueuedConnection);
  });
  pluginManager.setToastCallback(
      [&bridge](const QString &msg, const QString &lvl) {
        QMetaObject::invokeMethod(
            &bridge, [&bridge, msg, lvl]() { bridge.showToast(msg, lvl); },
            Qt::QueuedConnection);
      });
  pluginManager.initialize(
      {appDataDir + "/plugins",
       QCoreApplication::applicationDirPath() + "/plugins",
       QCoreApplication::applicationDirPath() + "/../plugins"});
  pluginManager.scanAndLoadPlugins();
  bridge.setPluginManager(&pluginManager);

  // Notify theme plugins on base theme change
  QObject::connect(themeLoader.get(), &adapters::ThemeLoader::themeChanged,
                   [&pluginManager, themePtr = themeLoader.get()]() {
                     pluginManager.routeBaseThemeChanged(themePtr->themeId());
                   });

  // Audio visualizer hook routing to plugins
  audioEngine->setAudioBufferCallback(
      [&pluginManager](const float *buffer, size_t frames, uint32_t sampleRate,
                       uint8_t channels) {
        pluginManager.routeAudioBuffer(buffer, frames, sampleRate, channels);
      });

  // Desktop integration (Single-instance IPC, MPRIS, Tray, Notifications)
  adapters::DesktopIntegrationManager desktop(&bridge);
  if (!desktop.initialize(app.arguments(), QDir::currentPath())) {
    // Secondary instance forwarded arguments to primary instance; exit cleanly
    return 0;
  }

  // Play next track when current one finishes
  audioEngine->setEndOfTrackCallback([&bridge]() {
    QMetaObject::invokeMethod(&bridge, "nextTrack", Qt::QueuedConnection);
  });

  // Restore playback session according to startup action preferences
  bridge.restorePlaybackSession();

  // QML engine and album art provider
  QString coversDir = appDataDir + "/covers";
  QQmlApplicationEngine engine;
  engine.addImageProvider(
      "albumart", new AlbumArtImageProvider(metadataAdapter.get(),
                                            dbAdapter.get(), coversDir));
  engine.rootContext()->setContextProperty("Theme", themeLoader.get());
  engine.rootContext()->setContextProperty("bridge", &bridge);
  engine.rootContext()->setContextProperty("desktopIntegration", &desktop);
  bridge.setQmlEngine(&engine);

  // Connect QML quit signal to application quit
  QObject::connect(&engine, &QQmlApplicationEngine::quit, &app,
                   &QCoreApplication::quit);

  // Guarantee audio stop and session save on application termination
  QObject::connect(&app, &QCoreApplication::aboutToQuit,
                   [&bridge, &audioEngine, &pluginManager]() {
                     audioEngine->stop();
                     bridge.savePlaybackSession();
                     pluginManager.unloadAllPlugins();
                   });

  QUrl url(QStringLiteral("qrc:/PlayerUI/ui_qt/qml/Main.qml"));
  if (!QFile::exists(QStringLiteral(":/PlayerUI/ui_qt/qml/Main.qml")) &&
      QFile::exists(QStringLiteral(":/qt/qml/PlayerUI/ui_qt/qml/Main.qml"))) {
    url = QUrl(QStringLiteral("qrc:/qt/qml/PlayerUI/ui_qt/qml/Main.qml"));
  }
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreated, &app,
      [&desktop, url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
          QCoreApplication::exit(-1);
          return;
        }
        if (auto *win = qobject_cast<QQuickWindow *>(obj)) {
          desktop.setMainWindow(win);
        }
      },
      Qt::QueuedConnection);

  engine.load(url);

  return app.exec();
}
