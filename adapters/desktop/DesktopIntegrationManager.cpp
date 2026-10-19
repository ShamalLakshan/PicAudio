#include "DesktopIntegrationManager.hpp"

#include <QCoreApplication>
#include <QFileInfo>
#include <QUrl>

#include "GlobalMediaKeysHook.hpp"
#include "MacMediaControls.hpp"
#include "MprisAdapter.hpp"
#include "QtBridge.hpp"
#include "WindowsMediaControls.hpp"

namespace adapters {

DesktopIntegrationManager::DesktopIntegrationManager(QtBridge *bridge,
                                                     QObject *parent)
    : QObject(parent), m_bridge(bridge),
      m_singleInstance(new SingleInstanceManager(this)),
      m_trayAdapter(new DesktopTrayAdapter(this)),
      m_notificationAdapter(new DesktopNotificationAdapter(this)),
      m_globalMediaKeysHook(new GlobalMediaKeysHook(this)) {

#if defined(Q_OS_WIN)
  m_mediaControls = new WindowsMediaControls(this);
#elif defined(Q_OS_MAC)
  m_mediaControls = new MacMediaControls(this);
#else
  m_mediaControls = new MprisAdapter(this);
#endif

  wireSignals();
}

DesktopIntegrationManager::~DesktopIntegrationManager() = default;

void DesktopIntegrationManager::setMainWindow(QQuickWindow *window) {
  m_window = window;
  if (m_window) {
    connect(m_window.data(), &QWindow::activeChanged, this, [this]() {
      bool focused = m_window && m_window->isActive();
      if (m_notificationAdapter) {
        m_notificationAdapter->setWindowFocused(focused);
      }
    });
  }
}

bool DesktopIntegrationManager::initialize(const QStringList &args,
                                           const QString &workingDir) {
  // Check if another instance is already running
  if (!m_singleInstance->checkOrStartPrimary(args, workingDir)) {
    return false; // Forwarded to primary instance; exit secondary
  }

  // Initialize desktop sub-adapters
  m_trayAdapter->initialize();
  m_notificationAdapter->initialize(m_trayAdapter->trayIcon());
  m_mediaControls->initialize();
  m_globalMediaKeysHook->initialize();

  // Apply initial settings from QtBridge
  if (m_bridge) {
    m_notificationAdapter->setEnabled(m_bridge->showNotifications());
    m_notificationAdapter->setDurationSec(m_bridge->notificationDurationSec());
    m_notificationAdapter->setSuppressWhenFocused(
        m_bridge->suppressNotificationsWhenFocused());
    m_globalMediaKeysHook->setEnabled(m_bridge->globalMediaKeysEnabled());
  }

  // Parse and execute command line arguments for the primary instance
  CliCommand cmd = SingleInstanceManager::parseArguments(args, workingDir);
  if (cmd.type != CliCommand::Type::None) {
    executeCliCommand(cmd);
  }

  return true;
}

void DesktopIntegrationManager::wireSignals() {
  if (m_bridge) {
    connect(m_bridge, &QtBridge::playbackChanged, this,
            &DesktopIntegrationManager::onPlaybackChanged);
    connect(m_bridge, &QtBridge::positionChanged, this,
            &DesktopIntegrationManager::onPositionChanged);
    connect(m_bridge, &QtBridge::volumeChanged, this,
            &DesktopIntegrationManager::onVolumeChanged);
    connect(m_bridge, &QtBridge::playbackModeChanged, this,
            &DesktopIntegrationManager::onPlaybackModeChanged);
    connect(m_bridge, &QtBridge::settingsChanged, this,
            &DesktopIntegrationManager::onSettingsChanged);
    connect(m_bridge, &QtBridge::hotkeysChanged, this,
            &DesktopIntegrationManager::onSettingsChanged);
  }

  // Wire Single Instance CLI signals
  connect(m_singleInstance, &SingleInstanceManager::commandReceived, this,
          &DesktopIntegrationManager::onCliCommandReceived);
  connect(m_singleInstance, &SingleInstanceManager::raiseRequested, this,
          &DesktopIntegrationManager::raiseMainWindow);

  // Wire Tray signals
  connect(m_trayAdapter, &DesktopTrayAdapter::playPauseRequested, this,
          [this]() {
            if (m_bridge)
              m_bridge->togglePlayPause();
          });
  connect(m_trayAdapter, &DesktopTrayAdapter::nextRequested, this, [this]() {
    if (m_bridge)
      m_bridge->nextTrack();
  });
  connect(m_trayAdapter, &DesktopTrayAdapter::previousRequested, this,
          [this]() {
            if (m_bridge)
              m_bridge->previousTrack();
          });
  connect(m_trayAdapter, &DesktopTrayAdapter::stopRequested, this, [this]() {
    if (m_bridge)
      m_bridge->stop();
  });
  connect(m_trayAdapter, &DesktopTrayAdapter::toggleMuteRequested, this,
          [this]() {
            if (m_bridge)
              m_bridge->toggleMute();
          });
  connect(m_trayAdapter, &DesktopTrayAdapter::setVolumePresetRequested, this,
          [this](float vol) {
            if (m_bridge)
              m_bridge->setVolume(vol);
          });
  connect(m_trayAdapter, &DesktopTrayAdapter::toggleWindowRequested, this,
          &DesktopIntegrationManager::toggleWindowVisibility);
  connect(m_trayAdapter, &DesktopTrayAdapter::quitRequested, this, [this]() {
    if (m_bridge) {
      m_bridge->savePlaybackSession();
    }
    QCoreApplication::quit();
  });

  // Wire Notification signals
  connect(m_notificationAdapter, &DesktopNotificationAdapter::nextRequested,
          this, [this]() {
            if (m_bridge)
              m_bridge->nextTrack();
          });
  connect(m_notificationAdapter,
          &DesktopNotificationAdapter::togglePlayPauseRequested, this,
          [this]() {
            if (m_bridge)
              m_bridge->togglePlayPause();
          });

  // Wire Media Controls (MPRIS / SMTC / NowPlaying) signals
  if (m_mediaControls) {
    connect(m_mediaControls, &PlatformMediaControls::playRequested, this,
            [this]() {
              if (m_bridge && !m_bridge->isPlaying())
                m_bridge->togglePlayPause();
            });
    connect(m_mediaControls, &PlatformMediaControls::pauseRequested, this,
            [this]() {
              if (m_bridge && m_bridge->isPlaying())
                m_bridge->togglePlayPause();
            });
    connect(m_mediaControls, &PlatformMediaControls::togglePlayPauseRequested,
            this, [this]() {
              if (m_bridge)
                m_bridge->togglePlayPause();
            });
    connect(m_mediaControls, &PlatformMediaControls::stopRequested, this,
            [this]() {
              if (m_bridge)
                m_bridge->stop();
            });
    connect(m_mediaControls, &PlatformMediaControls::nextRequested, this,
            [this]() {
              if (m_bridge)
                m_bridge->nextTrack();
            });
    connect(m_mediaControls, &PlatformMediaControls::previousRequested, this,
            [this]() {
              if (m_bridge)
                m_bridge->previousTrack();
            });
    connect(m_mediaControls, &PlatformMediaControls::seekRequested, this,
            [this](qint64 posMs) {
              if (m_bridge)
                m_bridge->seek(posMs);
            });
    connect(m_mediaControls, &PlatformMediaControls::setVolumeRequested, this,
            [this](float vol) {
              if (m_bridge)
                m_bridge->setVolume(vol);
            });
    connect(m_mediaControls, &PlatformMediaControls::openUriRequested, this,
            [this](const QString &uri) {
              if (m_bridge) {
                QString localPath = uri;
                if (localPath.startsWith(QStringLiteral("file://"))) {
                  localPath = QUrl(uri).toLocalFile();
                }
                QFileInfo fi(localPath);
                if (fi.isDir()) {
                  m_bridge->openFolder(localPath, false);
                } else {
                  m_bridge->openAudioFile(localPath);
                }
              }
            });
    connect(m_mediaControls, &PlatformMediaControls::raiseRequested, this,
            &DesktopIntegrationManager::raiseMainWindow);
    connect(m_mediaControls, &PlatformMediaControls::quitRequested, this,
            [this]() {
              if (m_bridge) {
                m_bridge->savePlaybackSession();
              }
              QCoreApplication::quit();
            });
  }

  // Wire Global Media Keys Hook signals
  if (m_globalMediaKeysHook) {
    connect(m_globalMediaKeysHook, &GlobalMediaKeysHook::playPauseRequested,
            this, [this]() {
              if (m_bridge)
                m_bridge->togglePlayPause();
            });
    connect(m_globalMediaKeysHook, &GlobalMediaKeysHook::stopRequested, this,
            [this]() {
              if (m_bridge)
                m_bridge->stop();
            });
    connect(m_globalMediaKeysHook, &GlobalMediaKeysHook::nextRequested, this,
            [this]() {
              if (m_bridge)
                m_bridge->nextTrack();
            });
    connect(m_globalMediaKeysHook, &GlobalMediaKeysHook::previousRequested,
            this, [this]() {
              if (m_bridge)
                m_bridge->previousTrack();
            });
    connect(m_globalMediaKeysHook, &GlobalMediaKeysHook::muteRequested, this,
            [this]() {
              if (m_bridge)
                m_bridge->toggleMute();
            });
    connect(m_globalMediaKeysHook, &GlobalMediaKeysHook::volumeUpRequested,
            this, [this]() {
              if (m_bridge)
                m_bridge->setVolume(m_bridge->volume() + 0.05f);
            });
    connect(m_globalMediaKeysHook, &GlobalMediaKeysHook::volumeDownRequested,
            this, [this]() {
              if (m_bridge)
                m_bridge->setVolume(m_bridge->volume() - 0.05f);
            });
  }
}

void DesktopIntegrationManager::onPlaybackChanged() {
  if (!m_bridge) {
    return;
  }

  QString title = m_bridge->currentTrackTitle();
  QString artist = m_bridge->currentArtist();
  QString album = m_bridge->currentAlbum();
  QString artUrl = m_bridge->currentArtUrl();
  qint64 durationMs = m_bridge->durationMs();
  QString durationStr = m_bridge->durationStr();
  QString genre = m_bridge->currentGenre();
  QString filePath = m_bridge->currentFilePath();
  QString audioSpecs = m_bridge->currentAudioSpecs();
  bool isPlaying = m_bridge->isPlaying();

  // Update Media Controls
  if (m_mediaControls) {
    m_mediaControls->updateMetadata(title, artist, album, artUrl, durationMs, 1,
                                    genre, filePath);
    m_mediaControls->updatePlaybackState(isPlaying, m_bridge->positionMs());
  }

  // Update Tray Icon & Tooltip
  m_trayAdapter->updateTrackInfo(title, artist, durationStr);
  m_trayAdapter->updatePlaybackState(isPlaying);

  // Trigger toast notification on new track playback start
  if (isPlaying && (!m_lastPlaying || title != m_lastTrackTitle) &&
      !title.isEmpty()) {
    QString artPath;
    if (artUrl.startsWith(QStringLiteral("image://albumart/"))) {
      // Local cover art path derived from cover directory
      QString hash = m_bridge->currentArtHash();
      if (!hash.isEmpty()) {
        QString possible =
            QString::fromLocal8Bit(qgetenv("PICAUDIO_DATA_DIR")) + "/covers/" +
            hash + ".jpg";
        if (QFileInfo::exists(possible)) {
          artPath = possible;
        }
      }
    } else if (artUrl.startsWith(QStringLiteral("file://"))) {
      artPath = QUrl(artUrl).toLocalFile();
    } else if (QFileInfo::exists(artUrl)) {
      artPath = artUrl;
    }

    m_notificationAdapter->showTrackNotification(title, artist, album,
                                                 audioSpecs, artPath);
  }

  m_lastTrackTitle = title;
  m_lastPlaying = isPlaying;
}

void DesktopIntegrationManager::onPositionChanged() {
  if (!m_bridge) {
    return;
  }
  qint64 posMs = m_bridge->positionMs();
  QString posStr = m_bridge->positionStr();

  if (m_mediaControls) {
    m_mediaControls->updatePosition(posMs);
  }
  m_trayAdapter->updatePosition(posStr);
}

void DesktopIntegrationManager::onVolumeChanged() {
  if (!m_bridge || !m_mediaControls) {
    return;
  }
  m_mediaControls->updateVolume(m_bridge->volume());
}

void DesktopIntegrationManager::onPlaybackModeChanged() {
  if (!m_bridge || !m_mediaControls) {
    return;
  }
  m_mediaControls->updateModes(m_bridge->repeatMode(), m_bridge->shuffleMode());
}

void DesktopIntegrationManager::onSettingsChanged() {
  if (!m_bridge) {
    return;
  }
  if (m_notificationAdapter) {
    m_notificationAdapter->setEnabled(m_bridge->showNotifications());
    m_notificationAdapter->setDurationSec(m_bridge->notificationDurationSec());
    m_notificationAdapter->setSuppressWhenFocused(
        m_bridge->suppressNotificationsWhenFocused());
  }
  if (m_globalMediaKeysHook) {
    m_globalMediaKeysHook->setEnabled(m_bridge->globalMediaKeysEnabled());
  }
}

void DesktopIntegrationManager::onCliCommandReceived(const CliCommand &cmd) {
  executeCliCommand(cmd);
}

void DesktopIntegrationManager::executeCliCommand(const CliCommand &cmd) {
  if (!m_bridge) {
    return;
  }

  switch (cmd.type) {
  case CliCommand::Type::Play:
    if (!m_bridge->isPlaying()) {
      m_bridge->togglePlayPause();
    }
    break;

  case CliCommand::Type::Pause:
    if (m_bridge->isPlaying()) {
      m_bridge->togglePlayPause();
    }
    break;

  case CliCommand::Type::Toggle:
    m_bridge->togglePlayPause();
    break;

  case CliCommand::Type::Stop:
    m_bridge->stop();
    break;

  case CliCommand::Type::Next:
    m_bridge->nextTrack();
    break;

  case CliCommand::Type::Previous:
    m_bridge->previousTrack();
    break;

  case CliCommand::Type::SetVolume:
    m_bridge->setVolume(cmd.volume);
    break;

  case CliCommand::Type::PlayFiles:
    if (!cmd.paths.isEmpty()) {
      const QString &first = cmd.paths.first();
      QFileInfo fi(first);
      if (fi.isDir()) {
        m_bridge->openFolder(first, false);
      } else {
        m_bridge->openAudioFile(first);
      }
      for (int i = 1; i < cmd.paths.size(); ++i) {
        m_bridge->openFolder(cmd.paths.at(i), true);
      }
    }
    break;

  case CliCommand::Type::EnqueueFiles:
    for (const QString &path : cmd.paths) {
      m_bridge->openFolder(path, true);
    }
    break;

  case CliCommand::Type::RaiseWindow:
    raiseMainWindow();
    break;

  case CliCommand::Type::None:
    break;
  }

  if (cmd.raiseWindow) {
    raiseMainWindow();
  }
}

void DesktopIntegrationManager::toggleWindowVisibility() {
  if (!m_window) {
    return;
  }

  if (m_window->isVisible() && m_window->visibility() != QWindow::Minimized) {
    m_window->hide();
    m_trayAdapter->updateWindowVisibility(false);
  } else {
    raiseMainWindow();
  }
}

void DesktopIntegrationManager::raiseMainWindow() {
  if (!m_window) {
    return;
  }

  m_window->showNormal();
  m_window->raise();
  m_window->requestActivate();
  m_trayAdapter->updateWindowVisibility(true);
}

bool DesktopIntegrationManager::handleWindowCloseRequest() {
  if (m_bridge &&
      m_bridge->closeAction() == QStringLiteral("Minimize to System Tray")) {
    if (m_window) {
      m_window->hide();
    }
    m_trayAdapter->updateWindowVisibility(false);
    return false; // Prevent application exit; continue background playback in
                  // tray
  }

  // Exit Application: stop audio playback, persist session state, and quit
  if (m_bridge) {
    m_bridge->stop();
    m_bridge->savePlaybackSession();
  }
  QMetaObject::invokeMethod(QCoreApplication::instance(), "quit",
                            Qt::QueuedConnection);
  return true; // Proceed with window closure
}

void DesktopIntegrationManager::handleWindowMinimizeRequest() {
  if (m_bridge &&
      m_bridge->minimizeAction() == QStringLiteral("Minimize to System Tray")) {
    if (m_window) {
      m_window->hide();
    }
    m_trayAdapter->updateWindowVisibility(false);
  }
}

} // namespace adapters
