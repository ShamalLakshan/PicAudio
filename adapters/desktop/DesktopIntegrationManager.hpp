#pragma once

#include <QObject>
#include <QPointer>
#include <QQuickWindow>
#include <memory>

#include "DesktopNotificationAdapter.hpp"
#include "DesktopTrayAdapter.hpp"
#include "PlatformMediaControls.hpp"
#include "SingleInstanceManager.hpp"

class QtBridge;

namespace adapters {

/**
 * @brief Central coordinator for OS desktop integration across platforms.
 *
 * Synchronizes platform media keys (MPRIS2/SMTC/NowPlaying), system tray,
 * native toast notifications, window lifecycle events, and single-instance IPC.
 */
class DesktopIntegrationManager : public QObject {
  Q_OBJECT

public:
  explicit DesktopIntegrationManager(QtBridge *bridge,
                                     QObject *parent = nullptr);
  ~DesktopIntegrationManager() override;

  /**
   * @brief Attaches the primary application QML window to the desktop manager.
   * @param window Pointer to the active QQuickWindow.
   */
  void setMainWindow(QQuickWindow *window);

  /**
   * @brief Initializes single-instance IPC, tray icon, notifications, and media
   * controls.
   * @param args Command-line arguments.
   * @param workingDir Invocation working directory.
   * @return True if initialized as primary instance; false if forwarded to
   * existing instance.
   */
  bool initialize(const QStringList &args,
                  const QString &workingDir = QString());

  /**
   * @brief Executes a parsed command line instruction.
   * @param cmd CliCommand to execute.
   */
  void executeCliCommand(const CliCommand &cmd);

  /**
   * @brief Toggles main window visibility between foreground and hidden/tray.
   */
  void toggleWindowVisibility();

  /**
   * @brief Brings the primary window to the foreground and raises focus.
   */
  void raiseMainWindow();

  /**
   * @brief Handles window close button click according to closeAction
   * preference.
   * @return True if close event should be accepted (exit); false if minimized
   * to tray.
   */
  bool handleWindowCloseRequest();

  /**
   * @brief Handles window minimize event according to minimizeAction
   * preference.
   */
  void handleWindowMinimizeRequest();

  [[nodiscard]] SingleInstanceManager *singleInstanceManager() const {
    return m_singleInstance;
  }
  [[nodiscard]] DesktopTrayAdapter *trayAdapter() const {
    return m_trayAdapter;
  }
  [[nodiscard]] DesktopNotificationAdapter *notificationAdapter() const {
    return m_notificationAdapter;
  }
  [[nodiscard]] PlatformMediaControls *mediaControls() const {
    return m_mediaControls;
  }
  [[nodiscard]] class GlobalMediaKeysHook *globalMediaKeysHook() const {
    return m_globalMediaKeysHook;
  }

private slots:
  void onPlaybackChanged();
  void onPositionChanged();
  void onVolumeChanged();
  void onPlaybackModeChanged();
  void onSettingsChanged();
  void onCliCommandReceived(const adapters::CliCommand &cmd);

private:
  void wireSignals();
  void updateMediaControls();

  QtBridge *m_bridge{nullptr};
  QPointer<QQuickWindow> m_window;

  SingleInstanceManager *m_singleInstance{nullptr};
  DesktopTrayAdapter *m_trayAdapter{nullptr};
  DesktopNotificationAdapter *m_notificationAdapter{nullptr};
  PlatformMediaControls *m_mediaControls{nullptr};
  class GlobalMediaKeysHook *m_globalMediaKeysHook{nullptr};

  QString m_lastTrackTitle;
  bool m_lastPlaying{false};
};

} // namespace adapters
