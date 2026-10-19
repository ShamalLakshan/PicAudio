#include "QtBridge.hpp"
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeySequence>
#include <QQmlApplicationEngine>
#include <QRegularExpression>
#include <QScreen>
#include <QSettings>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTextStream>
#include <QTranslator>
#include <QUrl>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <set>

QtBridge::QtBridge(core::PlayerService &player, core::LibraryService &library,
                   adapters::ThemeLoader *themeLoader, QObject *parent)
    : QObject(parent), m_player(player), m_library(library),
      m_themeLoader(themeLoader) {

  // Initialize hotkeys registry
  initDefaultHotkeys();

  // Remove missing tracks on startup
  m_library.purgeNonExistentTracks();

  m_albumModel = new AlbumListModel(&m_library.getDatabasePort(), this);
  m_trackModel = new TrackListModel(&m_library.getDatabasePort(), this);
  m_albumDetailTrackModel =
      new TrackListModel(&m_library.getDatabasePort(), this);
  m_queueTrackModel = new TrackListModel(&m_library.getDatabasePort(), this);
  m_historyTrackModel = new TrackListModel(&m_library.getDatabasePort(), this);

  m_mediaDevices = new QMediaDevices(this);
  connect(m_mediaDevices, &QMediaDevices::audioOutputsChanged, this,
          &QtBridge::refreshAudioDevices);
  refreshAudioDevices();

  if (m_themeLoader) {
    connect(m_themeLoader, &adapters::ThemeLoader::themeChanged, this,
            [this]() {
              QSettings s("PicAudio", "PicAudio");
              s.setValue("appearance/themeId", m_themeLoader->themeId());
            });
    connect(m_themeLoader, &adapters::ThemeLoader::uiScaleChanged, this,
            [this]() {
              QSettings s("PicAudio", "PicAudio");
              s.setValue("appearance/uiScale", m_themeLoader->uiScale());
              s.setValue("appearance/densityPreset",
                         m_themeLoader->densityPreset());
            });
  }

  initTableColumns();
  loadColumnSettings();
  loadViewLayoutSettings();
  loadSettings();
  refreshLibraryStats();
  refreshHistory();
  setupPositionTimer();
  setupVisualizerTimer();

  connect(m_trackModel, &TrackListModel::sortChanged, this,
          &QtBridge::saveColumnSettings);

  if (m_autoScanOnStartup && !m_monitoredFolders.isEmpty()) {
    rescanAllMonitoredFolders();
  }
}

QtBridge::~QtBridge() {
  savePlaybackSession();
  saveViewLayoutSettings();
  saveColumnSettings();
  saveSettings();
}

void QtBridge::loadSettings() {
  QSettings s("PicAudio", "PicAudio");

  // Playback & DSP
  m_gaplessPlayback = s.value("playback/gaplessPlayback", true).toBool();
  m_crossfadeEnabled = s.value("playback/crossfadeEnabled", false).toBool();
  m_crossfadeDurationSec =
      s.value("playback/crossfadeDurationSec", 2.0).toReal();
  m_crossfadeCurve =
      s.value("playback/crossfadeCurve", "Equal Power (Constant Volume)")
          .toString();
  m_replayGainMode =
      s.value("playback/replayGainMode", "Smart Gain (Auto Track/Album)")
          .toString();
  m_replayGainPreampDb = s.value("playback/replayGainPreampDb", 0).toInt();
  m_replayGainPreampWithoutGainDb =
      s.value("playback/replayGainPreampWithoutGainDb", -6).toInt();
  m_truePeakLimiter = s.value("playback/truePeakLimiter", true).toBool();
  m_shortSeekStepSec = s.value("playback/shortSeekStepSec", 5).toInt();
  m_longSeekStepSec = s.value("playback/longSeekStepSec", 30).toInt();
  m_stopAfterCurrentTrack =
      s.value("playback/stopAfterCurrentTrack", false).toBool();
  m_showRemainingTime = s.value("playback/showRemainingTime", false).toBool();
  m_volumeLocked = s.value("playback/volumeLocked", false).toBool();

  // Queue & Ergonomics
  m_doubleClickAction =
      s.value("playback/doubleClickAction", "Play Now").toString();
  m_middleClickAction =
      s.value("playback/middleClickAction", "Queue Last").toString();
  m_queueAutoFillMode =
      s.value("playback/queueAutoFillMode", "Loop Context").toString();
  m_historyRetentionLimit =
      s.value("playback/historyRetentionLimit", 200).toInt();
  m_player.setHistoryLimit(static_cast<size_t>(m_historyRetentionLimit));

  int rMode = s.value("playback/repeatMode", 0).toInt();
  int sMode = s.value("playback/shuffleMode", 0).toInt();
  m_player.setRepeatMode(static_cast<core::RepeatMode>(rMode));
  m_player.setShuffleMode(static_cast<core::ShuffleMode>(sMode));

  // Library & Folders
  m_monitoredFolders = s.value("library/monitoredFolders").toStringList();
  m_filesystemWatcher = s.value("library/filesystemWatcher", true).toBool();
  m_autoScanOnStartup = s.value("library/autoScanOnStartup", false).toBool();
  QStringList defaultFormats = {"FLAC", "WAV", "ALAC", "AIFF", "DSD (DSF/DFF)",
                                "MP3",  "AAC", "M4A",  "OGG",  "OPUS"};
  m_formatFilters =
      s.value("library/formatFilters", defaultFormats).toStringList();
  m_excludeFolders =
      s.value("library/excludeFolders",
              ".*, node_modules, temp, @eaDir, System Volume Information")
          .toString();
  m_artworkPriority =
      s.value("library/artworkPriority", "Embedded Tags First").toString();
  m_library.getMetadataExtractor().setArtworkPriority(
      m_artworkPriority != "External Folder Art First");
  m_groupAlbumsByTitle = s.value("library/groupAlbumsByTitle", true).toBool();
  m_library.getDatabasePort().setGroupAlbumsByTitle(m_groupAlbumsByTitle);

  // General
  m_language = s.value("general/language", "System Default").toString();
  m_startupAction =
      s.value("general/startupAction", "Restore Session").toString();
  m_closeAction = s.value("general/closeAction", "Exit Application").toString();
  m_minimizeAction =
      s.value("general/minimizeAction", "Minimize to Taskbar").toString();
  m_singleInstance = s.value("general/singleInstance", true).toBool();
  m_updateCheckInterval =
      s.value("general/updateCheckInterval", "Weekly").toString();
  m_showNotifications = s.value("general/showNotifications", true).toBool();
  m_notificationDurationSec =
      s.value("general/notificationDurationSec", 4).toInt();
  m_suppressNotificationsWhenFocused =
      s.value("general/suppressNotificationsWhenFocused", true).toBool();

  // Appearance
  m_fontFamily =
      s.value("appearance/fontFamily", "Inter, sans-serif").toString();
  m_baseFontSize = s.value("appearance/baseFontSize", 11).toInt();
  m_waveformSeekbar = s.value("appearance/waveformSeekbar", true).toBool();
  m_tableRowHeight = s.value("appearance/tableRowHeight", 32).toInt();
  m_tableAlternatingRows =
      s.value("appearance/tableAlternatingRows", false).toBool();
  m_artThumbnailQuality =
      s.value("appearance/artThumbnailQuality", "Smooth (High Quality)")
          .toString();
  m_artCacheLimitMb = s.value("appearance/artCacheLimitMb", 1024).toInt();
  m_showStatusBar = s.value("appearance/showStatusBar", true).toBool();

  if (m_themeLoader) {
    QString savedTheme =
        s.value("appearance/themeId", "dark-studio").toString();
    m_themeLoader->setThemeId(savedTheme);
    qreal savedScale = s.value("appearance/uiScale", 1.0).toReal();
    m_themeLoader->setUiScale(savedScale);
    QString savedDensity =
        s.value("appearance/densityPreset", "Standard").toString();
    m_themeLoader->setDensityPreset(savedDensity);
  }

  // Audio Output
  m_audioBackend =
      s.value("audio/backend", "Linux PipeWire Lock-Free Client").toString();
  m_currentAudioDevice =
      s.value("audio/device", "System Default Output").toString();
  m_bitPerfectExclusive = s.value("audio/bitPerfectExclusive", true).toBool();
  m_bufferLatencyMs = s.value("audio/bufferLatencyMs", 50).toInt();
  m_resamplerQuality =
      s.value("audio/resamplerQuality", "SoX Resampler High Quality")
          .toString();
  m_ditherMode =
      s.value("audio/ditherMode", "Flat TPDF (Triangular)").toString();
  m_channelProcessing =
      s.value("audio/channelProcessing", "Stereo Passthrough").toString();

  // Hotkeys & Accelerators
  m_globalMediaKeysEnabled =
      s.value("hotkeys/globalMediaKeysEnabled", true).toBool();
  for (auto &hk : m_hotkeys) {
    hk.currentSequence =
        s.value("hotkeys/" + hk.id, hk.defaultSequence).toString();
  }

  // Metadata & Scrobbling
  m_lastfmEnabled = s.value("metadata/lastfmEnabled", false).toBool();
  m_lastfmUsername = s.value("metadata/lastfmUsername", "").toString();
  m_lastfmSessionKey = s.value("metadata/lastfmSessionKey", "").toString();
  m_listenbrainzEnabled =
      s.value("metadata/listenbrainzEnabled", false).toBool();
  m_listenbrainzToken = s.value("metadata/listenbrainzToken", "").toString();
  m_listenbrainzApiUrl =
      s.value("metadata/listenbrainzApiUrl", "https://api.listenbrainz.org/1/")
          .toString();
  m_scrobbleThresholdPercent =
      s.value("metadata/scrobbleThresholdPercent", 50).toInt();
  m_scrobbleThresholdTimeSec =
      s.value("metadata/scrobbleThresholdTimeSec", 240).toInt();
  m_offlineScrobbleCache =
      s.value("metadata/offlineScrobbleCache", true).toBool();
  m_lyricsProviderOrder =
      s.value("metadata/lyricsProviderOrder", "Local .lrc sidecar first")
          .toString();
  m_autoFetchLyrics = s.value("metadata/autoFetchLyrics", true).toBool();

  // Diagnostics
  m_loggingVerbosity =
      s.value("diagnostics/loggingVerbosity", "Info").toString();
}

void QtBridge::saveSettings() {
  QSettings s("PicAudio", "PicAudio");

  // Playback & DSP
  s.setValue("playback/gaplessPlayback", m_gaplessPlayback);
  s.setValue("playback/crossfadeEnabled", m_crossfadeEnabled);
  s.setValue("playback/crossfadeDurationSec", m_crossfadeDurationSec);
  s.setValue("playback/crossfadeCurve", m_crossfadeCurve);
  s.setValue("playback/replayGainMode", m_replayGainMode);
  s.setValue("playback/replayGainPreampDb", m_replayGainPreampDb);
  s.setValue("playback/replayGainPreampWithoutGainDb",
             m_replayGainPreampWithoutGainDb);
  s.setValue("playback/truePeakLimiter", m_truePeakLimiter);
  s.setValue("playback/shortSeekStepSec", m_shortSeekStepSec);
  s.setValue("playback/longSeekStepSec", m_longSeekStepSec);
  s.setValue("playback/stopAfterCurrentTrack", m_stopAfterCurrentTrack);
  s.setValue("playback/showRemainingTime", m_showRemainingTime);
  s.setValue("playback/volumeLocked", m_volumeLocked);

  // Queue & Ergonomics
  s.setValue("playback/doubleClickAction", m_doubleClickAction);
  s.setValue("playback/middleClickAction", m_middleClickAction);
  s.setValue("playback/queueAutoFillMode", m_queueAutoFillMode);
  s.setValue("playback/historyRetentionLimit", m_historyRetentionLimit);
  s.setValue("playback/repeatMode", static_cast<int>(m_player.getRepeatMode()));
  s.setValue("playback/shuffleMode",
             static_cast<int>(m_player.getShuffleMode()));

  // Library & Folders
  s.setValue("library/monitoredFolders", m_monitoredFolders);
  s.setValue("library/filesystemWatcher", m_filesystemWatcher);
  s.setValue("library/autoScanOnStartup", m_autoScanOnStartup);
  s.setValue("library/formatFilters", m_formatFilters);
  s.setValue("library/excludeFolders", m_excludeFolders);
  s.setValue("library/artworkPriority", m_artworkPriority);
  s.setValue("library/groupAlbumsByTitle", m_groupAlbumsByTitle);

  // General
  s.setValue("general/language", m_language);
  s.setValue("general/startupAction", m_startupAction);
  s.setValue("general/closeAction", m_closeAction);
  s.setValue("general/minimizeAction", m_minimizeAction);
  s.setValue("general/singleInstance", m_singleInstance);
  s.setValue("general/updateCheckInterval", m_updateCheckInterval);
  s.setValue("general/showNotifications", m_showNotifications);
  s.setValue("general/notificationDurationSec", m_notificationDurationSec);
  s.setValue("general/suppressNotificationsWhenFocused",
             m_suppressNotificationsWhenFocused);

  // Appearance
  s.setValue("appearance/fontFamily", m_fontFamily);
  s.setValue("appearance/baseFontSize", m_baseFontSize);
  s.setValue("appearance/waveformSeekbar", m_waveformSeekbar);
  s.setValue("appearance/tableRowHeight", m_tableRowHeight);
  s.setValue("appearance/tableAlternatingRows", m_tableAlternatingRows);
  s.setValue("appearance/artThumbnailQuality", m_artThumbnailQuality);
  s.setValue("appearance/artCacheLimitMb", m_artCacheLimitMb);
  s.setValue("appearance/showStatusBar", m_showStatusBar);

  if (m_themeLoader) {
    s.setValue("appearance/themeId", m_themeLoader->themeId());
    s.setValue("appearance/uiScale", m_themeLoader->uiScale());
    s.setValue("appearance/densityPreset", m_themeLoader->densityPreset());
  }

  // Audio Output
  s.setValue("audio/backend", m_audioBackend);
  s.setValue("audio/device", m_currentAudioDevice);
  s.setValue("audio/bitPerfectExclusive", m_bitPerfectExclusive);
  s.setValue("audio/bufferLatencyMs", m_bufferLatencyMs);
  s.setValue("audio/resamplerQuality", m_resamplerQuality);
  s.setValue("audio/ditherMode", m_ditherMode);
  s.setValue("audio/channelProcessing", m_channelProcessing);

  // Hotkeys
  s.setValue("hotkeys/globalMediaKeysEnabled", m_globalMediaKeysEnabled);
  for (const auto &hk : m_hotkeys) {
    s.setValue("hotkeys/" + hk.id, hk.currentSequence);
  }

  // Metadata & Scrobbling
  s.setValue("metadata/lastfmEnabled", m_lastfmEnabled);
  s.setValue("metadata/lastfmUsername", m_lastfmUsername);
  s.setValue("metadata/lastfmSessionKey", m_lastfmSessionKey);
  s.setValue("metadata/listenbrainzEnabled", m_listenbrainzEnabled);
  s.setValue("metadata/listenbrainzToken", m_listenbrainzToken);
  s.setValue("metadata/listenbrainzApiUrl", m_listenbrainzApiUrl);
  s.setValue("metadata/scrobbleThresholdPercent", m_scrobbleThresholdPercent);
  s.setValue("metadata/scrobbleThresholdTimeSec", m_scrobbleThresholdTimeSec);
  s.setValue("metadata/offlineScrobbleCache", m_offlineScrobbleCache);
  s.setValue("metadata/lyricsProviderOrder", m_lyricsProviderOrder);
  s.setValue("metadata/autoFetchLyrics", m_autoFetchLyrics);

  // Diagnostics
  s.setValue("diagnostics/loggingVerbosity", m_loggingVerbosity);

  emit settingsChanged();
  emit playbackSettingsChanged();
  emit queueSettingsChanged();
  emit librarySettingsChanged();
  emit audioSettingsChanged();
  emit hotkeysChanged();
  emit metadataSettingsChanged();
  emit diagnosticsSettingsChanged();
}

void QtBridge::setupPositionTimer() {
  m_positionTimer = new QTimer(this);
  m_positionTimer->setInterval(250);
  connect(m_positionTimer, &QTimer::timeout, this, [this]() {
    if (m_isPlaying) {
      uint64_t realPos = m_player.getPositionMs();
      m_positionMs = static_cast<qint64>(realPos);
      m_positionStr = formatTime(m_positionMs);

      // A-B Looping check
      if (m_loopPointA >= 0 && m_loopPointB > m_loopPointA &&
          m_positionMs >= m_loopPointB) {
        seek(m_loopPointA);
      }

      emit positionChanged();
      updateActiveLyricIndex();

      // Check duration
      uint64_t realDur = m_player.getDurationMs();
      if (realDur > 0 && static_cast<qint64>(realDur) != m_durationMs) {
        m_durationMs = static_cast<qint64>(realDur);
        m_durationStr = formatTime(m_durationMs);
        emit playbackChanged();
      }
    }
  });
}

void QtBridge::setupVisualizerTimer() {
  m_visualizerTimer = new QTimer(this);
  m_visualizerTimer->setInterval(33); // ~30-60 FPS smooth updates
  connect(m_visualizerTimer, &QTimer::timeout, this,
          &QtBridge::updateVisualizerData);
  m_visualizerTimer->start();
}

void QtBridge::updateVisualizerData() {
  auto bands = m_player.getFrequencyBands(36);
  auto wave = m_player.getWaveformSamples(64);
  float level = m_player.getAudioLevel();

  bool hasEnergy = (level > 0.001f);
  if (!hasEnergy && !m_isPlaying && m_visualizerLevel < 0.001f &&
      m_visualizerBands.isEmpty()) {
    return;
  }

  m_visualizerBands.clear();
  m_visualizerBands.reserve(static_cast<qsizetype>(bands.size()));
  for (float b : bands) {
    m_visualizerBands.append(b);
  }

  m_visualizerWaveform.clear();
  m_visualizerWaveform.reserve(static_cast<qsizetype>(wave.size()));
  for (float w : wave) {
    m_visualizerWaveform.append(w);
  }

  m_visualizerLevel = level;
  emit visualizerDataChanged();
}

QString QtBridge::formatTime(qint64 ms) {
  if (ms < 0)
    return "00:00";
  qint64 totalSec = ms / 1000;
  qint64 hrs = totalSec / 3600;
  qint64 min = (totalSec % 3600) / 60;
  qint64 sec = totalSec % 60;
  char buf[32];
  if (hrs > 0) {
    std::snprintf(buf, sizeof(buf), "%lld:%02lld:%02lld",
                  static_cast<long long>(hrs), static_cast<long long>(min),
                  static_cast<long long>(sec));
  } else {
    std::snprintf(buf, sizeof(buf), "%02lld:%02lld",
                  static_cast<long long>(min), static_cast<long long>(sec));
  }
  return QString::fromUtf8(buf);
}

QString QtBridge::formatBytes(uint64_t bytes) {
  if (bytes == 0)
    return "0 MB";
  double mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
  if (mb >= 1024.0) {
    double gb = mb / 1024.0;
    return QString::asprintf("%.2f GB", gb);
  }
  return QString::asprintf("%.1f MB", mb);
}

