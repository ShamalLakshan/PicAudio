#pragma once

#include <QAction>
#include <QIcon>
#include <QMenu>
#include <QObject>
#include <QString>
#include <QSystemTrayIcon>

namespace adapters {

/**
 * @brief System tray icon adapter managing background playback and tray
 * lifecycle.
 */
class DesktopTrayAdapter : public QObject {
  Q_OBJECT

public:
  explicit DesktopTrayAdapter(QObject *parent = nullptr);
  ~DesktopTrayAdapter() override;

  /**
   * @brief Initializes the tray icon and its context menu.
   * @return True if system tray is supported on this platform.
   */
  bool initialize();

  /**
   * @brief Updates now-playing track info shown in tray menu and tooltip.
   * @param title Track title.
   * @param artist Artist name.
   * @param durationStr Formatted total duration (e.g. "03:45").
   */
  void updateTrackInfo(const QString &title, const QString &artist,
                       const QString &durationStr);

  /**
   * @brief Updates playback transport state and dynamic icon badge.
   * @param isPlaying True if track is currently playing.
   */
  void updatePlaybackState(bool isPlaying);

  /**
   * @brief Updates position string displayed in hover tooltip.
   * @param positionStr Formatted elapsed time (e.g. "01:23").
   */
  void updatePosition(const QString &positionStr);

  /**
   * @brief Updates window visibility text in context menu.
   * @param isWindowVisible True if the main window is currently visible.
   */
  void updateWindowVisibility(bool isWindowVisible);

  /**
   * @brief Returns the underlying QSystemTrayIcon for notifications.
   */
  [[nodiscard]] QSystemTrayIcon *trayIcon() const { return m_trayIcon; }

  /**
   * @brief Checks if the system tray is available on this system.
   */
  [[nodiscard]] bool isAvailable() const;

  /**
   * @brief Formats rich hover tooltip text based on current playback state.
   */
  [[nodiscard]] QString formattedTooltip() const;

signals:
  void playPauseRequested();
  void nextRequested();
  void previousRequested();
  void stopRequested();
  void toggleMuteRequested();
  void setVolumePresetRequested(float volume);
  void toggleWindowRequested();
  void quitRequested();

private slots:
  void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

private:
  void setupMenu();
  void refreshIcon();
  void updateTooltip();

  QSystemTrayIcon *m_trayIcon{nullptr};
  QMenu *m_menu{nullptr};

  QAction *m_trackInfoAction{nullptr};
  QAction *m_playPauseAction{nullptr};
  QAction *m_nextAction{nullptr};
  QAction *m_prevAction{nullptr};
  QAction *m_stopAction{nullptr};
  QAction *m_toggleWindowAction{nullptr};
  QAction *m_quitAction{nullptr};

  QMenu *m_volumeMenu{nullptr};

  QIcon m_baseIcon;
  QIcon m_playingIcon;

  QString m_currentTitle;
  QString m_currentArtist;
  QString m_durationStr;
  QString m_positionStr;
  bool m_isPlaying{false};
  bool m_windowVisible{true};
};

} // namespace adapters
