#include "WindowsMediaControls.hpp"

#if defined(_WIN32)
#include <systemmediatransportcontrolsinterop.h>
#include <windows.h>
#include <wrl/client.h>
#include <wrl/wrappers/corewrappers.h>
#endif

namespace adapters {

struct WindowsMediaControls::Impl {
  bool initialized{false};
};

WindowsMediaControls::WindowsMediaControls(QObject *parent)
    : PlatformMediaControls(parent), m_impl(new Impl()) {}

WindowsMediaControls::~WindowsMediaControls() { delete m_impl; }

bool WindowsMediaControls::initialize() {
#if defined(_WIN32)
  m_impl->initialized = true;
  return true;
#else
  return false;
#endif
}

void WindowsMediaControls::updateMetadata(
    const QString &title, const QString &artist, const QString &album,
    const QString &artUrl, qint64 durationMs, int trackNumber,
    const QString &genre, const QString &filePath) {
  Q_UNUSED(title);
  Q_UNUSED(artist);
  Q_UNUSED(album);
  Q_UNUSED(artUrl);
  Q_UNUSED(durationMs);
  Q_UNUSED(trackNumber);
  Q_UNUSED(genre);
  Q_UNUSED(filePath);
}

void WindowsMediaControls::updatePlaybackState(bool isPlaying,
                                               qint64 positionMs) {
  Q_UNUSED(isPlaying);
  Q_UNUSED(positionMs);
}

void WindowsMediaControls::updatePosition(qint64 positionMs) {
  Q_UNUSED(positionMs);
}

void WindowsMediaControls::updateVolume(float volume) { Q_UNUSED(volume); }

void WindowsMediaControls::updateModes(int repeatMode, int shuffleMode) {
  Q_UNUSED(repeatMode);
  Q_UNUSED(shuffleMode);
}

} // namespace adapters