QString QtBridge::currentArtUrl() const {
  if (m_currentArtHash.isEmpty()) {
    if (!m_currentFilePath.isEmpty()) {
      return "image://albumart/" + m_currentFilePath;
    }
    return "image://albumart/default";
  }
  return "image://albumart/" + m_currentArtHash;
}

int QtBridge::repeatMode() const {
  return static_cast<int>(m_player.getRepeatMode());
}

int QtBridge::shuffleMode() const {
  return static_cast<int>(m_player.getShuffleMode());
}

int QtBridge::queueRemainingCount() const {
  return static_cast<int>(m_player.getQueueService().getRemainingCount());
}

QString QtBridge::queueRemainingDurationStr() const {
  return formatTime(
      static_cast<qint64>(m_player.getQueueService().getRemainingDurationMs()));
}

int QtBridge::upNextCount() const {
  return static_cast<int>(m_player.getQueueService().getUpNext().size());
}

QStringList QtBridge::availablePlaylists() const {
  QString dirPath =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
      "/playlists";
  QDir dir(dirPath);
  if (!dir.exists()) {
    return QStringList();
  }
  QStringList filters;
  filters << "*.m3u8" << "*.m3u" << "*.smart.json";
  QStringList files = dir.entryList(filters, QDir::Files, QDir::Name);
  QStringList playlistNames;
  for (const auto &f : files) {
    if (f.endsWith(".smart.json")) {
      QString base = f;
      base.chop(11);
      playlistNames.append(base);
    } else {
      QFileInfo fi(f);
      playlistNames.append(fi.completeBaseName());
    }
  }
  playlistNames.removeDuplicates();
  return playlistNames;
}

int QtBridge::totalAlbums() const {
  return static_cast<int>(m_library.getDatabasePort().getAlbumCount());
}

int QtBridge::totalTracks() const {
  return static_cast<int>(m_library.getDatabasePort().getTrackCount());
}

QString QtBridge::totalDurationStr() const {
  auto all = m_library.getTracks();
  uint64_t totalMs = 0;
  for (const auto &t : all)
    totalMs += t.durationMs;
  uint64_t totalSec = totalMs / 1000;
  uint64_t hours = totalSec / 3600;
  uint64_t min = (totalSec % 3600) / 60;
  uint64_t sec = totalSec % 60;
  if (hours > 0) {
    return QString::asprintf("%lluh %02llum %02llus", (unsigned long long)hours,
                             (unsigned long long)min, (unsigned long long)sec);
  }
  return QString::asprintf("%llum %02llus", (unsigned long long)min,
                           (unsigned long long)sec);
}

QString QtBridge::totalLibrarySizeStr() const {
  auto all = m_library.getTracks();
  uint64_t totalBytes = 0;
  for (const auto &t : all)
    totalBytes += t.fileSizeBytes;
  return formatBytes(totalBytes);
}

void QtBridge::refreshLibraryStats() {
  auto all = m_library.getTracks();
  std::set<std::string> genres;
  std::set<std::string> artists;
  for (const auto &t : all) {
    if (!t.genre.empty())
      genres.insert(t.genre);
    if (!t.artist.empty())
      artists.insert(t.artist);
  }

  m_genresList.clear();
  for (const auto &g : genres)
    m_genresList.append(QString::fromStdString(g));

  m_artistsList.clear();
  for (const auto &a : artists)
    m_artistsList.append(QString::fromStdString(a));

  emit libraryStatsChanged();
}

void QtBridge::syncQueueModel() {
  auto upcoming = m_player.getQueueService().getUpcomingQueue();
  m_queueTrackModel->setTracks(upcoming);
  emit queueChanged();
}

void QtBridge::scanDirectory(const QString &folderPath) {
  if (m_isScanning)
    return;

  QString localPath = folderPath;
  QUrl url = QUrl::fromUserInput(folderPath);
  if (url.isLocalFile()) {
    localPath = url.toLocalFile();
  } else if (localPath.startsWith("file://")) {
    localPath = localPath.mid(7);
  }

  std::string path = localPath.toStdString();
  m_isScanning = true;
  m_scanStatusText = QString("Indexing: %1...").arg(localPath);
  m_scanScanned = 0;
  m_scanTotal = 0;
  emit scanningChanged();

  std::thread([this, path]() {
    size_t count = m_library.scanDirectory(path, [this](size_t scanned,
                                                        size_t total) {
      QMetaObject::invokeMethod(
          this,
          [this, scanned, total]() {
            m_scanScanned = static_cast<int>(scanned);
            m_scanTotal = static_cast<int>(total);
            m_scanStatusText =
                QString("Indexed %1 / %2 tracks...").arg(scanned).arg(total);
            emit scanningChanged();
          },
          Qt::QueuedConnection);
    });

    QMetaObject::invokeMethod(
        this,
        [this, count]() {
          m_albumModel->reload();
          m_trackModel->loadAllTracks();
          refreshLibraryStats();
          m_isScanning = false;
          m_scanStatusText =
              QString("Scan complete: %1 tracks indexed").arg(count);
          emit scanningChanged();
          emit scanFinished(static_cast<int>(count));
        },
        Qt::QueuedConnection);
  }).detach();
}

void QtBridge::purgeMissingTracks() {
  size_t count = m_library.purgeNonExistentTracks();
  m_albumModel->reload();
  m_trackModel->loadAllTracks();
  refreshLibraryStats();
  syncQueueModel();
  m_scanStatusText = QString("Purged %1 non-existent tracks").arg(count);
  emit scanningChanged();
}

void QtBridge::clearLibrary() {
  m_library.clearLibrary();
  m_player.clearQueue();
  m_queueTrackModel->setTracks({});
  m_albumModel->reload();
  m_trackModel->loadAllTracks();
  refreshLibraryStats();
  syncQueueModel();
  m_scanStatusText = "Library cleared";
  emit scanningChanged();
}

void QtBridge::search(const QString &query) {
  m_albumModel->setFilter(query);
  m_trackModel->setFilter(query);
}

void QtBridge::filterByLetter(const QString &letter) {
  if (letter.isEmpty() || letter == "All") {
    m_albumModel->setFilter("");
    m_trackModel->setFilter("");
  } else {
    m_albumModel->setFilter(letter);
    m_trackModel->setFilter(letter);
  }
}

void QtBridge::filterByGenre(const QString &genre) {
  if (genre.isEmpty() || genre == "All") {
    m_trackModel->setFilter("");
  } else {
    m_trackModel->setFilter(genre);
  }
}

void QtBridge::filterByArtist(const QString &artist) {
  if (artist.isEmpty() || artist == "All") {
    m_trackModel->setFilter("");
    m_albumModel->setFilter("");
  } else {
    m_trackModel->setFilter(artist);
    m_albumModel->setFilter(artist);
  }
}

void QtBridge::resetFilters() {
  m_albumModel->setFilter("");
  m_trackModel->setFilter("");
}

void QtBridge::openAlbumDetails(const QString &albumTitle,
                                const QString &albumArtist) {
  auto albumOpt = m_library.getDatabasePort().getAlbumById(
      "alb_" + albumTitle.toStdString() + "_" + albumArtist.toStdString());
  if (albumOpt) {
    const auto &a = *albumOpt;
    m_selectedAlbumTitle = QString::fromStdString(a.title);
    m_selectedAlbumArtist = QString::fromStdString(a.artist);
    m_selectedAlbumYear = a.year > 0 ? QString::number(a.year) : "";
    m_selectedAlbumGenre = QString::fromStdString(a.genre);
    m_selectedAlbumQuality = QString::fromStdString(a.qualityBadge);
    m_selectedAlbumDuration = QString::fromStdString(a.durationFormatted());
    m_selectedAlbumTrackCount = static_cast<int>(a.trackCount);
    m_selectedAlbumArtHash = QString::fromStdString(a.artHash);
  } else {
    m_selectedAlbumTitle = albumTitle;
    m_selectedAlbumArtist = albumArtist;
    m_selectedAlbumYear = "";
    m_selectedAlbumGenre = "";
    m_selectedAlbumQuality = "Lossless";
    m_selectedAlbumDuration = "";
    m_selectedAlbumTrackCount = 0;
    m_selectedAlbumArtHash = "";
  }

  m_albumDetailTrackModel->loadAlbumTracks(albumTitle, albumArtist);
  emit selectedAlbumChanged();
}

void QtBridge::playAlbum(const QString &albumTitle,
                         const QString &albumArtist) {
  playAlbumNow(albumTitle, albumArtist);
}

void QtBridge::playTrack(const QString &trackId) { playNow(trackId); }

void QtBridge::playNow(const QString &trackId) {
  playSelectionNow(QStringList{trackId});
}

void QtBridge::playSelectionNow(const QStringList &selectedIds) {
  if (selectedIds.isEmpty())
    return;

  auto allTracks = m_trackModel->tracks();
  if (allTracks.empty()) {
    allTracks = m_library.getTracks();
  }

  std::vector<core::Track> selectedTracks;
  std::vector<core::Track> remainingTracks;
  std::unordered_set<std::string> selectedSet;
  for (const auto &id : selectedIds) {
    selectedSet.insert(id.toStdString());
  }

  for (const auto &id : selectedIds) {
    for (const auto &t : allTracks) {
      if (t.id == id.toStdString()) {
        selectedTracks.push_back(t);
        break;
      }
    }
  }

  if (selectedTracks.size() < static_cast<size_t>(selectedIds.size())) {
    for (const auto &id : selectedIds) {
      bool found = false;
      for (const auto &st : selectedTracks) {
        if (st.id == id.toStdString()) {
          found = true;
          break;
        }
      }
      if (!found) {
        auto opt = m_library.getTrackById(id.toStdString());
        if (opt)
          selectedTracks.push_back(*opt);
      }
    }
  }

  for (const auto &t : allTracks) {
    if (selectedSet.find(t.id) == selectedSet.end()) {
      remainingTracks.push_back(t);
    }
  }

  std::vector<core::Track> newQueue;
  newQueue.reserve(selectedTracks.size() + remainingTracks.size());
  newQueue.insert(newQueue.end(), selectedTracks.begin(), selectedTracks.end());
  newQueue.insert(newQueue.end(), remainingTracks.begin(),
                  remainingTracks.end());

  if (!newQueue.empty()) {
    m_player.playQueue(newQueue, 0);
    updatePlaybackState(newQueue[0]);
    syncQueueModel();
  }
}

void QtBridge::playNext(const QString &trackId) {
  playTracksNext(QStringList{trackId});
}

void QtBridge::playTracksNext(const QStringList &trackIds) {
  if (trackIds.isEmpty())
    return;
  std::vector<core::Track> tracks;
  for (const auto &id : trackIds) {
    auto opt = m_library.getTrackById(id.toStdString());
    if (opt) {
      tracks.push_back(*opt);
    }
  }
  if (!tracks.empty()) {
    m_player.playNext(tracks);
    syncQueueModel();
  }
}

void QtBridge::queueLast(const QString &trackId) {
  queueTracksLast(QStringList{trackId});
}

void QtBridge::queueTracksLast(const QStringList &trackIds) {
  if (trackIds.isEmpty())
    return;
  std::vector<core::Track> tracks;
  for (const auto &id : trackIds) {
    auto opt = m_library.getTrackById(id.toStdString());
    if (opt) {
      tracks.push_back(*opt);
    }
  }
  if (!tracks.empty()) {
    m_player.queueLast(tracks);
    syncQueueModel();
  }
}

void QtBridge::playNextInAlbumShuffle(const QStringList &trackIds) {
  if (trackIds.isEmpty())
    return;
  std::vector<std::pair<std::string, std::string>> albums;
  std::unordered_set<std::string> seenAlbums;

  for (const auto &id : trackIds) {
    auto opt = m_library.getTrackById(id.toStdString());
    if (opt && !opt->album.empty()) {
      std::string key = opt->album + "|||" + opt->artist;
      if (seenAlbums.find(key) == seenAlbums.end()) {
        seenAlbums.insert(key);
        albums.emplace_back(opt->album, opt->artist);
      }
    }
  }

  std::vector<core::Track> allAlbumTracks;
  for (const auto &alb : albums) {
    auto trks = m_library.getTracksForAlbum(alb.first, alb.second);
    allAlbumTracks.insert(allAlbumTracks.end(), trks.begin(), trks.end());
  }

  if (!allAlbumTracks.empty()) {
    m_player.playNext(allAlbumTracks);
    syncQueueModel();
  }
}

void QtBridge::playAlbumNow(const QString &albumTitle,
                            const QString &albumArtist) {
  auto tracks = m_library.getTracksForAlbum(albumTitle.toStdString(),
                                            albumArtist.toStdString());
  if (!tracks.empty()) {
    m_player.playQueue(tracks, 0);
    updatePlaybackState(tracks[0]);
    syncQueueModel();
  }
}

void QtBridge::playAlbumNext(const QString &albumTitle,
                             const QString &albumArtist) {
  auto tracks = m_library.getTracksForAlbum(albumTitle.toStdString(),
                                            albumArtist.toStdString());
  if (!tracks.empty()) {
    m_player.playNext(tracks);
    syncQueueModel();
  }
}

void QtBridge::queueAlbumLast(const QString &albumTitle,
                              const QString &albumArtist) {
  auto tracks = m_library.getTracksForAlbum(albumTitle.toStdString(),
                                            albumArtist.toStdString());
  if (!tracks.empty()) {
    m_player.queueLast(tracks);
    syncQueueModel();
  }
}

bool QtBridge::addAlbumToPlaylist(const QString &albumTitle,
                                  const QString &albumArtist,
                                  const QString &playlistName) {
  auto tracks = m_library.getTracksForAlbum(albumTitle.toStdString(),
                                            albumArtist.toStdString());
  if (tracks.empty()) {
    return false;
  }
  QVariantList list;
  for (const auto &t : tracks) {
    QVariantMap map;
    map["id"] = QString::fromStdString(t.id);
    map["title"] = QString::fromStdString(t.title);
    map["artist"] = QString::fromStdString(t.artist);
    map["filePath"] = QString::fromStdString(t.filePath);
    map["durationMs"] = static_cast<qint64>(t.durationMs);
    list.append(map);
  }
  return addTracksToPlaylist(playlistName, list);
}

void QtBridge::showAlbumInFileManager(const QString &albumTitle,
                                      const QString &albumArtist) {
  auto tracks = m_library.getTracksForAlbum(albumTitle.toStdString(),
                                            albumArtist.toStdString());
  if (!tracks.empty()) {
    showInFileManager(QString::fromStdString(tracks[0].filePath));
  }
}

QVariantList QtBridge::getAlbumTracks(const QString &albumTitle,
                                      const QString &albumArtist) const {
  auto tracks = m_library.getTracksForAlbum(albumTitle.toStdString(),
                                            albumArtist.toStdString());
  QVariantList list;
  for (const auto &t : tracks) {
    QVariantMap map;
    map["id"] = QString::fromStdString(t.id);
    map["title"] = QString::fromStdString(t.title);
    map["artist"] = QString::fromStdString(t.artist);
    map["album"] = QString::fromStdString(t.album);
    map["genre"] = QString::fromStdString(t.genre);
    map["year"] = t.year;
    map["trackNumber"] = t.trackNumber;
    map["filePath"] = QString::fromStdString(t.filePath);
    map["durationMs"] = static_cast<qint64>(t.durationMs);
    map["durationFormatted"] = formatTime(t.durationMs);
    map["codec"] = QString::fromStdString(t.codec);
    map["sampleRate"] = t.sampleRate;
    map["bitDepth"] = t.bitDepth;
    map["bitrate"] = t.bitrate;
    list.append(map);
  }
  return list;
}

void QtBridge::searchAlbumArtworkOnline(const QString &albumTitle,
                                        const QString &albumArtist) {
  QString query = albumTitle + " " + albumArtist + " album cover art";
  QUrl url("https://musicbrainz.org/search?type=release&query=" +
           QUrl::toPercentEncoding(query));
  QDesktopServices::openUrl(url);
}

bool QtBridge::exportAlbumArtwork(const QString &albumTitle,
                                  const QString &albumArtist,
                                  const QString &targetFilePath) {
  auto tracks = m_library.getTracksForAlbum(albumTitle.toStdString(),
                                            albumArtist.toStdString());
  if (tracks.empty()) {
    tracks = m_library.getTracksForAlbum(albumTitle.toStdString(), "");
    if (tracks.empty()) {
      return false;
    }
  }
  QString clean = targetFilePath;
  if (clean.startsWith("file://")) {
    clean = QUrl(clean).toLocalFile();
  }
  auto art =
      m_library.getMetadataExtractor().extractArtwork(tracks[0].filePath);
  if (art && !art->data.empty()) {
    QFile file(clean);
    if (file.open(QIODevice::WriteOnly)) {
      file.write(reinterpret_cast<const char *>(art->data.data()),
                 art->data.size());
      file.close();
      return true;
    }
  }
  QString appDataDir =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QString cachedCover = appDataDir + "/covers/" +
                        QString::fromStdString(tracks[0].artHash) + ".jpg";
  if (QFile::exists(cachedCover)) {
    QFile::remove(clean);
    return QFile::copy(cachedCover, clean);
  }
  return false;
}

void QtBridge::removeFromQueue(int index) {
  if (index >= 0) {
    m_player.removeFromQueue(static_cast<size_t>(index));
    syncQueueModel();
  }
}

void QtBridge::moveQueueItem(int fromIndex, int toIndex) {
  if (fromIndex >= 0 && toIndex >= 0) {
    m_player.moveQueueItem(static_cast<size_t>(fromIndex),
                           static_cast<size_t>(toIndex));
    syncQueueModel();
  }
}

void QtBridge::clearQueue() {
  m_player.clearQueue();
  syncQueueModel();
}

void QtBridge::shuffleRemainingQueue() {
  m_player.shuffleRemaining();
  syncQueueModel();
}

bool QtBridge::saveQueueAsPlaylist(const QString &playlistName) {
  QString cleanName = playlistName.trimmed();
  if (cleanName.isEmpty()) {
    return false;
  }
  auto queueTracks = m_player.getQueueService().exportQueueAsPlaylist();
  if (queueTracks.empty()) {
    queueTracks = m_queueTrackModel->tracks();
  }
  if (queueTracks.empty()) {
    return false;
  }
  QVariantList list;
  for (const auto &t : queueTracks) {
    QVariantMap map;
    map["id"] = QString::fromStdString(t.id);
    map["title"] = QString::fromStdString(t.title);
    map["artist"] = QString::fromStdString(t.artist);
    map["filePath"] = QString::fromStdString(t.filePath);
    map["durationMs"] = static_cast<qint64>(t.durationMs);
    list.append(map);
  }
  return addTracksToPlaylist(cleanName, list);
}

