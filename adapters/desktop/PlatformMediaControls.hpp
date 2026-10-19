#pragma once

#include <QObject>
#include <QString>

namespace adapters {

/**
 * @brief Abstract platform media controls interface for OS desktop integration.
 *
 * Provides a unified abstraction for Linux MPRIS2, Windows SMTC, and macOS
 * Now Playing services to receive media keys and publish playback metadata.
 */
class PlatformMediaControls : public QObject {
  Q_OBJECT

public:
  explicit PlatformMediaControls(QObject *parent = nullptr) : QObject(parent) {}
  ~PlatformMediaControls() override = default;

  /**
   * @brief Initializes the platform media controls service.
   * @return True if registration succeeded.
   */
  virtual bool initialize() = 0;

  /**
   * @brief Updates now-playing metadata published to the OS.
   * @param title Track title.
   * @param artist Artist name.
   * @param album Album name.
   * @param artUrl URL or local file path to cover artwork.
   * @param durationMs Track duration in milliseconds.
   * @param trackNumber Track sequence number in album.
   * @param genre Genre label.
   * @param filePath Absolute path of audio file.
   */
  virtual void updateMetadata(const QString &title, const QString &artist,
                              const QString &album, const QString &artUrl,
                              qint64 durationMs, int trackNumber,
                              const QString &genre,
                              const QString &filePath) = 0;

  /**
   * @brief Updates playback transport state.
   * @param isPlaying True if currently playing; false if paused or stopped.
   * @param positionMs Current position in milliseconds.
   */
  virtual void updatePlaybackState(bool isPlaying, qint64 positionMs) = 0;

  /**
   * @brief Updates current timeline position.
   * @param positionMs Playback position in milliseconds.
   */
  virtual void updatePosition(qint64 positionMs) = 0;

  /**
   * @brief Updates published volume level.
   * @param volume Normalised volume from 0.0 to 1.0.
   */
  virtual void updateVolume(float volume) = 0;

  /**
   * @brief Updates loop and shuffle modes.
   * @param repeatMode 0: off, 1: repeat all, 2: repeat track.
   * @param shuffleMode 0: sequential, 1: shuffle.
   */
  virtual void updateModes(int repeatMode, int shuffleMode) = 0;

signals:
  void playRequested();
  void pauseRequested();
  void togglePlayPauseRequested();
  void stopRequested();
  void nextRequested();
  void previousRequested();
  void seekRequested(qint64 positionMs);
  void setVolumeRequested(float volume);
  void openUriRequested(const QString &uri);
  void raiseRequested();
  void quitRequested();
};

} // namespace adapters
