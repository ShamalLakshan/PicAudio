#include "MprisAdapter.hpp"

#include <QCoreApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QUrl>

#if defined(PICAUDIO_ENABLE_DBUS)
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#endif

namespace adapters {

#if defined(PICAUDIO_ENABLE_DBUS)

MprisRootAdaptor::MprisRootAdaptor(MprisAdapter *parent)
    : QDBusAbstractAdaptor(parent), m_adapter(parent) {}

QString MprisRootAdaptor::identity() const {
  return QStringLiteral("PicAudio");
}

QString MprisRootAdaptor::desktopEntry() const {
  return QStringLiteral("picaudio");
}

QStringList MprisRootAdaptor::supportedUriSchemes() const {
  return QStringList{QStringLiteral("file")};
}

QStringList MprisRootAdaptor::supportedMimeTypes() const {
  return QStringList{QStringLiteral("audio/flac"),
                     QStringLiteral("audio/x-flac"),
                     QStringLiteral("audio/x-wav"),
                     QStringLiteral("audio/wav"),
                     QStringLiteral("audio/mpeg"),
                     QStringLiteral("audio/mp3"),
                     QStringLiteral("audio/mp4"),
                     QStringLiteral("audio/m4a"),
                     QStringLiteral("audio/ogg"),
                     QStringLiteral("audio/opus"),
                     QStringLiteral("audio/x-aiff"),
                     QStringLiteral("audio/aac"),
                     QStringLiteral("audio/x-monkeys-audio"),
                     QStringLiteral("audio/x-ape"),
                     QStringLiteral("audio/x-wavpack"),
                     QStringLiteral("audio/x-ms-wma"),
                     QStringLiteral("application/x-cue"),
                     QStringLiteral("audio/x-mpegurl"),
                     QStringLiteral("audio/x-scpls")};
}

void MprisRootAdaptor::Raise() {
  if (m_adapter) {
    emit m_adapter->raiseRequested();
  }
}

void MprisRootAdaptor::Quit() {
  if (m_adapter) {
    emit m_adapter->quitRequested();
  }
}

MprisPlayerAdaptor::MprisPlayerAdaptor(MprisAdapter *parent)
    : QDBusAbstractAdaptor(parent), m_adapter(parent) {}

QString MprisPlayerAdaptor::playbackStatus() const {
  return m_adapter ? m_adapter->playbackStatus() : QStringLiteral("Stopped");
}

QString MprisPlayerAdaptor::loopStatus() const {
  return m_adapter ? m_adapter->loopStatus() : QStringLiteral("None");
}

void MprisPlayerAdaptor::setLoopStatus(const QString &loop) {
  if (m_adapter) {
    m_adapter->setLoopStatus(loop);
  }
}

double MprisPlayerAdaptor::rate() const {
  return m_adapter ? m_adapter->rate() : 1.0;
}

void MprisPlayerAdaptor::setRate(double r) {
  if (m_adapter) {
    m_adapter->setRate(r);
  }
}

bool MprisPlayerAdaptor::shuffle() const {
  return m_adapter && m_adapter->shuffle();
}

void MprisPlayerAdaptor::setShuffle(bool s) {
  if (m_adapter) {
    m_adapter->setShuffle(s);
  }
}

QVariantMap MprisPlayerAdaptor::metadata() const {
  return m_adapter ? m_adapter->metadata() : QVariantMap{};
}

double MprisPlayerAdaptor::volume() const {
  return m_adapter ? m_adapter->volume() : 1.0;
}

void MprisPlayerAdaptor::setVolume(double vol) {
  if (m_adapter) {
    m_adapter->setVolume(vol);
  }
}

qlonglong MprisPlayerAdaptor::position() const {
  return m_adapter ? m_adapter->position() : 0;
}

void MprisPlayerAdaptor::Next() {
  if (m_adapter) {
    emit m_adapter->nextRequested();
  }
}

void MprisPlayerAdaptor::Previous() {
  if (m_adapter) {
    emit m_adapter->previousRequested();
  }
}

void MprisPlayerAdaptor::Pause() {
  if (m_adapter) {
    emit m_adapter->pauseRequested();
  }
}

void MprisPlayerAdaptor::PlayPause() {
  if (m_adapter) {
    emit m_adapter->togglePlayPauseRequested();
  }
}

void MprisPlayerAdaptor::Stop() {
  if (m_adapter) {
    emit m_adapter->stopRequested();
  }
}

void MprisPlayerAdaptor::Play() {
  if (m_adapter) {
    emit m_adapter->playRequested();
  }
}

void MprisPlayerAdaptor::Seek(qlonglong Offset) {
  if (m_adapter) {
    m_adapter->triggerSeek(Offset);
  }
}

void MprisPlayerAdaptor::SetPosition(const QDBusObjectPath &TrackId,
                                     qlonglong Position) {
  if (m_adapter) {
    m_adapter->triggerSetPosition(TrackId.path(), Position);
  }
}

void MprisPlayerAdaptor::OpenUri(const QString &Uri) {
  if (m_adapter) {
    emit m_adapter->openUriRequested(Uri);
  }
}

#endif // PICAUDIO_ENABLE_DBUS

MprisAdapter::MprisAdapter(QObject *parent) : PlatformMediaControls(parent) {
#if defined(PICAUDIO_ENABLE_DBUS)
  m_rootAdaptor = new MprisRootAdaptor(this);
  m_playerAdaptor = new MprisPlayerAdaptor(this);
#endif
}

MprisAdapter::~MprisAdapter() {
#if defined(PICAUDIO_ENABLE_DBUS)
  if (m_registered) {
    QDBusConnection bus = QDBusConnection::sessionBus();
    bus.unregisterObject(QStringLiteral("/org/mpris/MediaPlayer2"));
    bus.unregisterService(QStringLiteral("org.mpris.MediaPlayer2.picaudio"));
  }
#endif
}

bool MprisAdapter::initialize() {
#if defined(PICAUDIO_ENABLE_DBUS)
  QDBusConnection bus = QDBusConnection::sessionBus();
  if (!bus.isConnected()) {
    return false;
  }

  // Register object on path
  if (!bus.registerObject(QStringLiteral("/org/mpris/MediaPlayer2"), this,
                          QDBusConnection::ExportAdaptors)) {
    return false;
  }

  // Try standard service name, fall back to PID-suffixed name
  QString serviceName = QStringLiteral("org.mpris.MediaPlayer2.picaudio");
  if (!bus.registerService(serviceName)) {
    serviceName = QStringLiteral("org.mpris.MediaPlayer2.picaudio.instance%1")
                      .arg(QCoreApplication::applicationPid());
    if (!bus.registerService(serviceName)) {
      return false;
    }
  }

  m_registered = true;
  return true;
#else
  return false;
#endif
}

void MprisAdapter::updateMetadata(const QString &title, const QString &artist,
                                  const QString &album, const QString &artUrl,
                                  qint64 durationMs, int trackNumber,
                                  const QString &genre,
                                  const QString &filePath) {
  m_durationMs = durationMs;

  // Build clean D-Bus object path for track id
  QString safeId =
      QString::number(qHash(filePath.isEmpty() ? title : filePath));
  m_currentTrackId =
      QStringLiteral("/org/mpris/MediaPlayer2/Track/%1").arg(safeId);

  QVariantMap meta;
#if defined(PICAUDIO_ENABLE_DBUS)
  meta.insert(QStringLiteral("mpris:trackid"),
              QVariant::fromValue(QDBusObjectPath(m_currentTrackId)));
#else
  meta.insert(QStringLiteral("mpris:trackid"), m_currentTrackId);
#endif

  // MPRIS uses microseconds for duration
  meta.insert(QStringLiteral("mpris:length"),
              static_cast<qlonglong>(durationMs * 1000));

  if (!artUrl.isEmpty()) {
    QString urlStr = artUrl;
    if (!urlStr.contains(QStringLiteral("://"))) {
      urlStr = QUrl::fromLocalFile(urlStr).toString();
    }
    meta.insert(QStringLiteral("mpris:artUrl"), urlStr);
  }

  if (!title.isEmpty()) {
    meta.insert(QStringLiteral("xesam:title"), title);
  }
  if (!artist.isEmpty()) {
    meta.insert(QStringLiteral("xesam:artist"), QStringList{artist});
    meta.insert(QStringLiteral("xesam:albumArtist"), QStringList{artist});
  }
  if (!album.isEmpty()) {
    meta.insert(QStringLiteral("xesam:album"), album);
  }
  if (!genre.isEmpty()) {
    meta.insert(QStringLiteral("xesam:genre"), QStringList{genre});
  }
  if (trackNumber > 0) {
    meta.insert(QStringLiteral("xesam:trackNumber"), trackNumber);
  }
  if (!filePath.isEmpty()) {
    meta.insert(QStringLiteral("xesam:url"),
                QUrl::fromLocalFile(filePath).toString());
  }

  m_metadata = meta;

  QVariantMap changed;
  changed.insert(QStringLiteral("Metadata"), m_metadata);
  emitPropertiesChanged(QStringLiteral("org.mpris.MediaPlayer2.Player"),
                        changed);
}

void MprisAdapter::updatePlaybackState(bool isPlaying, qint64 positionMs) {
  m_positionMs = positionMs;
  QString newStatus = isPlaying
                          ? QStringLiteral("Playing")
                          : (m_positionMs > 0 ? QStringLiteral("Paused")
                                              : QStringLiteral("Stopped"));
  if (newStatus != m_playbackStatus) {
    m_playbackStatus = newStatus;
    QVariantMap changed;
    changed.insert(QStringLiteral("PlaybackStatus"), m_playbackStatus);
    emitPropertiesChanged(QStringLiteral("org.mpris.MediaPlayer2.Player"),
                          changed);
  }
}

void MprisAdapter::updatePosition(qint64 positionMs) {
  m_positionMs = positionMs;
}

void MprisAdapter::updateVolume(float volume) {
  double v = static_cast<double>(std::clamp(volume, 0.0f, 1.0f));
  if (std::abs(v - m_volume) > 0.001) {
    m_volume = v;
    QVariantMap changed;
    changed.insert(QStringLiteral("Volume"), m_volume);
    emitPropertiesChanged(QStringLiteral("org.mpris.MediaPlayer2.Player"),
                          changed);
  }
}

void MprisAdapter::updateModes(int repeatMode, int shuffleMode) {
  QString newLoop = QStringLiteral("None");
  if (repeatMode == 1) {
    newLoop = QStringLiteral("Playlist");
  } else if (repeatMode == 2) {
    newLoop = QStringLiteral("Track");
  }

  bool newShuffle = (shuffleMode == 1);
  QVariantMap changed;
  if (newLoop != m_loopStatus) {
    m_loopStatus = newLoop;
    changed.insert(QStringLiteral("LoopStatus"), m_loopStatus);
  }
  if (newShuffle != m_shuffle) {
    m_shuffle = newShuffle;
    changed.insert(QStringLiteral("Shuffle"), m_shuffle);
  }

  if (!changed.isEmpty()) {
    emitPropertiesChanged(QStringLiteral("org.mpris.MediaPlayer2.Player"),
                          changed);
  }
}

void MprisAdapter::setLoopStatus(const QString &loop) {
  if (loop == QStringLiteral("Track")) {
    m_loopStatus = loop;
    emit setVolumeRequested(2); // Repeat track
  } else if (loop == QStringLiteral("Playlist")) {
    m_loopStatus = loop;
    emit setVolumeRequested(1); // Repeat all
  } else {
    m_loopStatus = QStringLiteral("None");
    emit setVolumeRequested(0);
  }
}

void MprisAdapter::setRate(double r) {
  if (r > 0.1 && r < 4.0) {
    m_rate = r;
    QVariantMap changed;
    changed.insert(QStringLiteral("Rate"), m_rate);
    emitPropertiesChanged(QStringLiteral("org.mpris.MediaPlayer2.Player"),
                          changed);
  }
}

void MprisAdapter::setShuffle(bool s) {
  m_shuffle = s;
  QVariantMap changed;
  changed.insert(QStringLiteral("Shuffle"), m_shuffle);
  emitPropertiesChanged(QStringLiteral("org.mpris.MediaPlayer2.Player"),
                        changed);
}

void MprisAdapter::setVolume(double vol) {
  float fVol = static_cast<float>(std::clamp(vol, 0.0, 1.0));
  m_volume = fVol;
  emit setVolumeRequested(fVol);
  QVariantMap changed;
  changed.insert(QStringLiteral("Volume"), m_volume);
  emitPropertiesChanged(QStringLiteral("org.mpris.MediaPlayer2.Player"),
                        changed);
}

qlonglong MprisAdapter::position() const {
  return static_cast<qlonglong>(m_positionMs * 1000);
}

void MprisAdapter::triggerSeek(qlonglong offsetUs) {
  qint64 offsetMs = offsetUs / 1000;
  qint64 newPosMs = std::max<qint64>(0, m_positionMs + offsetMs);
  m_positionMs = newPosMs;
  emit seekRequested(newPosMs);

#if defined(PICAUDIO_ENABLE_DBUS)
  if (m_playerAdaptor) {
    emit m_playerAdaptor->Seeked(newPosMs * 1000);
  }
#endif
}

void MprisAdapter::triggerSetPosition(const QString &trackId,
                                      qlonglong positionUs) {
  if (trackId == m_currentTrackId || trackId.isEmpty()) {
    qint64 posMs = std::max<qint64>(0, positionUs / 1000);
    m_positionMs = posMs;
    emit seekRequested(posMs);

#if defined(PICAUDIO_ENABLE_DBUS)
    if (m_playerAdaptor) {
      emit m_playerAdaptor->Seeked(posMs * 1000);
    }
#endif
  }
}

void MprisAdapter::emitPropertiesChanged(const QString &interface,
                                         const QVariantMap &changedProperties) {
#if defined(PICAUDIO_ENABLE_DBUS)
  if (!m_registered || changedProperties.isEmpty()) {
    return;
  }

  QDBusMessage signal = QDBusMessage::createSignal(
      QStringLiteral("/org/mpris/MediaPlayer2"),
      QStringLiteral("org.freedesktop.DBus.Properties"),
      QStringLiteral("PropertiesChanged"));

  signal << interface;
  signal << changedProperties;
  signal << QStringList{}; // empty invalidated properties

  QDBusConnection::sessionBus().send(signal);
#else
  Q_UNUSED(interface);
  Q_UNUSED(changedProperties);
#endif
}

} // namespace adapters