void QtBridge::playTrackAtIndex(int index) {
  auto trackMap = m_trackModel->getTrackAt(index);
  if (!trackMap.isEmpty()) {
    auto all = m_library.getTracks();
    size_t idx = std::clamp(static_cast<size_t>(index), size_t(0),
                            all.empty() ? 0 : all.size() - 1);
    m_player.playQueue(all, idx);
    if (!all.empty()) {
      updatePlaybackState(all[idx]);
    }
    syncQueueModel();
  }
}

void QtBridge::playTrackFromDetail(int index) {
  auto trackMap = m_albumDetailTrackModel->getTrackAt(index);
  if (!trackMap.isEmpty()) {
    auto tracks =
        m_library.getTracksForAlbum(m_selectedAlbumTitle.toStdString(),
                                    m_selectedAlbumArtist.toStdString());
    if (!tracks.empty()) {
      size_t idx =
          std::clamp(static_cast<size_t>(index), size_t(0), tracks.size() - 1);
      m_player.playQueue(tracks, idx);
      updatePlaybackState(tracks[idx]);
      syncQueueModel();
    }
  }
}

void QtBridge::playQueueTrack(int index) {
  auto upcoming = m_player.getQueueService().getUpcomingQueue();
  if (index >= 0 && index < static_cast<int>(upcoming.size())) {
    m_player.play(upcoming[static_cast<size_t>(index)]);
    updatePlaybackState(upcoming[static_cast<size_t>(index)]);
    // Remove current track from upcoming list
    m_player.removeFromQueue(static_cast<size_t>(index));
    syncQueueModel();
  }
}

void QtBridge::queueTrack(const QString &trackId) { queueLast(trackId); }

void QtBridge::updatePlaybackState(const core::Track &track) {
  m_isPlaying = true;

  m_currentTrackTitle = QString::fromStdString(track.title);
  m_currentArtist = QString::fromStdString(track.artist);
  m_currentAlbum = QString::fromStdString(track.album);
  m_currentYear = track.year > 0 ? QString::number(track.year) : "";
  m_currentGenre = QString::fromStdString(track.genre);
  m_currentCodec = QString::fromStdString(track.codec);
  m_currentFilePath = QString::fromStdString(track.filePath);
  m_currentFileSizeStr = formatBytes(track.fileSizeBytes);
  m_currentBitrate = static_cast<int>(track.bitrate);
  m_currentSampleRate = static_cast<int>(track.sampleRate);
  m_currentBitDepth = static_cast<int>(track.bitDepth);
  m_currentChannels = static_cast<int>(track.channels);

  // Audio specs
  QString spec = m_currentCodec;
  if (track.sampleRate > 0) {
    double khz = static_cast<double>(track.sampleRate) / 1000.0;
    spec += QString(" • %1 kHz").arg(khz, 0, 'f', 1);
  }
  if (track.bitDepth > 0) {
    spec += QString(" • %1-bit").arg(track.bitDepth);
  }
  if (track.bitrate > 0) {
    spec += QString(" • %1 kbps").arg(track.bitrate);
  }
  if (track.channels == 1)
    spec += " • Mono";
  else if (track.channels == 2)
    spec += " • Stereo";
  else if (track.channels > 2)
    spec += QString(" • %1 Ch").arg(track.channels);

  m_currentAudioSpecs = spec;
  m_currentArtHash = QString::fromStdString(track.artHash);
  m_durationMs = static_cast<qint64>(track.durationMs);
  m_positionMs = 0;
  m_positionStr = "00:00";
  m_durationStr = QString::fromStdString(track.durationFormatted());
  loadLyricsForTrack(m_currentFilePath);

  m_positionTimer->start();
  emit playbackChanged();
  emit positionChanged();
  syncQueueModel();
  refreshHistory();
}

void QtBridge::togglePlayPause() {
  m_player.togglePlayPause();
  m_isPlaying = m_player.isPlaying();
  if (m_isPlaying) {
    m_positionTimer->start();
  } else {
    m_positionTimer->stop();
  }
  emit playbackChanged();
}

void QtBridge::stop() {
  m_player.stop();
  m_isPlaying = false;
  m_positionMs = 0;
  m_positionStr = "00:00";
  m_activeLyricIndex = -1;
  m_positionTimer->stop();
  emit playbackChanged();
  emit positionChanged();
  emit activeLyricIndexChanged();
}

void QtBridge::nextTrack() {
  if (m_stopAfterCurrentTrack) {
    m_stopAfterCurrentTrack = false;
    stop();
    emit playbackSettingsChanged();
    return;
  }
  m_player.next();
  auto current = m_player.getCurrentTrack();
  if (current.has_value()) {
    updatePlaybackState(*current);
  } else {
    stop();
  }
}

void QtBridge::previousTrack() {
  if (m_positionMs > 3000) {
    seek(0);
    return;
  }
  m_player.previous();
  auto current = m_player.getCurrentTrack();
  if (current.has_value()) {
    updatePlaybackState(*current);
  }
}

void QtBridge::seek(qint64 posMs) {
  m_positionMs = std::clamp(posMs, qint64(0), m_durationMs);
  m_positionStr = formatTime(m_positionMs);
  m_player.seek(static_cast<uint64_t>(m_positionMs));
  emit positionChanged();
  updateActiveLyricIndex();
}

void QtBridge::setVolume(float volume) {
  m_volume = std::clamp(volume, 0.0f, 1.0f);
  m_player.setVolume(m_volume);
  m_isMuted = (m_volume == 0.0f);
  emit volumeChanged();
}

void QtBridge::toggleMute() {
  if (m_isMuted) {
    setVolume(m_prevVolume > 0.0f ? m_prevVolume : 0.8f);
  } else {
    m_prevVolume = m_volume;
    setVolume(0.0f);
  }
}

void QtBridge::setRepeatMode(int mode) {
  m_player.setRepeatMode(static_cast<core::RepeatMode>(mode));
  saveSettings();
  emit playbackModeChanged();
}

void QtBridge::cycleRepeatMode() {
  m_player.cycleRepeatMode();
  saveSettings();
  emit playbackModeChanged();
}

void QtBridge::setShuffleMode(int mode) {
  m_player.setShuffleMode(static_cast<core::ShuffleMode>(mode));
  saveSettings();
  syncQueueModel();
  emit playbackModeChanged();
}

void QtBridge::cycleShuffleMode() {
  int next = (shuffleMode() + 1) % 3;
  setShuffleMode(next);
}

void QtBridge::setPlaybackRate(qreal rate) {
  qreal clamped = std::clamp(rate, 0.25, 4.0);
  if (qFuzzyCompare(m_playbackRate, clamped))
    return;
  m_playbackRate = clamped;
  m_player.setPlaybackRate(static_cast<float>(m_playbackRate));
  emit playbackRateChanged();
}

void QtBridge::setLoopPointA() {
  m_loopPointA = m_positionMs;
  if (m_loopPointB >= 0 && m_loopPointB <= m_loopPointA) {
    m_loopPointB = -1;
  }
  emit loopPointsChanged();
}

void QtBridge::setLoopPointB() {
  if (m_positionMs > m_loopPointA) {
    m_loopPointB = m_positionMs;
  } else if (m_loopPointA >= 0) {
    m_loopPointB = m_loopPointA;
    m_loopPointA = m_positionMs;
  } else {
    m_loopPointA = 0;
    m_loopPointB = m_positionMs;
  }
  emit loopPointsChanged();
}

void QtBridge::clearLoop() {
  m_loopPointA = -1;
  m_loopPointB = -1;
  emit loopPointsChanged();
}

void QtBridge::startSleepTimer(int minutes) {
  if (minutes <= 0) {
    cancelSleepTimer();
    return;
  }
  m_sleepTimerRemainingSec = minutes * 60;
  m_sleepTimerActive = true;
  if (!m_sleepTimer) {
    m_sleepTimer = new QTimer(this);
    m_sleepTimer->setInterval(1000);
    connect(m_sleepTimer, &QTimer::timeout, this, [this]() {
      if (m_sleepTimerRemainingSec > 0) {
        --m_sleepTimerRemainingSec;
        emit sleepTimerChanged();
      }
      if (m_sleepTimerRemainingSec <= 0) {
        cancelSleepTimer();
        stop();
      }
    });
  }
  m_sleepTimer->start();
  emit sleepTimerChanged();
}

void QtBridge::cancelSleepTimer() {
  m_sleepTimerActive = false;
  m_sleepTimerRemainingSec = 0;
  if (m_sleepTimer) {
    m_sleepTimer->stop();
  }
  emit sleepTimerChanged();
}

bool QtBridge::createPlaylist(const QString &name) {
  QString cleanName = name.trimmed();
  if (cleanName.isEmpty())
    return false;
  QString dirPath =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
      "/playlists";
  QDir().mkpath(dirPath);
  QString filePath = dirPath + "/" + cleanName + ".m3u8";
  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    return false;
  QTextStream out(&file);
  out << "#EXTM3U\n";
  out << "#PLAYLIST:" << cleanName << "\n";
  auto selected = m_trackModel->getSelectedTracks();
  for (const auto &item : selected) {
    QVariantMap map = item.toMap();
    out << "#EXTINF:" << (map["durationMs"].toLongLong() / 1000) << ","
        << map["artist"].toString() << " - " << map["title"].toString() << "\n";
    out << map["filePath"].toString() << "\n";
  }
  file.close();
  emit playlistsChanged();
  return true;
}

bool QtBridge::createSmartPlaylist(const QString &name,
                                   const QString &rulesJson) {
  QString cleanName = name.trimmed();
  if (cleanName.isEmpty())
    return false;
  QString dirPath =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
      "/playlists";
  QDir().mkpath(dirPath);
  QString filePath = dirPath + "/" + cleanName + ".smart.json";
  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    return false;
  QJsonObject obj;
  obj["name"] = cleanName;
  obj["type"] = "smart";
  obj["rules"] = rulesJson;
  QJsonDocument doc(obj);
  file.write(doc.toJson());
  file.close();
  emit playlistsChanged();
  return true;
}

bool QtBridge::importPlaylist(const QString &filePath) {
  QString clean = filePath;
  if (clean.startsWith("file://")) {
    clean = QUrl(clean).toLocalFile();
  }
  QFile file(clean);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return false;
  QTextStream in(&file);
  std::vector<core::Track> tracks;
  QFileInfo playlistFi(clean);
  QDir baseDir = playlistFi.dir();

  while (!in.atEnd()) {
    QString line = in.readLine().trimmed();
    if (line.isEmpty() || line.startsWith("#"))
      continue;
    QString audioPath = line;
    if (!QFileInfo(audioPath).isAbsolute()) {
      audioPath = baseDir.absoluteFilePath(audioPath);
    }
    QFileInfo fi(audioPath);
    if (fi.exists() && fi.isFile()) {
      auto trkOpt =
          m_library.getMetadataExtractor().extract(audioPath.toStdString());
      if (trkOpt) {
        m_library.getDatabasePort().saveTrack(*trkOpt);
        tracks.push_back(*trkOpt);
      }
    }
  }
  file.close();
  if (!tracks.empty()) {
    m_player.queueLast(tracks);
    m_trackModel->loadAllTracks();
    m_albumModel->reload();
    syncQueueModel();
    refreshLibraryStats();
    emit playlistsChanged();
    return true;
  }
  return false;
}

bool QtBridge::addTracksToPlaylist(const QString &playlistName,
                                   const QVariantList &tracks) {
  QString cleanName = playlistName.trimmed();
  if (cleanName.isEmpty() || tracks.isEmpty())
    return false;
  QString dirPath =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
      "/playlists";
  QDir().mkpath(dirPath);
  QString filePath = dirPath + "/" + cleanName + ".m3u8";
  bool isNew = !QFile::exists(filePath);
  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    return false;
  QTextStream out(&file);
  if (isNew) {
    out << "#EXTM3U\n";
    out << "#PLAYLIST:" << cleanName << "\n";
  }
  for (const auto &item : tracks) {
    QVariantMap map = item.toMap();
    out << "#EXTINF:" << (map["durationMs"].toLongLong() / 1000) << ","
        << map["artist"].toString() << " - " << map["title"].toString() << "\n";
    out << map["filePath"].toString() << "\n";
  }
  file.close();
  emit playlistsChanged();
  return true;
}

bool QtBridge::removeTracksFromPlaylist(const QString &playlistName,
                                        const QVariantList &tracks) {
  QString cleanName = playlistName.trimmed();
  if (cleanName.isEmpty() || tracks.isEmpty())
    return false;
  QString dirPath =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
      "/playlists";
  QString filePath = dirPath + "/" + cleanName + ".m3u8";
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return false;

  std::unordered_set<std::string> pathsToRemove;
  for (const auto &item : tracks) {
    QVariantMap map = item.toMap();
    pathsToRemove.insert(map["filePath"].toString().toStdString());
  }

  QTextStream in(&file);
  QStringList remainingLines;
  QString pendingExtInf;
  while (!in.atEnd()) {
    QString line = in.readLine();
    QString trimmed = line.trimmed();
    if (trimmed.startsWith("#EXTINF:")) {
      pendingExtInf = line;
      continue;
    }
    if (!trimmed.isEmpty() && !trimmed.startsWith("#")) {
      if (pathsToRemove.count(trimmed.toStdString())) {
        pendingExtInf.clear();
        continue;
      }
    }
    if (!pendingExtInf.isEmpty()) {
      remainingLines.append(pendingExtInf);
      pendingExtInf.clear();
    }
    remainingLines.append(line);
  }
  file.close();

  if (file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
    QTextStream out(&file);
    for (const auto &line : remainingLines) {
      out << line << "\n";
    }
    file.close();
    emit playlistsChanged();
    return true;
  }
  return false;
}

void QtBridge::openPlaylist(const QString &playlistName) {
  QString cleanName = playlistName.trimmed();
  if (cleanName.isEmpty())
    return;
  QString dirPath =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
      "/playlists";

  QString smartPath = dirPath + "/" + cleanName + ".smart.json";
  if (QFile::exists(smartPath)) {
    QFile file(smartPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
      file.close();
      auto allTracks = m_library.getTracks();
      std::vector<core::Track> matched;
      if (doc.isObject()) {
        QString rules = doc.object()["rules"].toString().trimmed();
        if (!rules.isEmpty()) {
          for (const auto &t : allTracks) {
            if (QString::fromStdString(t.genre).contains(rules,
                                                         Qt::CaseInsensitive) ||
                QString::fromStdString(t.artist).contains(
                    rules, Qt::CaseInsensitive) ||
                QString::fromStdString(t.album).contains(rules,
                                                         Qt::CaseInsensitive)) {
              matched.push_back(t);
            }
          }
        } else {
          matched = allTracks;
        }
      } else {
        matched = allTracks;
      }
      m_trackModel->setTracks(matched);
      return;
    }
  }

  QString filePath = dirPath + "/" + cleanName + ".m3u8";
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return;

  QFileInfo playlistFi(filePath);
  QDir baseDir = playlistFi.dir();
  QTextStream in(&file);
  std::vector<core::Track> playlistTracks;

  while (!in.atEnd()) {
    QString line = in.readLine().trimmed();
    if (line.isEmpty() || line.startsWith("#"))
      continue;
    QString audioPath = line;
    if (!QFileInfo(audioPath).isAbsolute()) {
      audioPath = baseDir.absoluteFilePath(audioPath);
    }
    auto trackOpt =
        m_library.getDatabasePort().getTrackByPath(audioPath.toStdString());
    if (trackOpt) {
      playlistTracks.push_back(*trackOpt);
    } else {
      auto extracted =
          m_library.getMetadataExtractor().extract(audioPath.toStdString());
      if (extracted) {
        playlistTracks.push_back(*extracted);
      }
    }
  }
  file.close();

  m_trackModel->setTracks(playlistTracks);
}

void QtBridge::playPlaylist(const QString &playlistName) {
  openPlaylist(playlistName);
  const auto &tracks = m_trackModel->tracks();
  if (!tracks.empty()) {
    m_player.playQueue(tracks, 0);
    updatePlaybackState(tracks[0]);
    syncQueueModel();
  }
}

void QtBridge::playPlaylistNext(const QString &playlistName) {
  openPlaylist(playlistName);
  const auto &tracks = m_trackModel->tracks();
  if (!tracks.empty()) {
    m_player.playNext(tracks);
    syncQueueModel();
  }
}

void QtBridge::queuePlaylistLast(const QString &playlistName) {
  openPlaylist(playlistName);
  const auto &tracks = m_trackModel->tracks();
  if (!tracks.empty()) {
    m_player.queueLast(tracks);
    syncQueueModel();
  }
}

bool QtBridge::renamePlaylist(const QString &oldName, const QString &newName) {
  QString cleanOld = oldName.trimmed();
  QString cleanNew = newName.trimmed();
  if (cleanOld.isEmpty() || cleanNew.isEmpty() || cleanOld == cleanNew)
    return false;
  QString dirPath =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
      "/playlists";
  bool ok = false;
  QString oldM3u = dirPath + "/" + cleanOld + ".m3u8";
  QString newM3u = dirPath + "/" + cleanNew + ".m3u8";
  if (QFile::exists(oldM3u)) {
    QFile::remove(newM3u);
    ok = QFile::rename(oldM3u, newM3u);
  }
  QString oldSmart = dirPath + "/" + cleanOld + ".smart.json";
  QString newSmart = dirPath + "/" + cleanNew + ".smart.json";
  if (QFile::exists(oldSmart)) {
    QFile::remove(newSmart);
    ok = QFile::rename(oldSmart, newSmart) || ok;
  }
  if (ok) {
    emit playlistsChanged();
  }
  return ok;
}

bool QtBridge::duplicatePlaylist(const QString &playlistName) {
  QString cleanName = playlistName.trimmed();
  if (cleanName.isEmpty())
    return false;
  QString dirPath =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
      "/playlists";
  QString m3uPath = dirPath + "/" + cleanName + ".m3u8";
  QString smartPath = dirPath + "/" + cleanName + ".smart.json";
  QString newBase = cleanName + " (Copy)";
  int counter = 2;
  while (QFile::exists(dirPath + "/" + newBase + ".m3u8") ||
         QFile::exists(dirPath + "/" + newBase + ".smart.json")) {
    newBase = QString("%1 (Copy %2)").arg(cleanName).arg(counter++);
  }
  bool copied = false;
  if (QFile::exists(m3uPath)) {
    copied = QFile::copy(m3uPath, dirPath + "/" + newBase + ".m3u8");
  }
  if (QFile::exists(smartPath)) {
    copied = QFile::copy(smartPath, dirPath + "/" + newBase + ".smart.json") ||
             copied;
  }
  if (copied) {
    emit playlistsChanged();
  }
  return copied;
}

bool QtBridge::exportPlaylistFile(const QString &playlistName,
                                  const QString &targetFilePath) {
  QString cleanName = playlistName.trimmed();
  if (cleanName.isEmpty())
    return false;
  QString dirPath =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
      "/playlists";
  QString src = dirPath + "/" + cleanName + ".m3u8";
  if (!QFile::exists(src)) {
    src = dirPath + "/" + cleanName + ".smart.json";
  }
  if (!QFile::exists(src))
    return false;
  QString target = targetFilePath;
  if (target.startsWith("file://")) {
    target = QUrl(target).toLocalFile();
  }
  QFile::remove(target);
  return QFile::copy(src, target);
}

