#pragma once

#include <QObject>

namespace adapters {

/**
 * @brief System-wide global media keys listener.
 *
 * Hooks hardware media keys across Linux (D-Bus media-keys grab),
 * Windows (RegisterHotKey Win32 API), and macOS platforms to route
 * transport events when unfocused or minimized.
 */
class GlobalMediaKeysHook : public QObject {
  Q_OBJECT

public:
  explicit GlobalMediaKeysHook(QObject *parent = nullptr);
  ~GlobalMediaKeysHook() override;

  /**
   * @brief Initializes OS-specific global hook listeners.
   * @return True if hooks were registered successfully.
   */
  bool initialize();

  /**
   * @brief Enables or disables global hotkey capture.
   * @param enable True to capture keys, false to release.
   */
  void setEnabled(bool enable);

  /**
   * @brief Checks if global media key capture is enabled.
   * @return True if actively listening.
   */
  [[nodiscard]] bool isEnabled() const { return m_enabled; }

signals:
  void playPauseRequested();
  void stopRequested();
  void nextRequested();
  void previousRequested();
  void muteRequested();
  void volumeUpRequested();
  void volumeDownRequested();

private slots:
#if defined(PICAUDIO_ENABLE_DBUS)
  void onGnomeMediaKeyPressed(const QString &app, const QString &key);
#endif

private:
  void registerHooks();
  void unregisterHooks();

  bool m_enabled{true};
  bool m_registered{false};
  struct Impl;
  Impl *m_impl;
};

} // namespace adapters
