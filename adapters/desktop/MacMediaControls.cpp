#include "MacMediaControls.hpp"

namespace adapters {

struct MacMediaControls::Impl {
  bool initialized{false};
};

MacMediaControls::MacMediaControls(QObject *parent)
    : PlatformMediaControls(parent), m_impl(new Impl()) {}

MacMediaControls::~MacMediaControls() { delete m_impl; }

bool MacMediaControls::initialize() {
#if defined(__APPLE__)
  m_impl->initialized = true;
  return true;
#else
  return false;
#endif
}

void MacMediaControls::updateMetadata(const QString &title,
                                      const QString &artist,
                                      const QString &album,
                                      const QString &artUrl, qint64 durationMs,
                                      int trackNumber, const QString &genre,
                                      const QString &filePath) {
  Q_UNUSED(title);
  Q_UNUSED(artist);
  Q_UNUSED(album);
  Q_UNUSED(artUrl);
  Q_UNUSED(durationMs);
  Q_UNUSED(trackNumber);
  Q_UNUSED(genre);
  Q_UNUSED(filePath);
}

void MacMediaControls::updatePlaybackState(bool isPlaying, qint64 positionMs) {
  Q_UNUSED(isPlaying);
  Q_UNUSED(positionMs);
}

void MacMediaControls::updatePosition(qint64 positionMs) {
  Q_UNUSED(positionMs);
}

void MacMediaControls::updateVolume(float volume) { Q_UNUSED(volume); }

void MacMediaControls::updateModes(int repeatMode, int shuffleMode) {
  Q_UNUSED(repeatMode);
  Q_UNUSED(shuffleMode);
}

} // namespace adapters