bool QtBridge::deletePlaylist(const QString &playlistName) {
  QString cleanName = playlistName.trimmed();
  if (cleanName.isEmpty())
    return false;
  QString dirPath =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
      "/playlists";
  QString m3uPath = dirPath + "/" + cleanName + ".m3u8";
  QString smartPath = dirPath + "/" + cleanName + ".smart.json";
  bool removed = false;
  if (QFile::exists(m3uPath)) {
    removed = QFile::remove(m3uPath);
  }
  if (QFile::exists(smartPath)) {
    removed = QFile::remove(smartPath) || removed;
  }
  if (removed) {
    emit playlistsChanged();
  }
  return removed;
}

bool QtBridge::isSmartPlaylist(const QString &playlistName) const {
  QString dirPath =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
      "/playlists";
  return QFile::exists(dirPath + "/" + playlistName.trimmed() + ".smart.json");
}

QString QtBridge::getSmartPlaylistRules(const QString &playlistName) const {
  QString dirPath =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
      "/playlists";
  QFile file(dirPath + "/" + playlistName.trimmed() + ".smart.json");
  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (doc.isObject()) {
      return doc.object()["rules"].toString();
    }
  }
  return "";
}

bool QtBridge::updateSmartPlaylistRules(const QString &playlistName,
                                        const QString &rulesJson) {
  QString cleanName = playlistName.trimmed();
  if (cleanName.isEmpty())
    return false;
  return createSmartPlaylist(cleanName, rulesJson);
}

int QtBridge::deduplicateTracks() {
  auto all = m_library.getTracks();
  std::unordered_set<std::string> seen;
  std::vector<std::string> duplicateIds;
  std::vector<core::Track> removedTracks;

  for (const auto &track : all) {
    std::string key = track.title + "\n" + track.artist;
    if (seen.find(key) != seen.end()) {
      duplicateIds.push_back(track.id);
      removedTracks.push_back(track);
    } else {
      seen.insert(key);
    }
  }

  if (duplicateIds.empty())
    return 0;

  for (const auto &id : duplicateIds) {
    m_library.getDatabasePort().deleteTrack(id);
  }

  pushUndoCommand(
      QString("Deduplicate %1 Tracks").arg(duplicateIds.size()),
      [this, removedTracks]() {
        for (const auto &t : removedTracks) {
          m_library.getDatabasePort().saveTrack(t);
        }
        m_trackModel->loadAllTracks();
        m_albumModel->reload();
        refreshLibraryStats();
      },
      [this, duplicateIds]() {
        for (const auto &id : duplicateIds) {
          m_library.getDatabasePort().deleteTrack(id);
        }
        m_trackModel->loadAllTracks();
        m_albumModel->reload();
        refreshLibraryStats();
      });

  m_trackModel->loadAllTracks();
  m_albumModel->reload();
  refreshLibraryStats();
  return static_cast<int>(duplicateIds.size());
}

QString QtBridge::getLogFilePath() const {
  QString logDir =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QDir().mkpath(logDir);
  QString logPath = logDir + "/picaudio.log";
  if (!QFile::exists(logPath)) {
    QFile f(logPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
      QTextStream out(&f);
      out << "[PicAudio Session Log]\n";
      out << "Version: 0.1.0\n";
      out << "Backend: " << m_audioBackend << "\n";
      f.close();
    }
  }
  return logPath;
}

QVariantMap QtBridge::audioPipelineDiagnostics() const {
  QVariantMap map;
  map["sampleRate"] = m_currentSampleRate > 0 ? m_currentSampleRate : 44100;
  map["bitDepth"] = m_currentBitDepth > 0 ? m_currentBitDepth : 16;
  map["channels"] = m_currentChannels > 0 ? m_currentChannels : 2;
  map["codec"] = m_currentCodec.isEmpty() ? "FLAC / PCM" : m_currentCodec;
  map["bitrate"] = m_currentBitrate;
  map["bufferLatencyMs"] = m_bufferLatencyMs;
  map["audioBackend"] = m_audioBackend;
  map["currentAudioDevice"] = m_currentAudioDevice;
  map["isBitPerfect"] = m_bitPerfectExclusive;
  map["resamplerQuality"] = m_resamplerQuality;
  map["ditherMode"] = m_ditherMode;
  map["channelProcessing"] = m_channelProcessing;
  map["xruns"] = 0;
  map["state"] = m_isPlaying ? "Playing" : "Stopped";
  map["loopActive"] = isLoopActive();
  map["loopPointA"] = m_loopPointA;
  map["loopPointB"] = m_loopPointB;
  map["playbackRate"] = m_playbackRate;
  return map;
}

void QtBridge::playAll() {
  auto all = m_library.getTracks();
  if (!all.empty()) {
    m_player.playQueue(all, 0);
    updatePlaybackState(all[0]);
    syncQueueModel();
  }
}

void QtBridge::shuffleAll() {
  auto all = m_library.getTracks();
  if (!all.empty()) {
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(all.begin(), all.end(), g);
    m_player.playQueue(all, 0);
    updatePlaybackState(all[0]);
    syncQueueModel();
  }
}

void QtBridge::showInFileManager(const QString &filePath) {
  QFileInfo fi(filePath);
  if (fi.exists()) {
    QDesktopServices::openUrl(QUrl::fromLocalFile(fi.absolutePath()));
  }
}

void QtBridge::copyToClipboard(const QString &text) {
  QClipboard *cb = QGuiApplication::clipboard();
  if (cb) {
    cb->setText(text);
  }
}

QString QtBridge::formatDuration(qint64 ms) const { return formatTime(ms); }

void QtBridge::setShowRemainingTime(bool enable) {
  if (m_showRemainingTime != enable) {
    m_showRemainingTime = enable;
    saveSettings();
    emit showRemainingTimeChanged();
  }
}

void QtBridge::setVolumeLocked(bool locked) {
  if (m_volumeLocked != locked) {
    m_volumeLocked = locked;
    saveSettings();
    emit volumeLockedChanged();
  }
}

void QtBridge::openAudioFile(const QString &filePath) {
  QString cleanPath = filePath;
  QUrl url = QUrl::fromUserInput(filePath);
  if (url.isLocalFile()) {
    cleanPath = url.toLocalFile();
  } else if (cleanPath.startsWith("file://")) {
    cleanPath = cleanPath.mid(7);
  }

  QFileInfo fi(cleanPath);
  if (!fi.exists() || !fi.isFile())
    return;

  auto trackOpt =
      m_library.getMetadataExtractor().extract(cleanPath.toStdString());
  if (trackOpt) {
    auto track = *trackOpt;
    m_library.getDatabasePort().saveTrack(track);
    m_player.play(track);
    updatePlaybackState(track);
    m_trackModel->loadAllTracks();
    m_albumModel->reload();
    syncQueueModel();
  }
}

void QtBridge::openFolder(const QString &folderPath, bool enqueue) {
  QString cleanPath = folderPath;
  QUrl url = QUrl::fromUserInput(folderPath);
  if (url.isLocalFile()) {
    cleanPath = url.toLocalFile();
  } else if (cleanPath.startsWith("file://")) {
    cleanPath = cleanPath.mid(7);
  }

  QDir dir(cleanPath);
  if (!dir.exists())
    return;

  m_library.scanDirectory(cleanPath.toStdString());
  m_trackModel->loadAllTracks();
  m_albumModel->reload();
  refreshLibraryStats();

  auto all = m_library.getTracks();
  std::vector<core::Track> folderTracks;
  std::string prefix = cleanPath.toStdString();
  for (const auto &t : all) {
    if (t.filePath.rfind(prefix, 0) == 0) {
      folderTracks.push_back(t);
    }
  }

  if (folderTracks.empty())
    folderTracks = all;
  if (!folderTracks.empty()) {
    if (enqueue) {
      m_player.queueLast(folderTracks);
      syncQueueModel();
    } else {
      m_player.playQueue(folderTracks, 0);
      updatePlaybackState(folderTracks[0]);
      syncQueueModel();
    }
  }
}

QStringList QtBridge::extractAudioFilesFromUrls(const QList<QUrl> &urls) {
  QStringList audioFiles;
  static const QStringList extensions = {
      QStringLiteral(".flac"), QStringLiteral(".wav"), QStringLiteral(".aiff"),
      QStringLiteral(".aif"),  QStringLiteral(".mp3"), QStringLiteral(".m4a"),
      QStringLiteral(".alac"), QStringLiteral(".mp4"), QStringLiteral(".m4b"),
      QStringLiteral(".aac"),  QStringLiteral(".ogg"), QStringLiteral(".oga"),
      QStringLiteral(".opus"), QStringLiteral(".spx"), QStringLiteral(".dsf"),
      QStringLiteral(".dff"),  QStringLiteral(".ape"), QStringLiteral(".wv"),
      QStringLiteral(".wma"),  QStringLiteral(".cue"), QStringLiteral(".m3u"),
      QStringLiteral(".m3u8"), QStringLiteral(".pls")};

  for (const QUrl &url : urls) {
    QString path = url.isLocalFile() ? url.toLocalFile() : url.toString();
    if (path.startsWith(QStringLiteral("file://"))) {
      path = path.mid(7);
    }
    QFileInfo fi(path);
    if (!fi.exists()) {
      continue;
    }

    if (fi.isDir()) {
      QDirIterator it(path, QDir::Files | QDir::NoSymLinks,
                      QDirIterator::Subdirectories);
      while (it.hasNext()) {
        it.next();
        QString ext = QStringLiteral(".") + it.fileInfo().suffix().toLower();
        if (extensions.contains(ext)) {
          audioFiles.append(it.filePath());
        }
      }
    } else if (fi.isFile()) {
      QString ext = QStringLiteral(".") + fi.suffix().toLower();
      if (extensions.contains(ext)) {
        audioFiles.append(fi.absoluteFilePath());
      }
    }
  }
  return audioFiles;
}

void QtBridge::handleDropOnTracks(const QList<QUrl> &urls,
                                  bool playImmediately) {
  QStringList files = extractAudioFilesFromUrls(urls);
  if (files.isEmpty()) {
    return;
  }

  std::vector<core::Track> tracks;
  tracks.reserve(files.size());

  for (const QString &file : files) {
    if (file.endsWith(QStringLiteral(".cue"), Qt::CaseInsensitive)) {
      openCueSheet(file);
      continue;
    }
    auto trackOpt =
        m_library.getMetadataExtractor().extract(file.toStdString());
    if (trackOpt) {
      m_library.getDatabasePort().saveTrack(*trackOpt);
      tracks.push_back(*trackOpt);
    }
  }

  if (tracks.empty()) {
    return;
  }

  m_trackModel->loadAllTracks();
  m_albumModel->reload();
  refreshLibraryStats();

  if (playImmediately) {
    m_player.playQueue(tracks, 0);
    updatePlaybackState(tracks[0]);
  } else {
    m_player.queueLast(tracks);
  }
  syncQueueModel();
}

void QtBridge::handleDropOnQueue(const QList<QUrl> &urls) {
  QStringList files = extractAudioFilesFromUrls(urls);
  if (files.isEmpty()) {
    return;
  }

  std::vector<core::Track> tracks;
  tracks.reserve(files.size());

  for (const QString &file : files) {
    auto trackOpt =
        m_library.getMetadataExtractor().extract(file.toStdString());
    if (trackOpt) {
      m_library.getDatabasePort().saveTrack(*trackOpt);
      tracks.push_back(*trackOpt);
    }
  }

  if (!tracks.empty()) {
    m_player.queueLast(tracks);
    syncQueueModel();
  }
}

void QtBridge::handleDropOnPlaylist(const QList<QUrl> &urls,
                                    const QString &targetPlaylist) {
  QStringList files = extractAudioFilesFromUrls(urls);
  if (files.isEmpty()) {
    return;
  }

  QString plName = targetPlaylist.trimmed();
  if (plName.isEmpty()) {
    if (!urls.isEmpty()) {
      QString firstPath = urls.first().isLocalFile()
                              ? urls.first().toLocalFile()
                              : urls.first().toString();
      if (firstPath.startsWith(QStringLiteral("file://"))) {
        firstPath = firstPath.mid(7);
      }
      QFileInfo fi(firstPath);
      if (fi.isDir()) {
        plName = fi.fileName();
      } else {
        plName = fi.dir().dirName();
      }
    }
    if (plName.isEmpty()) {
      plName = QStringLiteral("Dropped Tracks");
    }
    createPlaylist(plName);
  }

  QVariantList trackVariants;
  for (const QString &file : files) {
    auto trackOpt =
        m_library.getMetadataExtractor().extract(file.toStdString());
    if (trackOpt) {
      m_library.getDatabasePort().saveTrack(*trackOpt);
      QVariantMap map;
      map.insert(QStringLiteral("filePath"),
                 QString::fromStdString(trackOpt->filePath));
      map.insert(QStringLiteral("title"),
                 QString::fromStdString(trackOpt->title));
      map.insert(QStringLiteral("artist"),
                 QString::fromStdString(trackOpt->artist));
      map.insert(QStringLiteral("durationMs"),
                 static_cast<qint64>(trackOpt->durationMs));
      trackVariants.append(map);
    }
  }

  if (!trackVariants.isEmpty()) {
    addTracksToPlaylist(plName, trackVariants);
    if (m_activePlaylistName == plName) {
      openPlaylist(plName);
    }
  }
}

void QtBridge::handleDropOnExplorer(const QList<QUrl> &urls) {
  for (const QUrl &url : urls) {
    QString path = url.isLocalFile() ? url.toLocalFile() : url.toString();
    if (path.startsWith(QStringLiteral("file://"))) {
      path = path.mid(7);
    }
    QFileInfo fi(path);
    if (fi.isDir()) {
      addMonitoredFolder(path);
      scanDirectory(path);
    } else if (fi.isFile()) {
      openAudioFile(path);
    }
  }
}

void QtBridge::openCueSheet(const QString &cueFilePath) {
  QString cleanPath = cueFilePath;
  QUrl url = QUrl::fromUserInput(cueFilePath);
  if (url.isLocalFile()) {
    cleanPath = url.toLocalFile();
  } else if (cleanPath.startsWith("file://")) {
    cleanPath = cleanPath.mid(7);
  }

  QFile cueFile(cleanPath);
  if (!cueFile.open(QIODevice::ReadOnly | QIODevice::Text))
    return;

  QFileInfo cueInfo(cleanPath);
  QDir cueDir = cueInfo.dir();

  QTextStream in(&cueFile);
  QString currentAudioFile;
  QString albumTitle = cueInfo.baseName();
  QString albumArtist = "Various Artists";
  std::vector<core::Track> cueTracks;

  while (!in.atEnd()) {
    QString line = in.readLine().trimmed();
    if (line.startsWith("PERFORMER ", Qt::CaseInsensitive) &&
        cueTracks.empty()) {
      albumArtist = line.mid(10).trimmed().remove('"');
    } else if (line.startsWith("TITLE ", Qt::CaseInsensitive) &&
               cueTracks.empty()) {
      albumTitle = line.mid(6).trimmed().remove('"');
    } else if (line.startsWith("FILE ", Qt::CaseInsensitive)) {
      int firstQuote = line.indexOf('"');
      int lastQuote = line.lastIndexOf('"');
      if (firstQuote != -1 && lastQuote > firstQuote) {
        currentAudioFile = line.mid(firstQuote + 1, lastQuote - firstQuote - 1);
      }
    } else if (line.startsWith("TRACK ", Qt::CaseInsensitive)) {
      core::Track t;
      t.id = "cue_" + std::to_string(cueTracks.size() + 1) + "_" +
             std::to_string(
                 std::chrono::system_clock::now().time_since_epoch().count());
      t.album = albumTitle.toStdString();
      t.albumArtist = albumArtist.toStdString();
      t.artist = albumArtist.toStdString();
      t.trackNumber = static_cast<uint32_t>(cueTracks.size() + 1);
      QString targetAudio = cueDir.absoluteFilePath(currentAudioFile);
      t.filePath = targetAudio.toStdString();
      t.codec = "FLAC";
      t.sampleRate = 44100;
      t.bitDepth = 16;
      t.channels = 2;
      t.bitrate = 1411;
      cueTracks.push_back(t);
    } else if (line.startsWith("TITLE ", Qt::CaseInsensitive) &&
               !cueTracks.empty()) {
      cueTracks.back().title = line.mid(6).trimmed().remove('"').toStdString();
    } else if (line.startsWith("PERFORMER ", Qt::CaseInsensitive) &&
               !cueTracks.empty()) {
      cueTracks.back().artist =
          line.mid(10).trimmed().remove('"').toStdString();
    }
  }

  cueFile.close();

  if (!cueTracks.empty()) {
    m_player.playQueue(cueTracks, 0);
    updatePlaybackState(cueTracks[0]);
    syncQueueModel();
  }
}

void QtBridge::openNetworkStream(const QString &streamUrl,
                                 const QString &streamName) {
  if (streamUrl.trimmed().isEmpty())
    return;

  core::Track streamTrack;
  streamTrack.id =
      "stream_" +
      std::to_string(
          std::chrono::system_clock::now().time_since_epoch().count());
  streamTrack.filePath = streamUrl.trimmed().toStdString();
  streamTrack.title = streamName.trimmed().isEmpty()
                          ? streamUrl.trimmed().toStdString()
                          : streamName.trimmed().toStdString();
  streamTrack.artist = "Internet Radio";
  streamTrack.album = "Live Stream";
  streamTrack.genre = "Radio";
  streamTrack.codec =
      streamUrl.contains(".flac", Qt::CaseInsensitive) ? "FLAC" : "MP3";
  streamTrack.sampleRate = 44100;
  streamTrack.bitDepth = 16;
  streamTrack.channels = 2;
  streamTrack.bitrate = 320;

  m_player.play(streamTrack);
  updatePlaybackState(streamTrack);
  syncQueueModel();
}

