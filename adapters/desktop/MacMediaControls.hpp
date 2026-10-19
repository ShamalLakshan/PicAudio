#pragma once

#include "PlatformMediaControls.hpp"
#include <QObject>

namespace adapters {

/**
 * @brief macOS Now Playing (MPNowPlayingInfoCenter & MPRemoteCommandCenter)
 * adapter.
 *
 * Publishes now-playing metadata and binds remote transport commands to macOS
 * Control Center, Lock Screen, and Touch Bar.
 */
class MacMediaControls : public PlatformMediaControls {
  Q_OBJECT

public:
  explicit MacMediaControls(QObject *parent = nullptr);
  ~MacMediaControls() override;

  bool initialize() override;
  void updateMetadata(const QString &title, const QString &artist,
                      const QString &album, const QString &artUrl,
                      qint64 durationMs, int trackNumber, const QString &genre,
                      const QString &filePath) override;
  void updatePlaybackState(bool isPlaying, qint64 positionMs) override;
  void updatePosition(qint64 positionMs) override;
  void updateVolume(float volume) override;
  void updateModes(int repeatMode, int shuffleMode) override;

private:
  struct Impl;
  Impl *m_impl{nullptr};
};

} // namespace adapters
