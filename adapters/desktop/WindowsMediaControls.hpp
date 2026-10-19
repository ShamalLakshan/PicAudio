#pragma once

#include "PlatformMediaControls.hpp"
#include <QObject>

namespace adapters {

/**
 * @brief Windows System Media Transport Controls (SMTC) integration adapter.
 *
 * Integrates with Windows 10/11 taskbar media flyout, lock screen, and
 * hardware media keyboard controls using Windows Runtime SMTC APIs.
 */
class WindowsMediaControls : public PlatformMediaControls {
  Q_OBJECT

public:
  explicit WindowsMediaControls(QObject *parent = nullptr);
  ~WindowsMediaControls() override;

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