bool QtBridge::exportActiveView(const QString &filePath,
                                const QString &format) {
  QString cleanPath = filePath;
  QUrl url = QUrl::fromUserInput(filePath);
  if (url.isLocalFile()) {
    cleanPath = url.toLocalFile();
  } else if (cleanPath.startsWith("file://")) {
    cleanPath = cleanPath.mid(7);
  }

  QFile file(cleanPath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    return false;
  }

  QTextStream out(&file);
  const auto &tracks = m_trackModel->tracks();
  QString fmt = format.toLower();
  if (fmt.isEmpty()) {
    if (cleanPath.endsWith(".m3u8", Qt::CaseInsensitive) ||
        cleanPath.endsWith(".m3u", Qt::CaseInsensitive))
      fmt = "m3u8";
    else if (cleanPath.endsWith(".csv", Qt::CaseInsensitive))
      fmt = "csv";
    else if (cleanPath.endsWith(".json", Qt::CaseInsensitive))
      fmt = "json";
    else
      fmt = "m3u8";
  }

  if (fmt == "m3u8" || fmt == "m3u") {
    out << "#EXTM3U\n";
    for (const auto &t : tracks) {
      uint64_t sec = t.durationMs / 1000;
      out << "#EXTINF:" << sec << "," << QString::fromStdString(t.artist)
          << " - " << QString::fromStdString(t.title) << "\n";
      out << QString::fromStdString(t.filePath) << "\n";
    }
  } else if (fmt == "csv") {
    out << "Track,Title,Artist,Album,Genre,Year,Duration,Bitrate,Codec,"
           "FilePath\n";
    for (const auto &t : tracks) {
      auto escapeCsv = [](const std::string &s) {
        QString str = QString::fromStdString(s);
        str.replace("\"", "\"\"");
        return "\"" + str + "\"";
      };
      out << t.trackNumber << "," << escapeCsv(t.title) << ","
          << escapeCsv(t.artist) << "," << escapeCsv(t.album) << ","
          << escapeCsv(t.genre) << "," << t.year << ","
          << QString::fromStdString(t.durationFormatted()) << "," << t.bitrate
          << "," << QString::fromStdString(t.codec) << ","
          << escapeCsv(t.filePath) << "\n";
    }
  } else if (fmt == "json") {
    QJsonArray arr;
    for (const auto &t : tracks) {
      QJsonObject obj;
      obj["id"] = QString::fromStdString(t.id);
      obj["title"] = QString::fromStdString(t.title);
      obj["artist"] = QString::fromStdString(t.artist);
      obj["album"] = QString::fromStdString(t.album);
      obj["albumArtist"] = QString::fromStdString(t.albumArtist);
      obj["genre"] = QString::fromStdString(t.genre);
      obj["year"] = static_cast<int>(t.year);
      obj["trackNumber"] = static_cast<int>(t.trackNumber);
      obj["discNumber"] = static_cast<int>(t.discNumber);
      obj["durationMs"] = static_cast<qint64>(t.durationMs);
      obj["durationFormatted"] = QString::fromStdString(t.durationFormatted());
      obj["sampleRate"] = static_cast<int>(t.sampleRate);
      obj["bitDepth"] = static_cast<int>(t.bitDepth);
      obj["channels"] = static_cast<int>(t.channels);
      obj["bitrate"] = static_cast<int>(t.bitrate);
      obj["codec"] = QString::fromStdString(t.codec);
      obj["filePath"] = QString::fromStdString(t.filePath);
      arr.append(obj);
    }
    QJsonDocument doc(arr);
    out << doc.toJson(QJsonDocument::Indented);
  }
  file.close();
  return true;
}

QString QtBridge::undoActionName() const {
  return m_undoStack.empty() ? QString() : m_undoStack.back().name;
}

QString QtBridge::redoActionName() const {
  return m_redoStack.empty() ? QString() : m_redoStack.back().name;
}

void QtBridge::pushUndoCommand(const QString &name,
                               std::function<void()> undoFn,
                               std::function<void()> redoFn) {
  m_undoStack.push_back({name, std::move(undoFn), std::move(redoFn)});
  m_redoStack.clear();
  emit undoRedoChanged();
}

void QtBridge::undo() {
  if (m_undoStack.empty())
    return;
  auto cmd = m_undoStack.back();
  m_undoStack.pop_back();
  if (cmd.undoFn)
    cmd.undoFn();
  m_redoStack.push_back(cmd);
  emit undoRedoChanged();
}

void QtBridge::redo() {
  if (m_redoStack.empty())
    return;
  auto cmd = m_redoStack.back();
  m_redoStack.pop_back();
  if (cmd.redoFn)
    cmd.redoFn();
  m_undoStack.push_back(cmd);
  emit undoRedoChanged();
}

void QtBridge::refreshHistory() {
  if (m_historyTrackModel) {
    m_historyTrackModel->setTracks(m_player.getQueueService().getHistory());
  }
}

bool QtBridge::updateTrackTags(const QString &trackId,
                               const QVariantMap &tags) {
  auto trackOpt = m_library.getTrackById(trackId.toStdString());
  if (!trackOpt)
    return false;

  core::Track oldTrack = *trackOpt;
  core::Track newTrack = oldTrack;

  if (tags.contains("title"))
    newTrack.title = tags["title"].toString().toStdString();
  if (tags.contains("artist"))
    newTrack.artist = tags["artist"].toString().toStdString();
  if (tags.contains("album"))
    newTrack.album = tags["album"].toString().toStdString();
  if (tags.contains("genre"))
    newTrack.genre = tags["genre"].toString().toStdString();
  if (tags.contains("year") && tags["year"].toInt() > 0)
    newTrack.year = static_cast<uint32_t>(tags["year"].toInt());
  if (tags.contains("trackNumber") && tags["trackNumber"].toInt() > 0)
    newTrack.trackNumber = static_cast<uint32_t>(tags["trackNumber"].toInt());

  m_library.getDatabasePort().saveTrack(newTrack);
  m_trackModel->loadAllTracks();
  m_albumModel->reload();

  pushUndoCommand(
      "Edit Tags: " + QString::fromStdString(newTrack.title),
      [this, oldTrack]() {
        m_library.getDatabasePort().saveTrack(oldTrack);
        m_trackModel->loadAllTracks();
        m_albumModel->reload();
      },
      [this, newTrack]() {
        m_library.getDatabasePort().saveTrack(newTrack);
        m_trackModel->loadAllTracks();
        m_albumModel->reload();
      });

  return true;
}

bool QtBridge::updateMultipleTrackTags(const QStringList &trackIds,
                                       const QVariantMap &tags) {
  if (trackIds.isEmpty())
    return false;
  std::vector<core::Track> oldTracks;
  std::vector<core::Track> newTracks;

  for (const auto &id : trackIds) {
    auto opt = m_library.getTrackById(id.toStdString());
    if (opt) {
      oldTracks.push_back(*opt);
      core::Track nt = *opt;
      if (tags.contains("artist") && !tags["artist"].toString().isEmpty())
        nt.artist = tags["artist"].toString().toStdString();
      if (tags.contains("album") && !tags["album"].toString().isEmpty())
        nt.album = tags["album"].toString().toStdString();
      if (tags.contains("genre") && !tags["genre"].toString().isEmpty())
        nt.genre = tags["genre"].toString().toStdString();
      if (tags.contains("year") && tags["year"].toInt() > 0)
        nt.year = static_cast<uint32_t>(tags["year"].toInt());
      newTracks.push_back(nt);
      m_library.getDatabasePort().saveTrack(nt);
    }
  }

  m_trackModel->loadAllTracks();
  m_albumModel->reload();

  pushUndoCommand(
      QString("Edit Tags (%1 tracks)").arg(newTracks.size()),
      [this, oldTracks]() {
        for (const auto &t : oldTracks) {
          m_library.getDatabasePort().saveTrack(t);
        }
        m_trackModel->loadAllTracks();
        m_albumModel->reload();
      },
      [this, newTracks]() {
        for (const auto &t : newTracks) {
          m_library.getDatabasePort().saveTrack(t);
        }
        m_trackModel->loadAllTracks();
        m_albumModel->reload();
      });

  return true;
}

void QtBridge::deleteSelectedTracks(const QStringList &trackIds) {
  if (trackIds.isEmpty())
    return;
  std::vector<core::Track> deletedTracks;
  for (const auto &id : trackIds) {
    auto opt = m_library.getTrackById(id.toStdString());
    if (opt) {
      deletedTracks.push_back(*opt);
      m_library.getDatabasePort().deleteTrack(id.toStdString());
    }
  }

  m_trackModel->loadAllTracks();
  m_albumModel->reload();
  refreshLibraryStats();

  pushUndoCommand(
      QString("Delete %1 tracks").arg(deletedTracks.size()),
      [this, deletedTracks]() {
        for (const auto &t : deletedTracks) {
          m_library.getDatabasePort().saveTrack(t);
        }
        m_trackModel->loadAllTracks();
        m_albumModel->reload();
        refreshLibraryStats();
      },
      [this, trackIds]() {
        for (const auto &id : trackIds) {
          m_library.getDatabasePort().deleteTrack(id.toStdString());
        }
        m_trackModel->loadAllTracks();
        m_albumModel->reload();
        refreshLibraryStats();
      });
}

bool QtBridge::deleteFilesFromDisk(const QStringList &trackIds) {
  if (trackIds.isEmpty())
    return false;
  bool success = true;
  for (const auto &id : trackIds) {
    auto opt = m_library.getTrackById(id.toStdString());
    if (opt) {
      QString path = QString::fromStdString(opt->filePath);
      if (QFile::exists(path)) {
        if (!QFile::remove(path)) {
          success = false;
        }
      }
      m_library.getDatabasePort().deleteTrack(id.toStdString());
    }
  }

  m_trackModel->loadAllTracks();
  m_albumModel->reload();
  refreshLibraryStats();
  return success;
}

void QtBridge::setTracksRating(const QStringList &trackIds, int rating) {
  QSettings s("PicAudio", "PicAudioPlayer");
  s.beginGroup("ratings");
  for (const auto &id : trackIds) {
    if (rating <= 0) {
      s.remove(id);
    } else {
      s.setValue(id, std::clamp(rating, 0, 5));
    }
  }
  s.endGroup();
  emit ratingsChanged();
}

int QtBridge::getTrackRating(const QString &trackId) const {
  QSettings s("PicAudio", "PicAudioPlayer");
  return s.value("ratings/" + trackId, 0).toInt();
}

void QtBridge::toggleTracksFavorite(const QStringList &trackIds) {
  QSettings s("PicAudio", "PicAudioPlayer");
  s.beginGroup("favorites");
  for (const auto &id : trackIds) {
    bool fav = s.value(id, false).toBool();
    s.setValue(id, !fav);
  }
  s.endGroup();
  emit ratingsChanged();
}

bool QtBridge::isTrackFavorite(const QString &trackId) const {
  QSettings s("PicAudio", "PicAudioPlayer");
  return s.value("favorites/" + trackId, false).toBool();
}

void QtBridge::addMonitoredFolder(const QString &folderPath) {
  QString local = folderPath;
  QUrl u = QUrl::fromUserInput(folderPath);
  if (u.isLocalFile())
    local = u.toLocalFile();
  else if (local.startsWith("file://"))
    local = local.mid(7);

  if (!local.isEmpty() && !m_monitoredFolders.contains(local)) {
    m_monitoredFolders.append(local);
    saveSettings();
    scanDirectory(local);
  }
}

void QtBridge::removeMonitoredFolder(int index) {
  if (index >= 0 && index < m_monitoredFolders.size()) {
    m_monitoredFolders.removeAt(index);
    saveSettings();
  }
}

void QtBridge::removeMonitoredFolderPath(const QString &folderPath) {
  QString local = folderPath;
  QUrl u = QUrl::fromUserInput(folderPath);
  if (u.isLocalFile())
    local = u.toLocalFile();
  else if (local.startsWith("file://"))
    local = local.mid(7);

  for (int i = 0; i < m_monitoredFolders.size(); ++i) {
    if (m_monitoredFolders[i] == local || m_monitoredFolders[i] == folderPath) {
      removeMonitoredFolder(i);
      return;
    }
  }
}

void QtBridge::rescanMonitoredFolder(const QString &folderPath) {
  QString local = folderPath;
  QUrl u = QUrl::fromUserInput(folderPath);
  if (u.isLocalFile())
    local = u.toLocalFile();
  else if (local.startsWith("file://"))
    local = local.mid(7);

  scanDirectory(local.isEmpty() ? folderPath : local);
}

void QtBridge::rescanAllMonitoredFolders() {
  for (const QString &folder : m_monitoredFolders) {
    scanDirectory(folder);
  }
}

void QtBridge::incrementalQuickScan() {
  if (m_isScanning.exchange(true))
    return;
  m_scanScanned = 0;
  m_scanTotal = 0;
  m_scanStatusText = "Quick scan in progress...";
  emit scanningChanged();

  std::vector<std::string> paths;
  for (const auto &folder : m_monitoredFolders) {
    paths.push_back(folder.toStdString());
  }

  std::thread([this, paths]() {
    size_t totalAdded = 0;
    for (const auto &path : paths) {
      totalAdded += m_library.incrementalQuickScan(
          path, [this](size_t scanned, size_t total) {
            QMetaObject::invokeMethod(
                this,
                [this, scanned, total]() {
                  m_scanScanned = static_cast<int>(scanned);
                  m_scanTotal = static_cast<int>(total);
                  m_scanStatusText = QString("Quick scanned %1 / %2 files...")
                                         .arg(scanned)
                                         .arg(total);
                  emit scanningChanged();
                },
                Qt::QueuedConnection);
          });
    }

    QMetaObject::invokeMethod(
        this,
        [this, totalAdded]() {
          m_albumModel->reload();
          m_trackModel->loadAllTracks();
          refreshLibraryStats();
          m_isScanning = false;
          m_scanStatusText = QString("Quick scan complete: %1 new tracks added")
                                 .arg(totalAdded);
          emit scanningChanged();
          emit scanFinished(static_cast<int>(totalAdded));
        },
        Qt::QueuedConnection);
  }).detach();
}

void QtBridge::setGaplessPlayback(bool enable) {
  if (m_gaplessPlayback != enable) {
    m_gaplessPlayback = enable;
    saveSettings();
  }
}

void QtBridge::setCrossfadeEnabled(bool enable) {
  if (m_crossfadeEnabled != enable) {
    m_crossfadeEnabled = enable;
    saveSettings();
  }
}

void QtBridge::setCrossfadeDurationSec(qreal sec) {
  if (!qFuzzyCompare(m_crossfadeDurationSec, sec)) {
    m_crossfadeDurationSec = sec;
    saveSettings();
  }
}

void QtBridge::setCrossfadeCurve(const QString &curve) {
  if (m_crossfadeCurve != curve) {
    m_crossfadeCurve = curve;
    saveSettings();
  }
}

void QtBridge::setReplayGainMode(const QString &mode) {
  if (m_replayGainMode != mode) {
    m_replayGainMode = mode;
    saveSettings();
  }
}

void QtBridge::setReplayGainPreampDb(int db) {
  if (m_replayGainPreampDb != db) {
    m_replayGainPreampDb = db;
    saveSettings();
  }
}

void QtBridge::setReplayGainPreampWithoutGainDb(int db) {
  if (m_replayGainPreampWithoutGainDb != db) {
    m_replayGainPreampWithoutGainDb = db;
    saveSettings();
  }
}

void QtBridge::setTruePeakLimiter(bool enable) {
  if (m_truePeakLimiter != enable) {
    m_truePeakLimiter = enable;
    saveSettings();
  }
}

void QtBridge::setShortSeekStepSec(int sec) {
  if (m_shortSeekStepSec != sec) {
    m_shortSeekStepSec = sec;
    saveSettings();
  }
}

void QtBridge::setLongSeekStepSec(int sec) {
  if (m_longSeekStepSec != sec) {
    m_longSeekStepSec = sec;
    saveSettings();
  }
}

void QtBridge::setStopAfterCurrentTrack(bool enable) {
  if (m_stopAfterCurrentTrack != enable) {
    m_stopAfterCurrentTrack = enable;
    saveSettings();
  }
}

void QtBridge::setDoubleClickAction(const QString &action) {
  if (m_doubleClickAction != action) {
    m_doubleClickAction = action;
    saveSettings();
  }
}

void QtBridge::setMiddleClickAction(const QString &action) {
  if (m_middleClickAction != action) {
    m_middleClickAction = action;
    saveSettings();
  }
}

void QtBridge::setQueueAutoFillMode(const QString &mode) {
  if (m_queueAutoFillMode != mode) {
    m_queueAutoFillMode = mode;
    saveSettings();
  }
}

void QtBridge::setHistoryRetentionLimit(int limit) {
  if (m_historyRetentionLimit != limit) {
    m_historyRetentionLimit = limit;
    m_player.setHistoryLimit(static_cast<size_t>(limit));
    saveSettings();
  }
}

void QtBridge::clearPlaybackHistory() {
  m_player.clearHistory();
  refreshHistory();
  emit queueChanged();
}

void QtBridge::setFilesystemWatcher(bool enable) {
  if (m_filesystemWatcher != enable) {
    m_filesystemWatcher = enable;
    saveSettings();
  }
}

void QtBridge::setAutoScanOnStartup(bool enable) {
  if (m_autoScanOnStartup != enable) {
    m_autoScanOnStartup = enable;
    saveSettings();
  }
}

void QtBridge::setFormatFilters(const QStringList &formats) {
  if (m_formatFilters != formats) {
    m_formatFilters = formats;
    saveSettings();
  }
}

void QtBridge::setFormatFilterEnabled(const QString &format, bool enabled) {
  if (enabled && !m_formatFilters.contains(format, Qt::CaseInsensitive)) {
    m_formatFilters.append(format);
    saveSettings();
  } else if (!enabled &&
             m_formatFilters.contains(format, Qt::CaseInsensitive)) {
    m_formatFilters.removeAll(format);
    saveSettings();
  }
}

bool QtBridge::isFormatFilterEnabled(const QString &format) const {
  for (const QString &f : m_formatFilters) {
    if (f.compare(format, Qt::CaseInsensitive) == 0)
      return true;
  }
  return false;
}

void QtBridge::setExcludeFolders(const QString &patterns) {
  if (m_excludeFolders != patterns) {
    m_excludeFolders = patterns;
    saveSettings();
  }
}

void QtBridge::setArtworkPriority(const QString &priority) {
  if (m_artworkPriority != priority) {
    m_artworkPriority = priority;
    m_library.getMetadataExtractor().setArtworkPriority(
        m_artworkPriority != "External Folder Art First");
    saveSettings();
  }
}

void QtBridge::setGroupAlbumsByTitle(bool enabled) {
  if (m_groupAlbumsByTitle != enabled) {
    m_groupAlbumsByTitle = enabled;
    m_library.getDatabasePort().setGroupAlbumsByTitle(enabled);
    saveSettings();
    emit librarySettingsChanged();
    emit libraryStatsChanged();
    m_albumModel->reload();
  }
}

void QtBridge::optimizeDatabase() {
  m_library.optimizeDatabase();
  m_scanStatusText = "Database optimized (VACUUM & REINDEX complete)";
  emit scanningChanged();
}

void QtBridge::exportDatabaseBackup(const QString &targetFilePath) {
  QString src =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
      "/picaudio_library.db";
  QString dest = targetFilePath;
  QUrl u = QUrl::fromUserInput(targetFilePath);
  if (u.isLocalFile())
    dest = u.toLocalFile();
  if (QFile::exists(dest))
    QFile::remove(dest);
  QFile::copy(src, dest);
}

void QtBridge::setLanguage(const QString &lang) {
  if (m_language != lang) {
    m_language = lang;
    saveSettings();
    applyLanguage(m_language);
    emit settingsChanged();
  }
}

QStringList QtBridge::availableLanguages() const {
  return {QStringLiteral("System Default"), QStringLiteral("English"),
          QStringLiteral("Deutsch"),        QStringLiteral("Español"),
          QStringLiteral("Français"),       QStringLiteral("日本語")};
}

