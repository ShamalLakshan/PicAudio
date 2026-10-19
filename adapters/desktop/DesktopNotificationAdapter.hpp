#pragma once

#include <QObject>
#include <QString>
#include <QSystemTrayIcon>

namespace adapters {

/**
 * @brief Native desktop toast notification adapter.
 *
 * Dispatches notifications with cover art and action buttons via Linux
 * org.freedesktop.Notifications D-Bus or QSystemTrayIcon fallback.
 */
class DesktopNotificationAdapter : public QObject {
  Q_OBJECT

public:
  explicit DesktopNotificationAdapter(QObject *parent = nullptr);
  ~DesktopNotificationAdapter() override = default;

  /**
   * @brief Connects to the system notification service.
   * @param trayIcon Optional system tray icon used for fallback notifications.
   */
  void initialize(QSystemTrayIcon *trayIcon = nullptr);

  /**
   * @brief Shows a track change notification toast.
   * @param title Track title.
   * @param artist Artist name.
   * @param album Album title.
   * @param audioSpecs Format specs string (e.g. "24-bit / 96kHz FLAC").
   * @param artFilePath Local filesystem path to album art image.
   */
  void showTrackNotification(const QString &title, const QString &artist,
                             const QString &album, const QString &audioSpecs,
                             const QString &artFilePath);

  /**
   * @brief Configures whether notifications are shown.
   */
  void setEnabled(bool enabled) { m_enabled = enabled; }
  [[nodiscard]] bool isEnabled() const { return m_enabled; }

  /**
   * @brief Sets notification display duration in seconds (2 - 10s).
   */
  void setDurationSec(int seconds);
  [[nodiscard]] int durationSec() const { return m_durationSec; }

  /**
   * @brief Configures suppression when PicAudio main window has focus.
   */
  void setSuppressWhenFocused(bool suppress) {
    m_suppressWhenFocused = suppress;
  }
  [[nodiscard]] bool suppressWhenFocused() const {
    return m_suppressWhenFocused;
  }

  /**
   * @brief Updates whether PicAudio currently has window focus.
   */
  void setWindowFocused(bool focused) { m_isWindowFocused = focused; }

  /**
   * @brief Updates full-screen DND mode state.
   */
  void setDndActive(bool active) { m_isDndActive = active; }

  /**
   * @brief Evaluates whether a notification should currently be suppressed.
   */
  [[nodiscard]] bool shouldSuppressNotification() const;

signals:
  void nextRequested();
  void togglePlayPauseRequested();

private slots:
#if defined(PICAUDIO_ENABLE_DBUS)
  void onActionInvoked(uint id, const QString &actionKey);
#endif

private:
  QSystemTrayIcon *m_trayIcon{nullptr};
  bool m_enabled{true};
  int m_durationSec{4};
  bool m_suppressWhenFocused{true};
  bool m_isWindowFocused{false};
  bool m_isDndActive{false};
  uint m_lastNotificationId{0};
  bool m_dbusNotificationsAvailable{false};
};

} // namespace adapters
