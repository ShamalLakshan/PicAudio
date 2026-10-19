#pragma once

#include "PlatformMediaControls.hpp"
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

#if defined(PICAUDIO_ENABLE_DBUS)
#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusObjectPath>
#endif

namespace adapters {

#if defined(PICAUDIO_ENABLE_DBUS)

class MprisAdapter;

/**
 * @brief D-Bus adaptor for org.mpris.MediaPlayer2 root interface.
 */
class MprisRootAdaptor : public QDBusAbstractAdaptor {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2")
  Q_PROPERTY(bool CanQuit READ canQuit)
  Q_PROPERTY(bool Fullscreen READ fullscreen WRITE setFullscreen)
  Q_PROPERTY(bool CanSetFullscreen READ canSetFullscreen)
  Q_PROPERTY(bool CanRaise READ canRaise)
  Q_PROPERTY(bool HasTrackList READ hasTrackList)
  Q_PROPERTY(QString Identity READ identity)
  Q_PROPERTY(QString DesktopEntry READ desktopEntry)
  Q_PROPERTY(QStringList SupportedUriSchemes READ supportedUriSchemes)
  Q_PROPERTY(QStringList SupportedMimeTypes READ supportedMimeTypes)

public:
  explicit MprisRootAdaptor(MprisAdapter *parent);

  [[nodiscard]] bool canQuit() const { return true; }
  [[nodiscard]] bool fullscreen() const { return false; }
  void setFullscreen(bool) {}
  [[nodiscard]] bool canSetFullscreen() const { return false; }
  [[nodiscard]] bool canRaise() const { return true; }
  [[nodiscard]] bool hasTrackList() const { return false; }
  [[nodiscard]] QString identity() const;
  [[nodiscard]] QString desktopEntry() const;
  [[nodiscard]] QStringList supportedUriSchemes() const;
  [[nodiscard]] QStringList supportedMimeTypes() const;

public slots:
  void Raise();
  void Quit();

private:
  MprisAdapter *m_adapter;
};

/**
 * @brief D-Bus adaptor for org.mpris.MediaPlayer2.Player interface.
 */
class MprisPlayerAdaptor : public QDBusAbstractAdaptor {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player")
  Q_PROPERTY(QString PlaybackStatus READ playbackStatus)
  Q_PROPERTY(QString LoopStatus READ loopStatus WRITE setLoopStatus)
  Q_PROPERTY(double Rate READ rate WRITE setRate)
  Q_PROPERTY(bool Shuffle READ shuffle WRITE setShuffle)
  Q_PROPERTY(QVariantMap Metadata READ metadata)
  Q_PROPERTY(double Volume READ volume WRITE setVolume)
  Q_PROPERTY(qlonglong Position READ position)
  Q_PROPERTY(double MinimumRate READ minimumRate)
  Q_PROPERTY(double MaximumRate READ maximumRate)
  Q_PROPERTY(bool CanGoNext READ canGoNext)
  Q_PROPERTY(bool CanGoPrevious READ canGoPrevious)
  Q_PROPERTY(bool CanPlay READ canPlay)
  Q_PROPERTY(bool CanPause READ canPause)
  Q_PROPERTY(bool CanSeek READ canSeek)
  Q_PROPERTY(bool CanControl READ canControl)

public:
  explicit MprisPlayerAdaptor(MprisAdapter *parent);

  [[nodiscard]] QString playbackStatus() const;
  [[nodiscard]] QString loopStatus() const;
  void setLoopStatus(const QString &loop);

  [[nodiscard]] double rate() const;
  void setRate(double r);

  [[nodiscard]] bool shuffle() const;
  void setShuffle(bool s);

  [[nodiscard]] QVariantMap metadata() const;
  [[nodiscard]] double volume() const;
  void setVolume(double vol);

  [[nodiscard]] qlonglong position() const;
  [[nodiscard]] double minimumRate() const { return 1.0; }
  [[nodiscard]] double maximumRate() const { return 1.0; }
  [[nodiscard]] bool canGoNext() const { return true; }
  [[nodiscard]] bool canGoPrevious() const { return true; }
  [[nodiscard]] bool canPlay() const { return true; }
  [[nodiscard]] bool canPause() const { return true; }
  [[nodiscard]] bool canSeek() const { return true; }
  [[nodiscard]] bool canControl() const { return true; }

public slots:
  void Next();
  void Previous();
  void Pause();
  void PlayPause();
  void Stop();
  void Play();
  void Seek(qlonglong Offset);
  void SetPosition(const QDBusObjectPath &TrackId, qlonglong Position);
  void OpenUri(const QString &Uri);

signals:
  void Seeked(qlonglong Position);

private:
  MprisAdapter *m_adapter;
};

#endif // PICAUDIO_ENABLE_DBUS

/**
 * @brief Linux MPRIS2 D-Bus adapter for desktop media keys and metadata.
 */
class MprisAdapter : public PlatformMediaControls {
  Q_OBJECT

public:
  explicit MprisAdapter(QObject *parent = nullptr);
  ~MprisAdapter() override;

  bool initialize() override;
  void updateMetadata(const QString &title, const QString &artist,
                      const QString &album, const QString &artUrl,
                      qint64 durationMs, int trackNumber, const QString &genre,
                      const QString &filePath) override;
  void updatePlaybackState(bool isPlaying, qint64 positionMs) override;
  void updatePosition(qint64 positionMs) override;
  void updateVolume(float volume) override;
  void updateModes(int repeatMode, int shuffleMode) override;

  [[nodiscard]] QString playbackStatus() const { return m_playbackStatus; }
  [[nodiscard]] QString loopStatus() const { return m_loopStatus; }
  void setLoopStatus(const QString &loop);
  [[nodiscard]] double rate() const { return m_rate; }
  void setRate(double r);
  [[nodiscard]] bool shuffle() const { return m_shuffle; }
  void setShuffle(bool s);
  [[nodiscard]] QVariantMap metadata() const { return m_metadata; }
  [[nodiscard]] double volume() const { return m_volume; }
  void setVolume(double vol);
  [[nodiscard]] qlonglong position() const;

  void triggerSeek(qlonglong offsetUs);
  void triggerSetPosition(const QString &trackId, qlonglong positionUs);

#if defined(PICAUDIO_ENABLE_DBUS)
  MprisPlayerAdaptor *playerAdaptor() const { return m_playerAdaptor; }
#endif

private:
  void emitPropertiesChanged(const QString &interface,
                             const QVariantMap &changedProperties);

  QString m_playbackStatus{"Stopped"};
  QString m_loopStatus{"None"};
  double m_rate{1.0};
  bool m_shuffle{false};
  QVariantMap m_metadata;
  double m_volume{1.0};
  qint64 m_positionMs{0};
  qint64 m_durationMs{0};
  QString m_currentTrackId{"/org/mpris/MediaPlayer2/Track/0"};
  bool m_registered{false};

#if defined(PICAUDIO_ENABLE_DBUS)
  MprisRootAdaptor *m_rootAdaptor{nullptr};
  MprisPlayerAdaptor *m_playerAdaptor{nullptr};
#endif
};

} // namespace adapters