void QtBridge::setQmlEngine(QQmlApplicationEngine *engine) {
  m_qmlEngine = engine;
  if (m_language != QStringLiteral("System Default")) {
    applyLanguage(m_language);
  }
}

void QtBridge::applyLanguage(const QString &lang) {
  if (!m_qmlEngine) {
    return;
  }
  if (!m_translator) {
    m_translator = new QTranslator(this);
    QCoreApplication::installTranslator(m_translator);
  }

  QString langCode;
  if (lang == QStringLiteral("English")) {
    langCode = QStringLiteral("en");
  } else if (lang == QStringLiteral("Deutsch")) {
    langCode = QStringLiteral("de");
  } else if (lang == QStringLiteral("Español")) {
    langCode = QStringLiteral("es");
  } else if (lang == QStringLiteral("Français")) {
    langCode = QStringLiteral("fr");
  } else if (lang == QStringLiteral("日本語")) {
    langCode = QStringLiteral("ja");
  } else {
    langCode = QLocale::system().name().left(2);
  }

  if (m_translator->load(QStringLiteral(":/i18n/picaudio_") + langCode)) {
    m_qmlEngine->retranslate();
  } else if (langCode != QStringLiteral("en")) {
    if (m_translator->load(QStringLiteral(":/i18n/picaudio_en"))) {
      m_qmlEngine->retranslate();
    }
  }
}

void QtBridge::setStartupAction(const QString &action) {
  if (m_startupAction != action) {
    m_startupAction = action;
    saveSettings();
  }
}

void QtBridge::setCloseAction(const QString &action) {
  if (m_closeAction != action) {
    m_closeAction = action;
    saveSettings();
  }
}

void QtBridge::setMinimizeAction(const QString &action) {
  if (m_minimizeAction != action) {
    m_minimizeAction = action;
    saveSettings();
  }
}

void QtBridge::setSingleInstance(bool enable) {
  if (m_singleInstance != enable) {
    m_singleInstance = enable;
    saveSettings();
  }
}

void QtBridge::setUpdateCheckInterval(const QString &interval) {
  if (m_updateCheckInterval != interval) {
    m_updateCheckInterval = interval;
    saveSettings();
  }
}

void QtBridge::setShowNotifications(bool enable) {
  if (m_showNotifications != enable) {
    m_showNotifications = enable;
    saveSettings();
  }
}

void QtBridge::setNotificationDurationSec(int sec) {
  if (m_notificationDurationSec != sec) {
    m_notificationDurationSec = sec;
    saveSettings();
  }
}

void QtBridge::setSuppressNotificationsWhenFocused(bool enable) {
  if (m_suppressNotificationsWhenFocused != enable) {
    m_suppressNotificationsWhenFocused = enable;
    saveSettings();
  }
}

void QtBridge::setFontFamily(const QString &font) {
  if (m_fontFamily != font) {
    m_fontFamily = font;
    saveSettings();
  }
}

void QtBridge::setBaseFontSize(int size) {
  if (m_baseFontSize != size) {
    m_baseFontSize = size;
    saveSettings();
  }
}

void QtBridge::setWaveformSeekbar(bool enable) {
  if (m_waveformSeekbar != enable) {
    m_waveformSeekbar = enable;
    saveSettings();
  }
}

void QtBridge::setTableRowHeight(int height) {
  if (m_tableRowHeight != height) {
    m_tableRowHeight = height;
    saveSettings();
  }
}

void QtBridge::setTableAlternatingRows(bool enable) {
  if (m_tableAlternatingRows != enable) {
    m_tableAlternatingRows = enable;
    saveSettings();
  }
}

void QtBridge::setArtThumbnailQuality(const QString &quality) {
  if (m_artThumbnailQuality != quality) {
    m_artThumbnailQuality = quality;
    saveSettings();
  }
}

void QtBridge::setArtCacheLimitMb(int limitMb) {
  if (m_artCacheLimitMb != limitMb) {
    m_artCacheLimitMb = limitMb;
    saveSettings();
  }
}

void QtBridge::setShowStatusBar(bool enable) {
  if (m_showStatusBar != enable) {
    m_showStatusBar = enable;
    saveSettings();
  }
}

void QtBridge::setAudioBackend(const QString &backend) {
  if (m_audioBackend != backend) {
    m_audioBackend = backend;
    saveSettings();
  }
}

void QtBridge::setCurrentAudioDevice(const QString &device) {
  if (m_currentAudioDevice != device) {
    m_currentAudioDevice = device;
    saveSettings();
  }
}

void QtBridge::setBitPerfectExclusive(bool exclusive) {
  if (m_bitPerfectExclusive != exclusive) {
    m_bitPerfectExclusive = exclusive;
    saveSettings();
  }
}

void QtBridge::setBufferLatencyMs(int ms) {
  if (m_bufferLatencyMs != ms) {
    m_bufferLatencyMs = ms;
    saveSettings();
  }
}

void QtBridge::setResamplerQuality(const QString &quality) {
  if (m_resamplerQuality != quality) {
    m_resamplerQuality = quality;
    saveSettings();
  }
}

void QtBridge::setDitherMode(const QString &mode) {
  if (m_ditherMode != mode) {
    m_ditherMode = mode;
    saveSettings();
  }
}

void QtBridge::setChannelProcessing(const QString &mode) {
  if (m_channelProcessing != mode) {
    m_channelProcessing = mode;
    saveSettings();
  }
}

void QtBridge::clearArtCache() {
  QString coversDir =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
      "/covers";
  QDir dir(coversDir);
  if (dir.exists()) {
    dir.removeRecursively();
    dir.mkpath(".");
  }
}

void QtBridge::checkForUpdates() {
  // Trigger or schedule update check logic
}

void QtBridge::refreshAudioDevices() {
  QStringList devices;
  devices.append("System Default Output");

  const auto outputs = QMediaDevices::audioOutputs();
  for (const auto &dev : outputs) {
    QString desc = dev.description();
    if (!desc.isEmpty() && !devices.contains(desc)) {
      devices.append(desc);
    }
  }
  m_availableAudioDevices = devices;
  emit audioDevicesChanged();
}

void QtBridge::initDefaultHotkeys() {
  m_hotkeys = {
      {"play_pause", "Playback", "Play / Pause playback", "Space", "Space"},
      {"stop", "Playback", "Stop playback", "Ctrl+.", "Ctrl+."},
      {"stop_after_current", "Playback", "Stop after current track",
       "Shift+Space", "Shift+Space"},
      {"next_track", "Playback", "Next track", "Ctrl+Right", "Ctrl+Right"},
      {"prev_track", "Playback", "Previous track (Reversible History)",
       "Ctrl+Left", "Ctrl+Left"},
      {"seek_forward_short", "Playback", "Seek forward (Short)", "Right",
       "Right"},
      {"seek_backward_short", "Playback", "Seek backward (Short)", "Left",
       "Left"},
      {"seek_forward_long", "Playback", "Seek forward (Long)", "Shift+Right",
       "Shift+Right"},
      {"seek_backward_long", "Playback", "Seek backward (Long)", "Shift+Left",
       "Shift+Left"},
      {"seek_beginning", "Playback", "Seek to Beginning of Track", "Home",
       "Home"},
      {"seek_end", "Playback", "Seek to End of Track", "End", "End"},
      {"volume_up", "Playback", "Volume up (+5%)", "Ctrl+Up", "Ctrl+Up"},
      {"volume_down", "Playback", "Volume down (-5%)", "Ctrl+Down",
       "Ctrl+Down"},
      {"toggle_mute", "Playback", "Mute / Unmute audio", "Ctrl+M", "Ctrl+M"},
      {"cycle_repeat", "Playback", "Cycle Repeat mode", "Ctrl+R", "Ctrl+R"},
      {"cycle_shuffle", "Playback", "Cycle Shuffle mode", "Ctrl+S", "Ctrl+S"},
      {"set_loop_a", "Playback", "Set A-B Loop Point A", "[", "["},
      {"set_loop_b", "Playback", "Set A-B Loop Point B", "]", "]"},
      {"clear_loop", "Playback", "Clear A-B Loop", "\\", "\\"},

      {"open_file", "File & Library", "Open Audio File...", "Ctrl+O", "Ctrl+O"},
      {"open_folder", "File & Library", "Open Folder / Directory...",
       "Ctrl+Shift+O", "Ctrl+Shift+O"},
      {"open_url", "File & Library", "Open URL / Network Stream...", "Ctrl+U",
       "Ctrl+U"},
      {"export_view", "File & Library", "Export Active View / Playlist...",
       "Ctrl+E", "Ctrl+E"},
      {"rescan_all", "File & Library", "Rescan All Monitored Folders", "F5",
       "F5"},
      {"quick_scan", "File & Library", "Incremental Quick Scan", "Ctrl+F5",
       "Ctrl+F5"},
      {"create_playlist", "File & Library", "Create New Playlist", "Ctrl+N",
       "Ctrl+N"},
      {"create_smart_playlist", "File & Library", "Create Smart Playlist",
       "Ctrl+Shift+N", "Ctrl+Shift+N"},
      {"minimize_tray", "File & Library", "Minimize to System Tray", "Ctrl+W",
       "Ctrl+W"},
      {"exit_app", "File & Library", "Exit Application", "Ctrl+Q", "Ctrl+Q"},

      {"undo", "Edit & Selection", "Undo Recent Operation", "Ctrl+Z", "Ctrl+Z"},
      {"redo", "Edit & Selection", "Redo Operation", "Ctrl+Y", "Ctrl+Y"},
      {"select_all", "Edit & Selection", "Select All Items in View", "Ctrl+A",
       "Ctrl+A"},
      {"invert_selection", "Edit & Selection", "Invert Selection",
       "Ctrl+Shift+A", "Ctrl+Shift+A"},
      {"clear_selection", "Edit & Selection", "Clear Selection", "Escape",
       "Escape"},
      {"edit_tags", "Edit & Selection", "Edit Track Tags...", "Ctrl+T",
       "Ctrl+T"},
      {"delete_selected", "Edit & Selection", "Delete / Remove Selected",
       "Delete", "Delete"},

      {"toggle_explorer", "View & Navigation", "Toggle Left Library Explorer",
       "Ctrl+1", "Ctrl+1"},
      {"toggle_filter_browser", "View & Navigation",
       "Toggle 3-Column Filter Browser", "Ctrl+2", "Ctrl+2"},
      {"toggle_inspector", "View & Navigation",
       "Toggle Right Audio Specs Inspector", "Ctrl+3", "Ctrl+3"},
      {"toggle_queue", "View & Navigation", "Toggle Up Next Play Queue Panel",
       "Ctrl+4", "Ctrl+4"},
      {"toggle_equalizer", "View & Navigation",
       "Toggle Parametric Equalizer Overlay", "Ctrl+5", "Ctrl+5"},
      {"equalizer_dsp", "View & Navigation", "Open DSP & Parametric Equalizer",
       "Ctrl+Shift+E", "Ctrl+Shift+E"},
      {"focus_search", "View & Navigation", "Focus Instant Search input",
       "Ctrl+F", "Ctrl+F"},
      {"clear_filter_escape", "View & Navigation",
       "Clear filter / Restore View", "Shift+Escape", "Shift+Escape"},
      {"view_track_table", "View & Navigation", "Track Table View", "Alt+1",
       "Alt+1"},
      {"view_album_grid", "View & Navigation", "Album Grid View", "Alt+2",
       "Alt+2"},
      {"view_album_expanded", "View & Navigation", "Album Expanded View",
       "Alt+3", "Alt+3"},
      {"view_history", "View & Navigation", "Listening History View", "Alt+4",
       "Alt+4"},
      {"toggle_mini_player", "View & Navigation", "Toggle Mini-Player Mode",
       "Ctrl+Shift+M", "Ctrl+Shift+M"},
      {"toggle_fullscreen", "View & Navigation", "Toggle Fullscreen Window",
       "F11", "F11"},
      {"zoom_in", "View & Navigation", "Zoom In Global UI Scale", "Ctrl++",
       "Ctrl++"},
      {"zoom_out", "View & Navigation", "Zoom Out Global UI Scale", "Ctrl+-",
       "Ctrl+-"},
      {"zoom_reset", "View & Navigation", "Reset Zoom Scale (100%)", "Ctrl+0",
       "Ctrl+0"},

      {"open_preferences", "Tools & Help", "Open Preferences Dialog", "Ctrl+,",
       "Ctrl+,"},
      {"help_docs", "Tools & Help", "Documentation & User Manual", "F1", "F1"},
      {"help_shortcuts", "Tools & Help", "Keyboard Shortcuts Cheat Sheet",
       "Ctrl+/", "Ctrl+/"},
      {"clear_queue", "Library & Editing", "Clear Playback Queue",
       "Ctrl+Shift+Del", "Ctrl+Shift+Del"},
      {"shuffle_queue", "Library & Editing", "Shuffle Playback Queue",
       "Ctrl+Shift+S", "Ctrl+Shift+S"}};
}

QVariantList QtBridge::hotkeysModel() const {
  QVariantList list;
  list.reserve(static_cast<qsizetype>(m_hotkeys.size()));
  for (const auto &entry : m_hotkeys) {
    QVariantMap map;
    map["id"] = entry.id;
    map["category"] = entry.category;
    map["actionName"] = entry.actionName;
    map["defaultSequence"] = entry.defaultSequence;
    map["currentSequence"] = entry.currentSequence;
    list.append(map);
  }
  return list;
}

void QtBridge::setGlobalMediaKeysEnabled(bool enable) {
  if (m_globalMediaKeysEnabled != enable) {
    m_globalMediaKeysEnabled = enable;
    saveSettings();
    emit hotkeysChanged();
  }
}

void QtBridge::setHotkey(const QString &actionId, const QString &keySequence) {
  for (auto &entry : m_hotkeys) {
    if (entry.id == actionId) {
      if (entry.currentSequence != keySequence) {
        entry.currentSequence = keySequence;
        saveSettings();
        emit hotkeysChanged();
      }
      return;
    }
  }
}

void QtBridge::restoreDefaultHotkeys() {
  for (auto &entry : m_hotkeys) {
    entry.currentSequence = entry.defaultSequence;
  }
  saveSettings();
  emit hotkeysChanged();
}

QString QtBridge::checkHotkeyConflict(const QString &actionId,
                                      const QString &keySequence) const {
  QString clean = keySequence.trimmed();
  if (clean.isEmpty()) {
    return "";
  }
  QKeySequence candidate(clean);
  if (candidate.isEmpty()) {
    return "";
  }

  for (const auto &entry : m_hotkeys) {
    if (entry.id != actionId && !entry.currentSequence.isEmpty()) {
      QKeySequence existing(entry.currentSequence);
      if (candidate == existing ||
          entry.currentSequence.compare(clean, Qt::CaseInsensitive) == 0) {
        return entry.actionName;
      }
    }
  }
  return "";
}

QString QtBridge::getHotkeySequence(const QString &actionId) const {
  for (const auto &entry : m_hotkeys) {
    if (entry.id == actionId) {
      return entry.currentSequence;
    }
  }
  return QString();
}

bool QtBridge::exportHotkeys(const QString &filePath) const {
  QString localPath = filePath;
  if (localPath.startsWith(QStringLiteral("file://"))) {
    localPath = QUrl(filePath).toLocalFile();
  }
  QJsonObject rootObj;
  rootObj[QStringLiteral("version")] = 1;
  rootObj[QStringLiteral("application")] = QStringLiteral("PicAudio");
  QJsonObject keymapObj;
  for (const auto &entry : m_hotkeys) {
    keymapObj[entry.id] = entry.currentSequence;
  }
  rootObj[QStringLiteral("keymaps")] = keymapObj;

  QJsonDocument doc(rootObj);
  QFile file(localPath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return false;
  }
  file.write(doc.toJson(QJsonDocument::Indented));
  file.close();
  return true;
}

bool QtBridge::importHotkeys(const QString &filePath) {
  QString localPath = filePath;
  if (localPath.startsWith(QStringLiteral("file://"))) {
    localPath = QUrl(filePath).toLocalFile();
  }
  QFile file(localPath);
  if (!file.open(QIODevice::ReadOnly)) {
    return false;
  }
  QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
  file.close();
  if (!doc.isObject()) {
    return false;
  }
  QJsonObject rootObj = doc.object();
  QJsonObject keymapObj = rootObj[QStringLiteral("keymaps")].toObject();
  if (keymapObj.isEmpty()) {
    return false;
  }
  bool changed = false;
  for (auto &entry : m_hotkeys) {
    if (keymapObj.contains(entry.id)) {
      QString newSeq = keymapObj.value(entry.id).toString();
      if (entry.currentSequence != newSeq) {
        entry.currentSequence = newSeq;
        changed = true;
      }
    }
  }
  if (changed) {
    saveSettings();
    emit hotkeysChanged();
  }
  return true;
}

void QtBridge::showToast(const QString &message, const QString &type) {
  emit toastRequested(message, type);
}

void QtBridge::setLastfmEnabled(bool enable) {
  if (m_lastfmEnabled != enable) {
    m_lastfmEnabled = enable;
    saveSettings();
    emit metadataSettingsChanged();
  }
}

void QtBridge::setLastfmUsername(const QString &user) {
  if (m_lastfmUsername != user) {
    m_lastfmUsername = user;
    saveSettings();
    emit metadataSettingsChanged();
  }
}

void QtBridge::setLastfmSessionKey(const QString &key) {
  if (m_lastfmSessionKey != key) {
    m_lastfmSessionKey = key;
    saveSettings();
    emit metadataSettingsChanged();
  }
}

void QtBridge::setListenbrainzEnabled(bool enable) {
  if (m_listenbrainzEnabled != enable) {
    m_listenbrainzEnabled = enable;
    saveSettings();
    emit metadataSettingsChanged();
  }
}

void QtBridge::setListenbrainzToken(const QString &token) {
  if (m_listenbrainzToken != token) {
    m_listenbrainzToken = token;
    saveSettings();
    emit metadataSettingsChanged();
  }
}

void QtBridge::setListenbrainzApiUrl(const QString &url) {
  if (m_listenbrainzApiUrl != url) {
    m_listenbrainzApiUrl = url;
    saveSettings();
    emit metadataSettingsChanged();
  }
}

void QtBridge::setScrobbleThresholdPercent(int percent) {
  if (m_scrobbleThresholdPercent != percent) {
    m_scrobbleThresholdPercent = percent;
    saveSettings();
    emit metadataSettingsChanged();
  }
}

void QtBridge::setScrobbleThresholdTimeSec(int sec) {
  if (m_scrobbleThresholdTimeSec != sec) {
    m_scrobbleThresholdTimeSec = sec;
    saveSettings();
    emit metadataSettingsChanged();
  }
}

void QtBridge::setOfflineScrobbleCache(bool enable) {
  if (m_offlineScrobbleCache != enable) {
    m_offlineScrobbleCache = enable;
    saveSettings();
    emit metadataSettingsChanged();
  }
}

void QtBridge::setLyricsProviderOrder(const QString &order) {
  if (m_lyricsProviderOrder != order) {
    m_lyricsProviderOrder = order;
    saveSettings();
    emit metadataSettingsChanged();
  }
}

void QtBridge::setAutoFetchLyrics(bool enable) {
  if (m_autoFetchLyrics != enable) {
    m_autoFetchLyrics = enable;
    saveSettings();
    emit metadataSettingsChanged();
  }
}

void QtBridge::setLoggingVerbosity(const QString &level) {
  if (m_loggingVerbosity != level) {
    m_loggingVerbosity = level;
    saveSettings();
    emit diagnosticsSettingsChanged();
  }
}

void QtBridge::openLogDirectory() {
  QString logDir =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
      "/logs";
  QDir().mkpath(logDir);
  QDesktopServices::openUrl(QUrl::fromLocalFile(logDir));
}

void QtBridge::exportDiagnosticsReport(const QString &targetFilePath) {
  QString dest = targetFilePath;
  QUrl u = QUrl::fromUserInput(targetFilePath);
  if (u.isLocalFile()) {
    dest = u.toLocalFile();
  }

  QFile file(dest);
  if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream out(&file);
    out << "PicAudio Player - Diagnostics Report\n";
    out << "==========================================\n\n";
    out << "OS: " << QSysInfo::prettyProductName() << " ("
        << QSysInfo::currentCpuArchitecture() << ")\n";
    out << "Kernel: " << QSysInfo::kernelType() << " "
        << QSysInfo::kernelVersion() << "\n";
    out << "Qt Version: " << QT_VERSION_STR << "\n\n";

    out << "[Audio Pipeline]\n";
    out << "Backend: " << m_audioBackend << "\n";
    out << "Device: " << m_currentAudioDevice << "\n";
    out << "Bit-Perfect Exclusive: "
        << (m_bitPerfectExclusive ? "Enabled" : "Disabled") << "\n";
    out << "Buffer Latency: " << m_bufferLatencyMs << " ms\n";
    out << "Resampler Quality: " << m_resamplerQuality << "\n";
    out << "Dither Mode: " << m_ditherMode << "\n";
    out << "Channel Processing: " << m_channelProcessing << "\n\n";

    out << "[Library & Storage]\n";
    out << "Indexed Tracks: " << totalTracks() << "\n";
    out << "Indexed Albums: " << totalAlbums() << "\n";
    out << "Total Library Duration: " << totalDurationStr() << "\n";
    out << "Monitored Folders Count: " << m_monitoredFolders.size() << "\n";
    for (const auto &folder : m_monitoredFolders) {
      out << "  - " << folder << "\n";
    }
    out << "Filesystem Watcher: "
        << (m_filesystemWatcher ? "Active" : "Disabled") << "\n";
    out << "Auto-scan on Startup: "
        << (m_autoScanOnStartup ? "Enabled" : "Disabled") << "\n\n";

    out << "[Scrobbling & Services]\n";
    out << "Last.fm Scrobbling: " << (m_lastfmEnabled ? "Enabled" : "Disabled")
        << " (User: " << m_lastfmUsername << ")\n";
    out << "ListenBrainz Scrobbling: "
        << (m_listenbrainzEnabled ? "Enabled" : "Disabled") << "\n";
    out << "Offline Scrobble Cache: "
        << (m_offlineScrobbleCache ? "Enabled" : "Disabled") << "\n";
    out << "Lyrics Provider Order: " << m_lyricsProviderOrder << "\n";
    out << "Auto-fetch Lyrics: " << (m_autoFetchLyrics ? "Enabled" : "Disabled")
        << "\n\n";

    out << "[Diagnostics & Logging]\n";
    out << "Logging Verbosity: " << m_loggingVerbosity << "\n\n";

    out << "[Hotkeys & Accelerators]\n";
    out << "Global Media Keys: "
        << (m_globalMediaKeysEnabled ? "Enabled" : "Disabled") << "\n";
    for (const auto &hk : m_hotkeys) {
      out << "  - [" << hk.category << "] " << hk.actionName << ": "
          << hk.currentSequence << " (Default: " << hk.defaultSequence << ")\n";
    }
    file.close();
  }
}

void QtBridge::openAudioDiagnostics() { emit audioDiagnosticsRequested(); }

QString QtBridge::checkDatabaseIntegrity() {
  bool ok = m_library.optimizeDatabase();
  if (ok) {
    return QString(
        "Integrity check passed: Database schema, tables, and indices are "
        "valid and healthy.");
  }
  return QString(
      "Integrity check warning: SQLite database reported warnings during "
      "verification.");
}

void QtBridge::resetAllSettingsToDefaults() {
  QSettings s("PicAudio", "PicAudio");
  s.clear();

  // Playback & DSP defaults
  m_gaplessPlayback = true;
  m_crossfadeEnabled = false;
  m_crossfadeDurationSec = 2.0;
  m_crossfadeCurve = "Equal Power (Constant Volume)";
  m_replayGainMode = "Smart Gain (Auto Track/Album)";
  m_replayGainPreampDb = 0;
  m_replayGainPreampWithoutGainDb = -6;
  m_truePeakLimiter = true;
  m_shortSeekStepSec = 5;
  m_longSeekStepSec = 30;
  m_stopAfterCurrentTrack = false;

  // Queue defaults
  m_doubleClickAction = "Play Now";
  m_middleClickAction = "Queue Last";
  m_queueAutoFillMode = "Loop Context";
  m_historyRetentionLimit = 200;
  m_player.setHistoryLimit(200);

  // Library defaults
  m_monitoredFolders.clear();
  m_filesystemWatcher = true;
  m_autoScanOnStartup = false;
  m_formatFilters = {"FLAC", "WAV", "ALAC", "AIFF", "DSD (DSF/DFF)",
                     "MP3",  "AAC", "M4A",  "OGG",  "OPUS"};
  m_excludeFolders =
      ".*, node_modules, temp, @eaDir, System Volume Information";
  m_artworkPriority = "Embedded Tags First";
  m_groupAlbumsByTitle = true;
  m_library.getDatabasePort().setGroupAlbumsByTitle(true);

  // General defaults
  m_language = "System Default";
  m_startupAction = "Restore Session";
  m_closeAction = "Exit Application";
  m_minimizeAction = "Minimize to Taskbar";
  m_singleInstance = true;
  m_updateCheckInterval = "Weekly";
  m_showNotifications = true;
  m_notificationDurationSec = 4;
  m_suppressNotificationsWhenFocused = true;

  // Appearance defaults
  m_fontFamily = "Inter, sans-serif";
  m_baseFontSize = 11;
  m_waveformSeekbar = true;
  m_tableRowHeight = 32;
  m_tableAlternatingRows = false;
  m_artThumbnailQuality = "Smooth (High Quality)";
  m_artCacheLimitMb = 1024;
  m_showStatusBar = true;

  if (m_themeLoader) {
    m_themeLoader->setThemeId("dark-studio");
    m_themeLoader->setUiScale(1.0);
    m_themeLoader->setDensityPreset("Standard");
  }

  // Audio Output defaults
  m_audioBackend = "Linux PipeWire Lock-Free Client";
  m_currentAudioDevice = "System Default Output";
  m_bitPerfectExclusive = true;
  m_bufferLatencyMs = 50;
  m_resamplerQuality = "SoX Resampler High Quality";
  m_ditherMode = "Flat TPDF (Triangular)";
  m_channelProcessing = "Stereo Passthrough";

  // Hotkeys defaults
  m_globalMediaKeysEnabled = true;
  for (auto &hk : m_hotkeys) {
    hk.currentSequence = hk.defaultSequence;
  }

  // Metadata & Scrobbling defaults
  m_lastfmEnabled = false;
  m_lastfmUsername = "";
  m_lastfmSessionKey = "";
  m_listenbrainzEnabled = false;
  m_listenbrainzToken = "";
  m_listenbrainzApiUrl = "https://api.listenbrainz.org/1/";
  m_scrobbleThresholdPercent = 50;
  m_scrobbleThresholdTimeSec = 240;
  m_offlineScrobbleCache = true;
  m_lyricsProviderOrder = "Local .lrc sidecar first";
  m_autoFetchLyrics = true;

  // Diagnostics defaults
  m_loggingVerbosity = "Info";

  saveSettings();

  emit settingsChanged();
  emit playbackSettingsChanged();
  emit queueSettingsChanged();
  emit librarySettingsChanged();
  emit audioSettingsChanged();
  emit hotkeysChanged();
  emit metadataSettingsChanged();
  emit diagnosticsSettingsChanged();
}

void QtBridge::initTableColumns() {
  m_columns = {
      {"trackNumber", "Track #", "#", true, 36, 36, true},
      {"title", "Title", "TITLE", true, 200, 200, true},
      {"artist", "Artist", "ARTIST", true, 130, 130, true},
      {"album", "Album", "ALBUM", true, 130, 130, true},
      {"albumArtist", "Album Artist", "ALBUM ARTIST", false, 120, 120, false},
      {"genre", "Genre", "GENRE", true, 90, 90, true},
      {"year", "Year", "YEAR", true, 46, 46, true},
      {"codec", "Format / Codec", "FORMAT", true, 72, 72, true},
      {"bitrate", "Bitrate", "BITRATE", true, 64, 64, true},
      {"sampleRate", "Sample Rate", "SAMPLE RATE", false, 80, 80, false},
      {"bitDepth", "Bit Depth", "BIT DEPTH", false, 60, 60, false},
      {"playCount", "Play Count", "PLAYS", false, 52, 52, false},
      {"rating", "Rating", "RATING", false, 72, 72, false},
      {"dateAdded", "Date Added", "DATE ADDED", false, 90, 90, false},
      {"filePath", "File Path", "FILE PATH", false, 180, 180, false},
      {"fileSize", "File Size", "SIZE", false, 64, 64, false},
      {"duration", "Duration", "TIME", true, 50, 50, true}};
}

QVariantMap QtBridge::tableColumns() const {
  QVariantMap map;
  for (const auto &col : m_columns) {
    QVariantMap item;
    item["key"] = col.key;
    item["title"] = col.title;
    item["header"] = col.header;
    item["visible"] = col.visible;
    item["width"] = col.width;
    map[col.key] = item;
  }
  return map;
}

QStringList QtBridge::tableColumnKeys() const {
  QStringList keys;
  keys.reserve(static_cast<qsizetype>(m_columns.size()));
  for (const auto &col : m_columns) {
    keys.append(col.key);
  }
  return keys;
}

bool QtBridge::isColumnVisible(const QString &colKey) const {
  for (const auto &col : m_columns) {
    if (col.key == colKey)
      return col.visible;
  }
  return false;
}

int QtBridge::getColumnWidth(const QString &colKey) const {
  for (const auto &col : m_columns) {
    if (col.key == colKey)
      return col.width;
  }
  return 100;
}

void QtBridge::setColumnVisible(const QString &colKey, bool visible) {
  bool changed = false;
  for (auto &col : m_columns) {
    if (col.key == colKey && col.visible != visible) {
      col.visible = visible;
      changed = true;
      break;
    }
  }
  if (changed) {
    saveColumnSettings();
    emit tableColumnsChanged();
  }
}

void QtBridge::toggleColumnVisible(const QString &colKey) {
  for (auto &col : m_columns) {
    if (col.key == colKey) {
      col.visible = !col.visible;
      saveColumnSettings();
      emit tableColumnsChanged();
      break;
    }
  }
}

void QtBridge::setColumnWidth(const QString &colKey, int width) {
  for (auto &col : m_columns) {
    if (col.key == colKey) {
      col.width = std::max(20, width);
      saveColumnSettings();
      emit tableColumnsChanged();
      break;
    }
  }
}

void QtBridge::autoSizeColumn(const QString &colKey) {
  auto tracks = m_trackModel ? m_trackModel->tracks() : m_library.getTracks();
  for (auto &col : m_columns) {
    if (col.key == colKey) {
      int optimalWidth = col.defaultWidth;
      size_t maxLen = col.header.length();
      for (const auto &t : tracks) {
        if (colKey == "title")
          maxLen = std::max(maxLen, t.title.length());
        else if (colKey == "artist")
          maxLen = std::max(maxLen, t.artist.length());
        else if (colKey == "album")
          maxLen = std::max(maxLen, t.album.length());
        else if (colKey == "albumArtist")
          maxLen = std::max(maxLen, t.albumArtist.length());
        else if (colKey == "genre")
          maxLen = std::max(maxLen, t.genre.length());
        else if (colKey == "filePath")
          maxLen = std::max(maxLen, t.filePath.length());
      }
      if (colKey == "title")
        optimalWidth = std::clamp(static_cast<int>(maxLen * 7 + 24), 120, 360);
      else if (colKey == "artist" || colKey == "album" ||
               colKey == "albumArtist")
        optimalWidth = std::clamp(static_cast<int>(maxLen * 7 + 24), 80, 260);
      else if (colKey == "genre")
        optimalWidth = std::clamp(static_cast<int>(maxLen * 7 + 20), 70, 160);
      else if (colKey == "filePath")
        optimalWidth = std::clamp(static_cast<int>(maxLen * 6 + 20), 140, 400);

      col.width = optimalWidth;
      break;
    }
  }
  saveColumnSettings();
  emit tableColumnsChanged();
}

void QtBridge::autoSizeAllColumns() {
  auto tracks = m_trackModel ? m_trackModel->tracks() : m_library.getTracks();
  for (auto &col : m_columns) {
    if (!col.visible)
      continue;
    int optimalWidth = col.defaultWidth;
    size_t maxLen = col.header.length();
    for (const auto &t : tracks) {
      if (col.key == "title")
        maxLen = std::max(maxLen, t.title.length());
      else if (col.key == "artist")
        maxLen = std::max(maxLen, t.artist.length());
      else if (col.key == "album")
        maxLen = std::max(maxLen, t.album.length());
      else if (col.key == "albumArtist")
        maxLen = std::max(maxLen, t.albumArtist.length());
      else if (col.key == "genre")
        maxLen = std::max(maxLen, t.genre.length());
      else if (col.key == "filePath")
        maxLen = std::max(maxLen, t.filePath.length());
    }
    if (col.key == "title")
      optimalWidth = std::clamp(static_cast<int>(maxLen * 7 + 24), 120, 360);
    else if (col.key == "artist" || col.key == "album" ||
             col.key == "albumArtist")
      optimalWidth = std::clamp(static_cast<int>(maxLen * 7 + 24), 80, 260);
    else if (col.key == "genre")
      optimalWidth = std::clamp(static_cast<int>(maxLen * 7 + 20), 70, 160);
    else if (col.key == "filePath")
      optimalWidth = std::clamp(static_cast<int>(maxLen * 6 + 20), 140, 400);

    col.width = optimalWidth;
  }
  saveColumnSettings();
  emit tableColumnsChanged();
}

void QtBridge::resetTableColumns() {
  initTableColumns();
  if (m_trackModel) {
    m_trackModel->clearSort();
  }
  saveColumnSettings();
  emit tableColumnsChanged();
}

void QtBridge::moveColumn(int fromIndex, int toIndex) {
  if (fromIndex >= 0 && fromIndex < static_cast<int>(m_columns.size()) &&
      toIndex >= 0 && toIndex < static_cast<int>(m_columns.size()) &&
      fromIndex != toIndex) {
    auto col = m_columns[static_cast<size_t>(fromIndex)];
    m_columns.erase(m_columns.begin() + fromIndex);
    m_columns.insert(m_columns.begin() + toIndex, col);
    saveColumnSettings();
    emit tableColumnsChanged();
  }
}

void QtBridge::setColumnOrder(const QStringList &orderKeys) {
  if (orderKeys.isEmpty())
    return;
  std::vector<ColumnMeta> reordered;
  reordered.reserve(m_columns.size());
  for (const auto &key : orderKeys) {
    for (const auto &col : m_columns) {
      if (col.key == key) {
        reordered.push_back(col);
        break;
      }
    }
  }
  for (const auto &col : m_columns) {
    bool found = false;
    for (const auto &r : reordered) {
      if (r.key == col.key) {
        found = true;
        break;
      }
    }
    if (!found) {
      reordered.push_back(col);
    }
  }
  m_columns = std::move(reordered);
  saveColumnSettings();
  emit tableColumnsChanged();
}

QStringList QtBridge::tableColumnOrder() const { return tableColumnKeys(); }

void QtBridge::loadColumnSettings() {
  QSettings s("PicAudio", "PicAudio");
  s.beginGroup("table_columns");

  QStringList savedOrder = s.value("order").toStringList();
  if (!savedOrder.isEmpty()) {
    std::vector<ColumnMeta> reordered;
    reordered.reserve(m_columns.size());
    for (const auto &k : savedOrder) {
      for (const auto &col : m_columns) {
        if (col.key == k) {
          reordered.push_back(col);
          break;
        }
      }
    }
    for (const auto &col : m_columns) {
      bool found = false;
      for (const auto &r : reordered) {
        if (r.key == col.key) {
          found = true;
          break;
        }
      }
      if (!found) {
        reordered.push_back(col);
      }
    }
    m_columns = std::move(reordered);
  }

  for (auto &col : m_columns) {
    if (s.contains(col.key + "/visible")) {
      col.visible = s.value(col.key + "/visible").toBool();
    }
    if (s.contains(col.key + "/width")) {
      col.width = s.value(col.key + "/width").toInt();
    }
  }

  QString sortCol = s.value("sort/column", "").toString();
  bool sortAsc = s.value("sort/ascending", true).toBool();
  if (!sortCol.isEmpty() && m_trackModel) {
    m_trackModel->sortBy(sortCol, sortAsc);
  }

  s.endGroup();
}

void QtBridge::saveColumnSettings() {
  QSettings s("PicAudio", "PicAudio");
  s.beginGroup("table_columns");
  s.setValue("order", tableColumnOrder());
  for (const auto &col : m_columns) {
    s.setValue(col.key + "/visible", col.visible);
    s.setValue(col.key + "/width", col.width);
  }
  if (m_trackModel) {
    s.setValue("sort/column", m_trackModel->sortColumn());
    s.setValue("sort/ascending", m_trackModel->sortAscending());
  }
  s.endGroup();
  s.sync();
}

void QtBridge::saveWindowGeometry(int x, int y, int width, int height,
                                  bool isMaximized) {
  QSettings s("PicAudio", "PicAudio");
  s.beginGroup("window_geometry");
  s.setValue("x", x);
  s.setValue("y", y);
  s.setValue("width", width);
  s.setValue("height", height);
  s.setValue("maximized", isMaximized);
  s.endGroup();
  s.sync();
}

QVariantMap QtBridge::loadWindowGeometry(int defaultWidth, int defaultHeight) {
  QSettings s("PicAudio", "PicAudio");
  s.beginGroup("window_geometry");
  bool hasSaved = s.contains("width") && s.contains("height");
  int x = s.value("x", 0).toInt();
  int y = s.value("y", 0).toInt();
  int w = s.value("width", defaultWidth).toInt();
  int h = s.value("height", defaultHeight).toInt();
  bool isMaximized = s.value("maximized", false).toBool();
  s.endGroup();

  if (w < 400)
    w = defaultWidth;
  if (h < 300)
    h = defaultHeight;

  QRect savedRect(x, y, w, h);
  const auto screens = QGuiApplication::screens();
  bool intersectsActiveScreen = screens.isEmpty();
  for (const auto *screen : screens) {
    if (!screen)
      continue;
    QRect avail = screen->availableGeometry();
    QRect inter = avail.intersected(savedRect);
    if (inter.width() >= 100 && inter.height() >= 100) {
      intersectsActiveScreen = true;
      break;
    }
  }

  if (!hasSaved || !intersectsActiveScreen) {
    QScreen *primary = QGuiApplication::primaryScreen();
    QRect avail =
        primary ? primary->availableGeometry() : QRect(0, 0, 1920, 1080);
    w = std::min(w, avail.width());
    h = std::min(h, avail.height());
    x = avail.x() + (avail.width() - w) / 2;
    y = avail.y() + (avail.height() - h) / 2;
  }

  QVariantMap res;
  res["x"] = x;
  res["y"] = y;
  res["width"] = w;
  res["height"] = h;
  res["maximized"] = isMaximized;
  res["valid"] = hasSaved;
  return res;
}

void QtBridge::setLeftPanelWidth(int width) {
  int clamped = std::clamp(width, 140, 450);
  if (m_leftPanelWidth != clamped) {
    m_leftPanelWidth = clamped;
    saveViewLayoutSettings();
    emit viewLayoutChanged();
  }
}

void QtBridge::setShowLeftPanel(bool show) {
  if (m_showLeftPanel != show) {
    m_showLeftPanel = show;
    saveViewLayoutSettings();
    emit viewLayoutChanged();
  }
}

void QtBridge::setRightPanelWidth(int width) {
  int clamped = std::clamp(width, 180, 500);
  if (m_rightPanelWidth != clamped) {
    m_rightPanelWidth = clamped;
    saveViewLayoutSettings();
    emit viewLayoutChanged();
  }
}

void QtBridge::setShowRightPanel(bool show) {
  if (m_showRightPanel != show) {
    m_showRightPanel = show;
    saveViewLayoutSettings();
    emit viewLayoutChanged();
  }
}

void QtBridge::setColumnBrowserHeight(int height) {
  int clamped = std::clamp(height, 70, 350);
  if (m_columnBrowserHeight != clamped) {
    m_columnBrowserHeight = clamped;
    saveViewLayoutSettings();
    emit viewLayoutChanged();
  }
}

void QtBridge::setShowColumnBrowser(bool show) {
  if (m_showColumnBrowser != show) {
    m_showColumnBrowser = show;
    saveViewLayoutSettings();
    emit viewLayoutChanged();
  }
}

void QtBridge::setActiveWorkspaceViewMode(int mode) {
  if (m_activeWorkspaceViewMode != mode) {
    m_activeWorkspaceViewMode = mode;
    saveViewLayoutSettings();
    emit viewLayoutChanged();
  }
}

void QtBridge::setActiveNavSection(int nav) {
  if (m_activeNavSection != nav) {
    m_activeNavSection = nav;
    saveViewLayoutSettings();
    emit viewLayoutChanged();
  }
}

void QtBridge::setActivePlaylistName(const QString &name) {
  if (m_activePlaylistName != name) {
    m_activePlaylistName = name;
    saveViewLayoutSettings();
    emit viewLayoutChanged();
  }
}

void QtBridge::loadViewLayoutSettings() {
  QSettings s("PicAudio", "PicAudio");
  s.beginGroup("view_layout");
  m_leftPanelWidth = s.value("leftPanelWidth", 210).toInt();
  m_showLeftPanel = s.value("showLeftPanel", true).toBool();
  m_rightPanelWidth = s.value("rightPanelWidth", 270).toInt();
  m_showRightPanel = s.value("showRightPanel", true).toBool();
  m_columnBrowserHeight = s.value("columnBrowserHeight", 121).toInt();
  m_showColumnBrowser = s.value("showColumnBrowser", false).toBool();
  m_activeWorkspaceViewMode = s.value("activeWorkspaceViewMode", 1).toInt();
  m_activeNavSection = s.value("activeNavSection", 0).toInt();
  m_activePlaylistName = s.value("activePlaylistName", "").toString();
  s.endGroup();
}

void QtBridge::saveViewLayoutSettings() {
  QSettings s("PicAudio", "PicAudio");
  s.beginGroup("view_layout");
  s.setValue("leftPanelWidth", m_leftPanelWidth);
  s.setValue("showLeftPanel", m_showLeftPanel);
  s.setValue("rightPanelWidth", m_rightPanelWidth);
  s.setValue("showRightPanel", m_showRightPanel);
  s.setValue("columnBrowserHeight", m_columnBrowserHeight);
  s.setValue("showColumnBrowser", m_showColumnBrowser);
  s.setValue("activeWorkspaceViewMode", m_activeWorkspaceViewMode);
  s.setValue("activeNavSection", m_activeNavSection);
  s.setValue("activePlaylistName", m_activePlaylistName);
  s.endGroup();
  s.sync();
}

void QtBridge::savePlaybackSession() {
  QSettings s("PicAudio", "PicAudio");
  s.beginGroup("session");

  auto currentTrackOpt = m_player.getCurrentTrack();
  if (currentTrackOpt.has_value()) {
    s.setValue("has_session", true);
    s.setValue("track_id", QString::fromStdString(currentTrackOpt->id));
    s.setValue("track_path", QString::fromStdString(currentTrackOpt->filePath));
    s.setValue("track_title", QString::fromStdString(currentTrackOpt->title));
    s.setValue("track_artist", QString::fromStdString(currentTrackOpt->artist));
    s.setValue("track_album", QString::fromStdString(currentTrackOpt->album));
    s.setValue("position_ms", static_cast<qint64>(m_player.getPositionMs()));
  } else {
    s.setValue("has_session", false);
  }

  float volToSave =
      m_isMuted ? (m_prevVolume > 0.0f ? m_prevVolume : 0.8f) : m_volume;
  s.setValue("volume", volToSave);
  s.setValue("muted", m_isMuted);
  s.setValue("shuffle_mode", static_cast<int>(m_player.getShuffleMode()));
  s.setValue("repeat_mode", static_cast<int>(m_player.getRepeatMode()));

  // Up Next Queue serialization
  const auto &upNext = m_player.getQueueService().getUpNext();
  QJsonArray upNextArr;
  for (const auto &t : upNext) {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(t.id);
    obj["filePath"] = QString::fromStdString(t.filePath);
    obj["title"] = QString::fromStdString(t.title);
    obj["artist"] = QString::fromStdString(t.artist);
    obj["album"] = QString::fromStdString(t.album);
    obj["durationMs"] = static_cast<qint64>(t.durationMs);
    upNextArr.append(obj);
  }
  s.setValue("up_next", QString::fromUtf8(QJsonDocument(upNextArr).toJson(
                            QJsonDocument::Compact)));

  // Main Stream serialization
  const auto &mainStream = m_player.getQueueService().getMainStream();
  QJsonArray mainStreamArr;
  for (const auto &t : mainStream) {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(t.id);
    obj["filePath"] = QString::fromStdString(t.filePath);
    obj["title"] = QString::fromStdString(t.title);
    obj["artist"] = QString::fromStdString(t.artist);
    obj["album"] = QString::fromStdString(t.album);
    obj["durationMs"] = static_cast<qint64>(t.durationMs);
    mainStreamArr.append(obj);
  }
  s.setValue("main_stream",
             QString::fromUtf8(
                 QJsonDocument(mainStreamArr).toJson(QJsonDocument::Compact)));
  s.setValue("main_stream_index",
             static_cast<int>(m_player.getQueueService().getMainStreamIndex()));

  // History stack serialization
  const auto &history = m_player.getQueueService().getHistory();
  QJsonArray historyArr;
  for (const auto &t : history) {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(t.id);
    obj["filePath"] = QString::fromStdString(t.filePath);
    obj["title"] = QString::fromStdString(t.title);
    obj["artist"] = QString::fromStdString(t.artist);
    obj["album"] = QString::fromStdString(t.album);
    obj["durationMs"] = static_cast<qint64>(t.durationMs);
    historyArr.append(obj);
  }
  s.setValue("history",
             QString::fromUtf8(
                 QJsonDocument(historyArr).toJson(QJsonDocument::Compact)));

  s.endGroup();
  s.sync();
}

void QtBridge::restorePlaybackSession() {
  if (m_startupAction == "Open Empty") {
    return;
  }

  QSettings s("PicAudio", "PicAudio");
  s.beginGroup("session");

  if (!s.value("has_session", false).toBool()) {
    s.endGroup();
    return;
  }

  float vol = s.value("volume", 0.8f).toFloat();
  bool muted = s.value("muted", false).toBool();
  int sMode = s.value("shuffle_mode", 0).toInt();
  int rMode = s.value("repeat_mode", 0).toInt();

  if (muted) {
    m_prevVolume = vol;
    setVolume(0.0f);
    m_isMuted = true;
  } else {
    setVolume(vol);
    m_isMuted = false;
  }
  setShuffleMode(sMode);
  setRepeatMode(rMode);

  QString trackId = s.value("track_id").toString();
  QString trackPath = s.value("track_path").toString();
  qint64 posMs = s.value("position_ms", 0).toLongLong();

  std::optional<core::Track> currentTrack;
  if (!trackId.isEmpty()) {
    currentTrack = m_library.getTrackById(trackId.toStdString());
  }
  if (!currentTrack.has_value() && !trackPath.isEmpty()) {
    currentTrack =
        m_library.getDatabasePort().getTrackByPath(trackPath.toStdString());
  }
  if (!currentTrack.has_value() && !trackPath.isEmpty()) {
    core::Track t;
    t.id = trackId.toStdString();
    t.filePath = trackPath.toStdString();
    t.title = s.value("track_title").toString().toStdString();
    t.artist = s.value("track_artist").toString().toStdString();
    t.album = s.value("track_album").toString().toStdString();
    currentTrack = t;
  }

  // Parse up_next
  std::vector<core::Track> upNext;
  QString upNextJson = s.value("up_next").toString();
  if (!upNextJson.isEmpty()) {
    auto doc = QJsonDocument::fromJson(upNextJson.toUtf8());
    for (const auto &val : doc.array()) {
      auto obj = val.toObject();
      std::string id = obj["id"].toString().toStdString();
      auto opt = m_library.getTrackById(id);
      if (opt.has_value()) {
        upNext.push_back(*opt);
      } else {
        core::Track t;
        t.id = id;
        t.filePath = obj["filePath"].toString().toStdString();
        t.title = obj["title"].toString().toStdString();
        t.artist = obj["artist"].toString().toStdString();
        t.album = obj["album"].toString().toStdString();
        t.durationMs = static_cast<uint64_t>(obj["durationMs"].toInteger());
        upNext.push_back(t);
      }
    }
  }

  // Parse main_stream
  std::vector<core::Track> mainStream;
  QString mainStreamJson = s.value("main_stream").toString();
  if (!mainStreamJson.isEmpty()) {
    auto doc = QJsonDocument::fromJson(mainStreamJson.toUtf8());
    for (const auto &val : doc.array()) {
      auto obj = val.toObject();
      std::string id = obj["id"].toString().toStdString();
      auto opt = m_library.getTrackById(id);
      if (opt.has_value()) {
        mainStream.push_back(*opt);
      } else {
        core::Track t;
        t.id = id;
        t.filePath = obj["filePath"].toString().toStdString();
        t.title = obj["title"].toString().toStdString();
        t.artist = obj["artist"].toString().toStdString();
        t.album = obj["album"].toString().toStdString();
        t.durationMs = static_cast<uint64_t>(obj["durationMs"].toInteger());
        mainStream.push_back(t);
      }
    }
  }
  size_t mainStreamIndex =
      static_cast<size_t>(s.value("main_stream_index", 0).toInt());

  // Parse history
  std::vector<core::Track> history;
  QString historyJson = s.value("history").toString();
  if (!historyJson.isEmpty()) {
    auto doc = QJsonDocument::fromJson(historyJson.toUtf8());
    for (const auto &val : doc.array()) {
      auto obj = val.toObject();
      std::string id = obj["id"].toString().toStdString();
      auto opt = m_library.getTrackById(id);
      if (opt.has_value()) {
        history.push_back(*opt);
      } else {
        core::Track t;
        t.id = id;
        t.filePath = obj["filePath"].toString().toStdString();
        t.title = obj["title"].toString().toStdString();
        t.artist = obj["artist"].toString().toStdString();
        t.album = obj["album"].toString().toStdString();
        t.durationMs = static_cast<uint64_t>(obj["durationMs"].toInteger());
        history.push_back(t);
      }
    }
  }

  s.endGroup();

  bool autoResume = (m_startupAction == "Play Immediately");
  m_player.restoreSession(currentTrack, static_cast<uint64_t>(posMs), upNext,
                          mainStream, mainStreamIndex, history, autoResume);

  if (currentTrack.has_value()) {
    updatePlaybackState(*currentTrack);
    m_positionMs = posMs;
    m_positionStr = formatTime(m_positionMs);
    emit positionChanged();
  }
  syncQueueModel();
  emit queueChanged();
}

void QtBridge::loadLyricsForTrack(const QString &filePath) {
  if (filePath.isEmpty()) {
    m_hasLyrics = false;
    m_isLyricsSynchronized = false;
    m_currentLyrics = "";
    m_lyricsList.clear();
    m_currentLyricsLines.clear();
    m_activeLyricIndex = -1;
    emit lyricsChanged();
    emit activeLyricIndexChanged();
    return;
  }

  QFileInfo fileInfo(filePath);
  QString basePath = fileInfo.path() + "/" + fileInfo.completeBaseName();

  // Try sidecar .lrc or .txt first
  QString lrcPath = basePath + ".lrc";
  if (QFile::exists(lrcPath)) {
    QFile file(lrcPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QString content = QString::fromUtf8(file.readAll());
      parseLrcContent(content);
      return;
    }
  }

  QString txtPath = basePath + ".txt";
  if (QFile::exists(txtPath)) {
    QFile file(txtPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QString content = QString::fromUtf8(file.readAll());
      parseLrcContent(content);
      return;
    }
  }

  // Try embedded metadata tags
  std::string embeddedLyrics =
      m_library.getMetadataExtractor().extractLyrics(filePath.toStdString());
  if (!embeddedLyrics.empty()) {
    parseLrcContent(QString::fromUtf8(embeddedLyrics.c_str()));
    return;
  }

  // No lyrics found
  m_hasLyrics = false;
  m_isLyricsSynchronized = false;
  m_currentLyrics = "";
  m_lyricsList.clear();
  m_currentLyricsLines.clear();
  m_activeLyricIndex = -1;
  emit lyricsChanged();
  emit activeLyricIndexChanged();
}

void QtBridge::parseLrcContent(const QString &content) {
  m_lyricsList.clear();
  m_currentLyricsLines.clear();
  m_currentLyrics = content;

  if (content.trimmed().isEmpty()) {
    m_hasLyrics = false;
    m_isLyricsSynchronized = false;
    m_activeLyricIndex = -1;
    emit lyricsChanged();
    emit activeLyricIndexChanged();
    return;
  }

  static const QRegularExpression timeRegex(
      R"(\[(\d{1,2}):(\d{2})(?:\.(\d{1,3}))?\])");

  QStringList rawLines = content.split('\n');
  bool hasAnyTimestamp = false;

  for (const QString &rawLine : rawLines) {
    QString trimmed = rawLine.trimmed();
    if (trimmed.isEmpty())
      continue;

    // Skip LRC metadata tags like [ar:Artist], [ti:Title], [al:Album]
    if (trimmed.startsWith("[ar:") || trimmed.startsWith("[ti:") ||
        trimmed.startsWith("[al:") || trimmed.startsWith("[by:") ||
        trimmed.startsWith("[length:") || trimmed.startsWith("[re:") ||
        trimmed.startsWith("[ve:") || trimmed.startsWith("[offset:")) {
      continue;
    }

    auto matchIterator = timeRegex.globalMatch(trimmed);
    if (matchIterator.hasNext()) {
      hasAnyTimestamp = true;
      int lastTimestampEnd = 0;
      QList<qint64> timestamps;
      while (matchIterator.hasNext()) {
        auto match = matchIterator.next();
        qint64 min = match.captured(1).toLongLong();
        qint64 sec = match.captured(2).toLongLong();
        qint64 ms = 0;
        if (!match.captured(3).isEmpty()) {
          QString msStr = match.captured(3);
          if (msStr.length() == 1) {
            ms = msStr.toLongLong() * 100;
          } else if (msStr.length() == 2) {
            ms = msStr.toLongLong() * 10;
          } else {
            ms = msStr.toLongLong();
          }
        }
        timestamps.append((min * 60 + sec) * 1000 + ms);
        lastTimestampEnd = match.capturedEnd();
      }
      QString text = trimmed.mid(lastTimestampEnd).trimmed();
      for (qint64 t : timestamps) {
        LyricLine line;
        line.timeMs = t;
        line.timeStr = formatTime(t);
        line.text = text;
        m_lyricsList.append(line);
      }
    } else {
      LyricLine line;
      line.timeMs = -1;
      line.timeStr = "";
      line.text = trimmed;
      m_lyricsList.append(line);
    }
  }

  if (hasAnyTimestamp) {
    std::sort(m_lyricsList.begin(), m_lyricsList.end(),
              [](const LyricLine &a, const LyricLine &b) {
                return a.timeMs < b.timeMs;
              });
    m_isLyricsSynchronized = true;
  } else {
    m_isLyricsSynchronized = false;
  }

  m_hasLyrics = !m_lyricsList.isEmpty();

  for (const auto &line : m_lyricsList) {
    QVariantMap map;
    map["timeMs"] = line.timeMs;
    map["timeStr"] = line.timeStr;
    map["text"] = line.text;
    m_currentLyricsLines.append(map);
  }

  updateActiveLyricIndex();
  emit lyricsChanged();
}

void QtBridge::updateActiveLyricIndex() {
  if (!m_isLyricsSynchronized || m_lyricsList.isEmpty()) {
    if (m_activeLyricIndex != -1) {
      m_activeLyricIndex = -1;
      emit activeLyricIndexChanged();
    }
    return;
  }

  int newIndex = -1;
  for (int i = 0; i < m_lyricsList.size(); ++i) {
    if (m_lyricsList[i].timeMs <= m_positionMs) {
      newIndex = i;
    } else {
      break;
    }
  }

  if (newIndex != m_activeLyricIndex) {
    m_activeLyricIndex = newIndex;
    emit activeLyricIndexChanged();
  }
}

void QtBridge::seekToLyric(int lineIndex) {
  if (lineIndex >= 0 && lineIndex < m_lyricsList.size()) {
    if (m_lyricsList[lineIndex].timeMs >= 0) {
      seek(m_lyricsList[lineIndex].timeMs);
    }
  }
}

void QtBridge::reloadLyrics() {
  if (!m_currentFilePath.isEmpty()) {
    loadLyricsForTrack(m_currentFilePath);
  }
}
