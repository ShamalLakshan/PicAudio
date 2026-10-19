#pragma once

#include <QAudioDevice>
#include <QMediaDevices>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <atomic>
#include <memory>
#include <thread>
#include <vector>

#include "core/services/LibraryService.hpp"
#include "core/services/PlayerService.hpp"
#include "core/services/QueueService.hpp"
#include "models/AlbumListModel.hpp"
#include "models/TrackListModel.hpp"
#include "theming/ThemeLoader.hpp"

/**
 * @brief Hotkey configuration entry mapping an action to key sequence.
 */
struct HotkeyEntry {
  QString id;
  QString category;
  QString actionName;
  QString defaultSequence;
  QString currentSequence;
};

/**
 * @brief Bridge exposing player services, library data, and theme tokens to
 * QML.
 */
class QtBridge : public QObject {
  Q_OBJECT

  // Models
  Q_PROPERTY(AlbumListModel *albumModel READ albumModel CONSTANT)
  Q_PROPERTY(TrackListModel *trackModel READ trackModel CONSTANT)
  Q_PROPERTY(
      TrackListModel *albumDetailTrackModel READ albumDetailTrackModel CONSTANT)
  Q_PROPERTY(TrackListModel *queueTrackModel READ queueTrackModel CONSTANT)
  Q_PROPERTY(TrackListModel *historyTrackModel READ historyTrackModel CONSTANT)

  // Theming
  Q_PROPERTY(adapters::ThemeLoader *theme READ theme CONSTANT)

  // Current Playing Track State
  Q_PROPERTY(
      QString currentTrackTitle READ currentTrackTitle NOTIFY playbackChanged)
  Q_PROPERTY(QString currentArtist READ currentArtist NOTIFY playbackChanged)
  Q_PROPERTY(QString currentAlbum READ currentAlbum NOTIFY playbackChanged)
  Q_PROPERTY(QString currentYear READ currentYear NOTIFY playbackChanged)
  Q_PROPERTY(QString currentGenre READ currentGenre NOTIFY playbackChanged)
  Q_PROPERTY(QString currentCodec READ currentCodec NOTIFY playbackChanged)
  Q_PROPERTY(
      QString currentAudioSpecs READ currentAudioSpecs NOTIFY playbackChanged)
  Q_PROPERTY(QString currentArtHash READ currentArtHash NOTIFY playbackChanged)
  Q_PROPERTY(QString currentArtUrl READ currentArtUrl NOTIFY playbackChanged)
  Q_PROPERTY(
      QString currentFilePath READ currentFilePath NOTIFY playbackChanged)
  Q_PROPERTY(
      QString currentFileSizeStr READ currentFileSizeStr NOTIFY playbackChanged)
  Q_PROPERTY(int currentBitrate READ currentBitrate NOTIFY playbackChanged)
  Q_PROPERTY(
      int currentSampleRate READ currentSampleRate NOTIFY playbackChanged)
  Q_PROPERTY(int currentBitDepth READ currentBitDepth NOTIFY playbackChanged)
  Q_PROPERTY(int currentChannels READ currentChannels NOTIFY playbackChanged)
  Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY playbackChanged)
  Q_PROPERTY(qint64 positionMs READ positionMs NOTIFY positionChanged)
  Q_PROPERTY(qint64 durationMs READ durationMs NOTIFY playbackChanged)
  Q_PROPERTY(QString positionStr READ positionStr NOTIFY positionChanged)
  Q_PROPERTY(QString durationStr READ durationStr NOTIFY playbackChanged)
  Q_PROPERTY(float volume READ volume WRITE setVolume NOTIFY volumeChanged)
  Q_PROPERTY(bool isMuted READ isMuted NOTIFY volumeChanged)
  Q_PROPERTY(bool volumeLocked READ volumeLocked WRITE setVolumeLocked NOTIFY
                 volumeLockedChanged)
  Q_PROPERTY(bool showRemainingTime READ showRemainingTime WRITE
                 setShowRemainingTime NOTIFY showRemainingTimeChanged)

  // Lyrics State
  Q_PROPERTY(QString currentLyrics READ currentLyrics NOTIFY lyricsChanged)
  Q_PROPERTY(bool hasLyrics READ hasLyrics NOTIFY lyricsChanged)
  Q_PROPERTY(
      bool isLyricsSynchronized READ isLyricsSynchronized NOTIFY lyricsChanged)
  Q_PROPERTY(QVariantList currentLyricsLines READ currentLyricsLines NOTIFY
                 lyricsChanged)
  Q_PROPERTY(
      int activeLyricIndex READ activeLyricIndex NOTIFY activeLyricIndexChanged)

  // Real-Time Audio Visualizer Data (Issue #66)
  Q_PROPERTY(QVariantList visualizerBands READ visualizerBands NOTIFY
                 visualizerDataChanged)
  Q_PROPERTY(QVariantList visualizerWaveform READ visualizerWaveform NOTIFY
                 visualizerDataChanged)
  Q_PROPERTY(
      float visualizerLevel READ visualizerLevel NOTIFY visualizerDataChanged)

  // Queue & Playback Modes
  Q_PROPERTY(int repeatMode READ repeatMode WRITE setRepeatMode NOTIFY
                 playbackModeChanged)
  Q_PROPERTY(int shuffleMode READ shuffleMode WRITE setShuffleMode NOTIFY
                 playbackModeChanged)
  Q_PROPERTY(
      int queueRemainingCount READ queueRemainingCount NOTIFY queueChanged)
  Q_PROPERTY(QString queueRemainingDurationStr READ queueRemainingDurationStr
                 NOTIFY queueChanged)
  Q_PROPERTY(int upNextCount READ upNextCount NOTIFY queueChanged)
  Q_PROPERTY(QStringList availablePlaylists READ availablePlaylists NOTIFY
                 playlistsChanged)
  Q_PROPERTY(
      QVariantMap tableColumns READ tableColumns NOTIFY tableColumnsChanged)
  Q_PROPERTY(QStringList tableColumnKeys READ tableColumnKeys NOTIFY
                 tableColumnsChanged)
  Q_PROPERTY(QStringList tableColumnOrder READ tableColumnOrder WRITE
                 setColumnOrder NOTIFY tableColumnsChanged)

  // View Layout & Panel Persistence (Issue #14)
  Q_PROPERTY(int leftPanelWidth READ leftPanelWidth WRITE setLeftPanelWidth
                 NOTIFY viewLayoutChanged)
  Q_PROPERTY(bool showLeftPanel READ showLeftPanel WRITE setShowLeftPanel NOTIFY
                 viewLayoutChanged)
  Q_PROPERTY(int rightPanelWidth READ rightPanelWidth WRITE setRightPanelWidth
                 NOTIFY viewLayoutChanged)
  Q_PROPERTY(bool showRightPanel READ showRightPanel WRITE setShowRightPanel
                 NOTIFY viewLayoutChanged)
  Q_PROPERTY(int columnBrowserHeight READ columnBrowserHeight WRITE
                 setColumnBrowserHeight NOTIFY viewLayoutChanged)
  Q_PROPERTY(bool showColumnBrowser READ showColumnBrowser WRITE
                 setShowColumnBrowser NOTIFY viewLayoutChanged)
  Q_PROPERTY(int activeWorkspaceViewMode READ activeWorkspaceViewMode WRITE
                 setActiveWorkspaceViewMode NOTIFY viewLayoutChanged)
  Q_PROPERTY(int activeNavSection READ activeNavSection WRITE
                 setActiveNavSection NOTIFY viewLayoutChanged)
  Q_PROPERTY(QString activePlaylistName READ activePlaylistName WRITE
                 setActivePlaylistName NOTIFY viewLayoutChanged)

  // Library Statistics
  Q_PROPERTY(int totalAlbums READ totalAlbums NOTIFY libraryStatsChanged)
  Q_PROPERTY(int totalTracks READ totalTracks NOTIFY libraryStatsChanged)
  Q_PROPERTY(
      QString totalDurationStr READ totalDurationStr NOTIFY libraryStatsChanged)
  Q_PROPERTY(QString totalLibrarySizeStr READ totalLibrarySizeStr NOTIFY
                 libraryStatsChanged)
  Q_PROPERTY(QStringList genresList READ genresList NOTIFY libraryStatsChanged)
  Q_PROPERTY(
      QStringList artistsList READ artistsList NOTIFY libraryStatsChanged)

  // Scanning State
  Q_PROPERTY(bool isScanning READ isScanning NOTIFY scanningChanged)
  Q_PROPERTY(QString scanStatusText READ scanStatusText NOTIFY scanningChanged)
  Q_PROPERTY(int scanScanned READ scanScanned NOTIFY scanningChanged)
  Q_PROPERTY(int scanTotal READ scanTotal NOTIFY scanningChanged)

  // Selected Album Details
  Q_PROPERTY(QString selectedAlbumTitle READ selectedAlbumTitle NOTIFY
                 selectedAlbumChanged)
  Q_PROPERTY(QString selectedAlbumArtist READ selectedAlbumArtist NOTIFY
                 selectedAlbumChanged)
  Q_PROPERTY(QString selectedAlbumYear READ selectedAlbumYear NOTIFY
                 selectedAlbumChanged)
  Q_PROPERTY(QString selectedAlbumGenre READ selectedAlbumGenre NOTIFY
                 selectedAlbumChanged)
  Q_PROPERTY(QString selectedAlbumQuality READ selectedAlbumQuality NOTIFY
                 selectedAlbumChanged)
  Q_PROPERTY(QString selectedAlbumDuration READ selectedAlbumDuration NOTIFY
                 selectedAlbumChanged)
  Q_PROPERTY(int selectedAlbumTrackCount READ selectedAlbumTrackCount NOTIFY
                 selectedAlbumChanged)
  Q_PROPERTY(QString selectedAlbumArtHash READ selectedAlbumArtHash NOTIFY
                 selectedAlbumChanged)

  // Settings (Playback & DSP)
  Q_PROPERTY(bool gaplessPlayback READ gaplessPlayback WRITE setGaplessPlayback
                 NOTIFY playbackSettingsChanged)
  Q_PROPERTY(bool crossfadeEnabled READ crossfadeEnabled WRITE
                 setCrossfadeEnabled NOTIFY playbackSettingsChanged)
  Q_PROPERTY(qreal crossfadeDurationSec READ crossfadeDurationSec WRITE
                 setCrossfadeDurationSec NOTIFY playbackSettingsChanged)
  Q_PROPERTY(QString crossfadeCurve READ crossfadeCurve WRITE setCrossfadeCurve
                 NOTIFY playbackSettingsChanged)
  Q_PROPERTY(QString replayGainMode READ replayGainMode WRITE setReplayGainMode
                 NOTIFY playbackSettingsChanged)
  Q_PROPERTY(int replayGainPreampDb READ replayGainPreampDb WRITE
                 setReplayGainPreampDb NOTIFY playbackSettingsChanged)
  Q_PROPERTY(
      int replayGainPreampWithoutGainDb READ replayGainPreampWithoutGainDb WRITE
          setReplayGainPreampWithoutGainDb NOTIFY playbackSettingsChanged)
  Q_PROPERTY(bool truePeakLimiter READ truePeakLimiter WRITE setTruePeakLimiter
                 NOTIFY playbackSettingsChanged)
  Q_PROPERTY(int shortSeekStepSec READ shortSeekStepSec WRITE
                 setShortSeekStepSec NOTIFY playbackSettingsChanged)
  Q_PROPERTY(int longSeekStepSec READ longSeekStepSec WRITE setLongSeekStepSec
                 NOTIFY playbackSettingsChanged)
  Q_PROPERTY(bool stopAfterCurrentTrack READ stopAfterCurrentTrack WRITE
                 setStopAfterCurrentTrack NOTIFY playbackSettingsChanged)
  Q_PROPERTY(qreal playbackRate READ playbackRate WRITE setPlaybackRate NOTIFY
                 playbackRateChanged)
  Q_PROPERTY(qint64 loopPointA READ loopPointA NOTIFY loopPointsChanged)
  Q_PROPERTY(qint64 loopPointB READ loopPointB NOTIFY loopPointsChanged)
  Q_PROPERTY(bool isLoopActive READ isLoopActive NOTIFY loopPointsChanged)
  Q_PROPERTY(
      bool isSleepTimerActive READ isSleepTimerActive NOTIFY sleepTimerChanged)
  Q_PROPERTY(int sleepTimerRemainingSec READ sleepTimerRemainingSec NOTIFY
                 sleepTimerChanged)
  Q_PROPERTY(QVariantMap audioPipelineDiagnostics READ audioPipelineDiagnostics
                 NOTIFY audioSettingsChanged)

  // Settings (Queue Ergonomics)
  Q_PROPERTY(QString doubleClickAction READ doubleClickAction WRITE
                 setDoubleClickAction NOTIFY queueSettingsChanged)
  Q_PROPERTY(QString middleClickAction READ middleClickAction WRITE
                 setMiddleClickAction NOTIFY queueSettingsChanged)
  Q_PROPERTY(QString queueAutoFillMode READ queueAutoFillMode WRITE
                 setQueueAutoFillMode NOTIFY queueSettingsChanged)
  Q_PROPERTY(int historyRetentionLimit READ historyRetentionLimit WRITE
                 setHistoryRetentionLimit NOTIFY queueSettingsChanged)

  // Settings (Library, Monitored Folders & Formats)
  Q_PROPERTY(QStringList monitoredFolders READ monitoredFolders NOTIFY
                 librarySettingsChanged)
  Q_PROPERTY(bool filesystemWatcher READ filesystemWatcher WRITE
                 setFilesystemWatcher NOTIFY librarySettingsChanged)
  Q_PROPERTY(bool autoScanOnStartup READ autoScanOnStartup WRITE
                 setAutoScanOnStartup NOTIFY librarySettingsChanged)
  Q_PROPERTY(QStringList formatFilters READ formatFilters WRITE setFormatFilters
                 NOTIFY librarySettingsChanged)
  Q_PROPERTY(QString excludeFolders READ excludeFolders WRITE setExcludeFolders
                 NOTIFY librarySettingsChanged)
  Q_PROPERTY(QString artworkPriority READ artworkPriority WRITE
                 setArtworkPriority NOTIFY librarySettingsChanged)
  Q_PROPERTY(bool groupAlbumsByTitle READ groupAlbumsByTitle WRITE
                 setGroupAlbumsByTitle NOTIFY librarySettingsChanged)

  // Settings (General)
  Q_PROPERTY(
      QString language READ language WRITE setLanguage NOTIFY settingsChanged)
  Q_PROPERTY(QString startupAction READ startupAction WRITE setStartupAction
                 NOTIFY settingsChanged)
  Q_PROPERTY(QString closeAction READ closeAction WRITE setCloseAction NOTIFY
                 settingsChanged)
  Q_PROPERTY(QString minimizeAction READ minimizeAction WRITE setMinimizeAction
                 NOTIFY settingsChanged)
  Q_PROPERTY(bool singleInstance READ singleInstance WRITE setSingleInstance
                 NOTIFY settingsChanged)
  Q_PROPERTY(QString updateCheckInterval READ updateCheckInterval WRITE
                 setUpdateCheckInterval NOTIFY settingsChanged)
  Q_PROPERTY(bool showNotifications READ showNotifications WRITE
                 setShowNotifications NOTIFY settingsChanged)
  Q_PROPERTY(int notificationDurationSec READ notificationDurationSec WRITE
                 setNotificationDurationSec NOTIFY settingsChanged)
  Q_PROPERTY(bool suppressNotificationsWhenFocused READ
                 suppressNotificationsWhenFocused WRITE
                     setSuppressNotificationsWhenFocused NOTIFY settingsChanged)

  // Settings (Appearance)
  Q_PROPERTY(QString fontFamily READ fontFamily WRITE setFontFamily NOTIFY
                 settingsChanged)
  Q_PROPERTY(int baseFontSize READ baseFontSize WRITE setBaseFontSize NOTIFY
                 settingsChanged)
  Q_PROPERTY(bool waveformSeekbar READ waveformSeekbar WRITE setWaveformSeekbar
                 NOTIFY settingsChanged)
  Q_PROPERTY(int tableRowHeight READ tableRowHeight WRITE setTableRowHeight
                 NOTIFY settingsChanged)
  Q_PROPERTY(bool tableAlternatingRows READ tableAlternatingRows WRITE
                 setTableAlternatingRows NOTIFY settingsChanged)
  Q_PROPERTY(QString artThumbnailQuality READ artThumbnailQuality WRITE
                 setArtThumbnailQuality NOTIFY settingsChanged)
  Q_PROPERTY(int artCacheLimitMb READ artCacheLimitMb WRITE setArtCacheLimitMb
                 NOTIFY settingsChanged)
  Q_PROPERTY(bool showStatusBar READ showStatusBar WRITE setShowStatusBar NOTIFY
                 settingsChanged)

  // Settings (Audio Output & Hardware Pipeline)
  Q_PROPERTY(QString audioBackend READ audioBackend WRITE setAudioBackend NOTIFY
                 audioSettingsChanged)
  Q_PROPERTY(QStringList availableAudioDevices READ availableAudioDevices NOTIFY
                 audioDevicesChanged)
  Q_PROPERTY(QString currentAudioDevice READ currentAudioDevice WRITE
                 setCurrentAudioDevice NOTIFY audioSettingsChanged)
  Q_PROPERTY(bool bitPerfectExclusive READ bitPerfectExclusive WRITE
                 setBitPerfectExclusive NOTIFY audioSettingsChanged)
  Q_PROPERTY(int bufferLatencyMs READ bufferLatencyMs WRITE setBufferLatencyMs
                 NOTIFY audioSettingsChanged)
  Q_PROPERTY(QString resamplerQuality READ resamplerQuality WRITE
                 setResamplerQuality NOTIFY audioSettingsChanged)
  Q_PROPERTY(QString ditherMode READ ditherMode WRITE setDitherMode NOTIFY
                 audioSettingsChanged)
  Q_PROPERTY(QString channelProcessing READ channelProcessing WRITE
                 setChannelProcessing NOTIFY audioSettingsChanged)

  // Settings (Hotkeys & Accelerators)
  Q_PROPERTY(bool globalMediaKeysEnabled READ globalMediaKeysEnabled WRITE
                 setGlobalMediaKeysEnabled NOTIFY hotkeysChanged)
  Q_PROPERTY(QVariantList hotkeysModel READ hotkeysModel NOTIFY hotkeysChanged)

  // Settings (Metadata & Scrobbling)
  Q_PROPERTY(bool lastfmEnabled READ lastfmEnabled WRITE setLastfmEnabled NOTIFY
                 metadataSettingsChanged)
  Q_PROPERTY(QString lastfmUsername READ lastfmUsername WRITE setLastfmUsername
                 NOTIFY metadataSettingsChanged)
  Q_PROPERTY(QString lastfmSessionKey READ lastfmSessionKey WRITE
                 setLastfmSessionKey NOTIFY metadataSettingsChanged)
  Q_PROPERTY(bool listenbrainzEnabled READ listenbrainzEnabled WRITE
                 setListenbrainzEnabled NOTIFY metadataSettingsChanged)
  Q_PROPERTY(QString listenbrainzToken READ listenbrainzToken WRITE
                 setListenbrainzToken NOTIFY metadataSettingsChanged)
  Q_PROPERTY(QString listenbrainzApiUrl READ listenbrainzApiUrl WRITE
                 setListenbrainzApiUrl NOTIFY metadataSettingsChanged)
  Q_PROPERTY(int scrobbleThresholdPercent READ scrobbleThresholdPercent WRITE
                 setScrobbleThresholdPercent NOTIFY metadataSettingsChanged)
  Q_PROPERTY(int scrobbleThresholdTimeSec READ scrobbleThresholdTimeSec WRITE
                 setScrobbleThresholdTimeSec NOTIFY metadataSettingsChanged)
  Q_PROPERTY(bool offlineScrobbleCache READ offlineScrobbleCache WRITE
                 setOfflineScrobbleCache NOTIFY metadataSettingsChanged)
  Q_PROPERTY(QString lyricsProviderOrder READ lyricsProviderOrder WRITE
                 setLyricsProviderOrder NOTIFY metadataSettingsChanged)
  Q_PROPERTY(bool autoFetchLyrics READ autoFetchLyrics WRITE setAutoFetchLyrics
                 NOTIFY metadataSettingsChanged)

  // Settings (Diagnostics & Advanced)
  Q_PROPERTY(QString loggingVerbosity READ loggingVerbosity WRITE
                 setLoggingVerbosity NOTIFY diagnosticsSettingsChanged)

  // Undo / Redo
  Q_PROPERTY(bool canUndo READ canUndo NOTIFY undoRedoChanged)
  Q_PROPERTY(bool canRedo READ canRedo NOTIFY undoRedoChanged)
  Q_PROPERTY(QString undoActionName READ undoActionName NOTIFY undoRedoChanged)
  Q_PROPERTY(QString redoActionName READ redoActionName NOTIFY undoRedoChanged)

public:
  explicit QtBridge(core::PlayerService &player, core::LibraryService &library,
                    adapters::ThemeLoader *themeLoader = nullptr,
                    QObject *parent = nullptr);
  ~QtBridge() override;

  [[nodiscard]] AlbumListModel *albumModel() const { return m_albumModel; }
  [[nodiscard]] TrackListModel *trackModel() const { return m_trackModel; }
  [[nodiscard]] TrackListModel *albumDetailTrackModel() const {
    return m_albumDetailTrackModel;
  }
  [[nodiscard]] TrackListModel *queueTrackModel() const {
    return m_queueTrackModel;
  }
  [[nodiscard]] TrackListModel *historyTrackModel() const {
    return m_historyTrackModel;
  }
  [[nodiscard]] bool canUndo() const { return !m_undoStack.empty(); }
  [[nodiscard]] bool canRedo() const { return !m_redoStack.empty(); }
  [[nodiscard]] QString undoActionName() const;
  [[nodiscard]] QString redoActionName() const;
  [[nodiscard]] adapters::ThemeLoader *theme() const { return m_themeLoader; }

  [[nodiscard]] QString currentTrackTitle() const {
    return m_currentTrackTitle;
  }
  [[nodiscard]] QString currentArtist() const { return m_currentArtist; }
  [[nodiscard]] QString currentAlbum() const { return m_currentAlbum; }
  [[nodiscard]] QString currentYear() const { return m_currentYear; }
  [[nodiscard]] QString currentGenre() const { return m_currentGenre; }
  [[nodiscard]] QString currentCodec() const { return m_currentCodec; }
  [[nodiscard]] QString currentAudioSpecs() const {
    return m_currentAudioSpecs;
  }
  [[nodiscard]] QString currentArtHash() const { return m_currentArtHash; }
  [[nodiscard]] QString currentArtUrl() const;
  [[nodiscard]] QString currentFilePath() const { return m_currentFilePath; }
  [[nodiscard]] QString currentFileSizeStr() const {
    return m_currentFileSizeStr;
  }
  [[nodiscard]] int currentBitrate() const { return m_currentBitrate; }
  [[nodiscard]] int currentSampleRate() const { return m_currentSampleRate; }
  [[nodiscard]] int currentBitDepth() const { return m_currentBitDepth; }
  [[nodiscard]] int currentChannels() const { return m_currentChannels; }
  [[nodiscard]] bool isPlaying() const { return m_isPlaying; }
  [[nodiscard]] qint64 positionMs() const { return m_positionMs; }
  [[nodiscard]] qint64 durationMs() const { return m_durationMs; }
  [[nodiscard]] QString positionStr() const { return m_positionStr; }
  [[nodiscard]] QString durationStr() const { return m_durationStr; }
  [[nodiscard]] float volume() const { return m_volume; }
  [[nodiscard]] bool isMuted() const { return m_isMuted; }
  [[nodiscard]] bool volumeLocked() const { return m_volumeLocked; }
  [[nodiscard]] bool showRemainingTime() const { return m_showRemainingTime; }
  [[nodiscard]] Q_INVOKABLE QString formatDuration(qint64 ms) const;

  [[nodiscard]] QString currentLyrics() const { return m_currentLyrics; }
  [[nodiscard]] bool hasLyrics() const { return m_hasLyrics; }
  [[nodiscard]] bool isLyricsSynchronized() const {
    return m_isLyricsSynchronized;
  }
  [[nodiscard]] QVariantList currentLyricsLines() const {
    return m_currentLyricsLines;
  }
  [[nodiscard]] int activeLyricIndex() const { return m_activeLyricIndex; }

  [[nodiscard]] QVariantList visualizerBands() const {
    return m_visualizerBands;
  }
  [[nodiscard]] QVariantList visualizerWaveform() const {
    return m_visualizerWaveform;
  }
  [[nodiscard]] float visualizerLevel() const { return m_visualizerLevel; }

  [[nodiscard]] int repeatMode() const;
  [[nodiscard]] int shuffleMode() const;
  [[nodiscard]] int queueRemainingCount() const;
  [[nodiscard]] QString queueRemainingDurationStr() const;
  [[nodiscard]] int upNextCount() const;
  [[nodiscard]] QStringList availablePlaylists() const;

  [[nodiscard]] int totalAlbums() const;
  [[nodiscard]] int totalTracks() const;
  [[nodiscard]] QString totalDurationStr() const;
  [[nodiscard]] QString totalLibrarySizeStr() const;
  [[nodiscard]] QStringList genresList() const { return m_genresList; }
  [[nodiscard]] QStringList artistsList() const { return m_artistsList; }

  [[nodiscard]] bool isScanning() const { return m_isScanning; }
  [[nodiscard]] QString scanStatusText() const { return m_scanStatusText; }
  [[nodiscard]] int scanScanned() const { return m_scanScanned; }
  [[nodiscard]] int scanTotal() const { return m_scanTotal; }

  [[nodiscard]] QString selectedAlbumTitle() const {
    return m_selectedAlbumTitle;
  }
  [[nodiscard]] QString selectedAlbumArtist() const {
    return m_selectedAlbumArtist;
  }
  [[nodiscard]] QString selectedAlbumYear() const {
    return m_selectedAlbumYear;
  }
  [[nodiscard]] QString selectedAlbumGenre() const {
    return m_selectedAlbumGenre;
  }
  [[nodiscard]] QString selectedAlbumQuality() const {
    return m_selectedAlbumQuality;
  }
  [[nodiscard]] QString selectedAlbumDuration() const {
    return m_selectedAlbumDuration;
  }
  [[nodiscard]] int selectedAlbumTrackCount() const {
    return m_selectedAlbumTrackCount;
  }
  [[nodiscard]] QString selectedAlbumArtHash() const {
    return m_selectedAlbumArtHash;
  }

  // Playback & DSP Settings Getters
  [[nodiscard]] bool gaplessPlayback() const { return m_gaplessPlayback; }
  [[nodiscard]] bool crossfadeEnabled() const { return m_crossfadeEnabled; }
  [[nodiscard]] qreal crossfadeDurationSec() const {
    return m_crossfadeDurationSec;
  }
  [[nodiscard]] QString crossfadeCurve() const { return m_crossfadeCurve; }
  [[nodiscard]] QString replayGainMode() const { return m_replayGainMode; }
  [[nodiscard]] int replayGainPreampDb() const { return m_replayGainPreampDb; }
  [[nodiscard]] int replayGainPreampWithoutGainDb() const {
    return m_replayGainPreampWithoutGainDb;
  }
  [[nodiscard]] bool truePeakLimiter() const { return m_truePeakLimiter; }
  [[nodiscard]] int shortSeekStepSec() const { return m_shortSeekStepSec; }
  [[nodiscard]] int longSeekStepSec() const { return m_longSeekStepSec; }
  [[nodiscard]] bool stopAfterCurrentTrack() const {
    return m_stopAfterCurrentTrack;
  }

  // Queue Ergonomics Settings Getters
  [[nodiscard]] QString doubleClickAction() const {
    return m_doubleClickAction;
  }
  [[nodiscard]] QString middleClickAction() const {
    return m_middleClickAction;
  }
  [[nodiscard]] QString queueAutoFillMode() const {
    return m_queueAutoFillMode;
  }
  [[nodiscard]] int historyRetentionLimit() const {
    return m_historyRetentionLimit;
  }

  // Library & Formats Settings Getters
  [[nodiscard]] QStringList monitoredFolders() const {
    return m_monitoredFolders;
  }
  [[nodiscard]] bool filesystemWatcher() const { return m_filesystemWatcher; }
  [[nodiscard]] bool autoScanOnStartup() const { return m_autoScanOnStartup; }
  [[nodiscard]] QStringList formatFilters() const { return m_formatFilters; }
  [[nodiscard]] QString excludeFolders() const { return m_excludeFolders; }
  [[nodiscard]] QString artworkPriority() const { return m_artworkPriority; }
  [[nodiscard]] bool groupAlbumsByTitle() const { return m_groupAlbumsByTitle; }

  // General Settings Getters
  [[nodiscard]] QString language() const { return m_language; }
  [[nodiscard]] QString startupAction() const { return m_startupAction; }
  [[nodiscard]] QString closeAction() const { return m_closeAction; }
  [[nodiscard]] QString minimizeAction() const { return m_minimizeAction; }
  [[nodiscard]] bool singleInstance() const { return m_singleInstance; }
  [[nodiscard]] QString updateCheckInterval() const {
    return m_updateCheckInterval;
  }
  [[nodiscard]] bool showNotifications() const { return m_showNotifications; }
  [[nodiscard]] int notificationDurationSec() const {
    return m_notificationDurationSec;
  }
  [[nodiscard]] bool suppressNotificationsWhenFocused() const {
    return m_suppressNotificationsWhenFocused;
  }

  // Appearance Settings Getters
  [[nodiscard]] QString fontFamily() const { return m_fontFamily; }
  [[nodiscard]] int baseFontSize() const { return m_baseFontSize; }
  [[nodiscard]] bool waveformSeekbar() const { return m_waveformSeekbar; }
  [[nodiscard]] int tableRowHeight() const { return m_tableRowHeight; }
  [[nodiscard]] bool tableAlternatingRows() const {
    return m_tableAlternatingRows;
  }
  [[nodiscard]] QString artThumbnailQuality() const {
    return m_artThumbnailQuality;
  }
  [[nodiscard]] int artCacheLimitMb() const { return m_artCacheLimitMb; }
  [[nodiscard]] bool showStatusBar() const { return m_showStatusBar; }

  // Audio Output Settings Getters
  [[nodiscard]] QString audioBackend() const { return m_audioBackend; }
  [[nodiscard]] QStringList availableAudioDevices() const {
    return m_availableAudioDevices;
  }
  [[nodiscard]] QString currentAudioDevice() const {
    return m_currentAudioDevice;
  }
  [[nodiscard]] bool bitPerfectExclusive() const {
    return m_bitPerfectExclusive;
  }
  [[nodiscard]] int bufferLatencyMs() const { return m_bufferLatencyMs; }
  [[nodiscard]] QString resamplerQuality() const { return m_resamplerQuality; }
  [[nodiscard]] QString ditherMode() const { return m_ditherMode; }
  [[nodiscard]] QString channelProcessing() const {
    return m_channelProcessing;
  }

  // Hotkeys Settings Getters
  [[nodiscard]] bool globalMediaKeysEnabled() const {
    return m_globalMediaKeysEnabled;
  }
  [[nodiscard]] QVariantList hotkeysModel() const;

  // Metadata & Scrobbling Settings Getters
  [[nodiscard]] bool lastfmEnabled() const { return m_lastfmEnabled; }
  [[nodiscard]] QString lastfmUsername() const { return m_lastfmUsername; }
  [[nodiscard]] QString lastfmSessionKey() const { return m_lastfmSessionKey; }
  [[nodiscard]] bool listenbrainzEnabled() const {
    return m_listenbrainzEnabled;
  }
  [[nodiscard]] QString listenbrainzToken() const {
    return m_listenbrainzToken;
  }
  [[nodiscard]] QString listenbrainzApiUrl() const {
    return m_listenbrainzApiUrl;
  }
  [[nodiscard]] int scrobbleThresholdPercent() const {
    return m_scrobbleThresholdPercent;
  }
  [[nodiscard]] int scrobbleThresholdTimeSec() const {
    return m_scrobbleThresholdTimeSec;
  }
  [[nodiscard]] bool offlineScrobbleCache() const {
    return m_offlineScrobbleCache;
  }
  [[nodiscard]] QString lyricsProviderOrder() const {
    return m_lyricsProviderOrder;
  }
  [[nodiscard]] bool autoFetchLyrics() const { return m_autoFetchLyrics; }

  // Diagnostics Settings Getters
  [[nodiscard]] QString loggingVerbosity() const { return m_loggingVerbosity; }

  // QML-invokable actions
  Q_INVOKABLE void scanDirectory(const QString &folderPath);
  Q_INVOKABLE void purgeMissingTracks();
  Q_INVOKABLE void clearLibrary();
  Q_INVOKABLE void search(const QString &query);
  Q_INVOKABLE void filterByLetter(const QString &letter);
  Q_INVOKABLE void filterByGenre(const QString &genre);
  Q_INVOKABLE void filterByArtist(const QString &artist);
  Q_INVOKABLE void resetFilters();
  Q_INVOKABLE void openAlbumDetails(const QString &albumTitle,
                                    const QString &albumArtist);
  Q_INVOKABLE void playAlbum(const QString &albumTitle,
                             const QString &albumArtist);
  Q_INVOKABLE void playTrack(const QString &trackId);
  Q_INVOKABLE void playTrackAtIndex(int index);
  Q_INVOKABLE void playTrackFromDetail(int index);
  Q_INVOKABLE void playQueueTrack(int index);
  Q_INVOKABLE void queueTrack(const QString &trackId);

  // Queue operations
  Q_INVOKABLE void playNow(const QString &trackId);
  Q_INVOKABLE void playSelectionNow(const QStringList &selectedIds);
  Q_INVOKABLE void playNext(const QString &trackId);
  Q_INVOKABLE void playTracksNext(const QStringList &trackIds);
  Q_INVOKABLE void queueLast(const QString &trackId);
  Q_INVOKABLE void queueTracksLast(const QStringList &trackIds);
  Q_INVOKABLE void playNextInAlbumShuffle(const QStringList &trackIds);
  Q_INVOKABLE void playAlbumNow(const QString &albumTitle,
                                const QString &albumArtist);
  Q_INVOKABLE void playAlbumNext(const QString &albumTitle,
                                 const QString &albumArtist);
  Q_INVOKABLE void queueAlbumLast(const QString &albumTitle,
                                  const QString &albumArtist);
  /**
   * @brief Adds all tracks belonging to an album into a target playlist.
   * @param albumTitle Title of the album.
   * @param albumArtist Primary artist of the album.
   * @param playlistName Name of the destination playlist.
   * @return True if tracks were appended successfully.
   */
  Q_INVOKABLE bool addAlbumToPlaylist(const QString &albumTitle,
                                      const QString &albumArtist,
                                      const QString &playlistName);

  /**
   * @brief Opens the filesystem directory containing the album tracks.
   * @param albumTitle Title of the album.
   * @param albumArtist Primary artist of the album.
   */
  Q_INVOKABLE void showAlbumInFileManager(const QString &albumTitle,
                                          const QString &albumArtist);

  /**
   * @brief Retrieves all track entities for an album formatted for QML.
   * @param albumTitle Title of the album.
   * @param albumArtist Primary artist of the album.
   * @return List of track metadata maps.
   */
  [[nodiscard]] Q_INVOKABLE QVariantList
  getAlbumTracks(const QString &albumTitle, const QString &albumArtist) const;

  /**
   * @brief Initiates online artwork search for the specified album.
   * @param albumTitle Title of the album.
   * @param albumArtist Primary artist of the album.
   */
  Q_INVOKABLE void searchAlbumArtworkOnline(const QString &albumTitle,
                                            const QString &albumArtist);

  /**
   * @brief Exports the cover artwork of an album to a target file path.
   * @param albumTitle Title of the album.
   * @param albumArtist Primary artist of the album.
   * @param targetFilePath Destination image path on disk.
   * @return True if artwork was successfully extracted and written.
   */
  Q_INVOKABLE bool exportAlbumArtwork(const QString &albumTitle,
                                      const QString &albumArtist,
                                      const QString &targetFilePath);

  Q_INVOKABLE void removeFromQueue(int index);
  Q_INVOKABLE void moveQueueItem(int fromIndex, int toIndex);
  Q_INVOKABLE void clearQueue();
  Q_INVOKABLE void shuffleRemainingQueue();

  /**
   * @brief Exports the current playback queue into a new static playlist.
   * @param playlistName Name of the new playlist to create.
   * @return True if saved successfully.
   */
  Q_INVOKABLE bool saveQueueAsPlaylist(const QString &playlistName);

  // Mode toggles
  Q_INVOKABLE void setRepeatMode(int mode);
  Q_INVOKABLE void cycleRepeatMode();
  Q_INVOKABLE void setShuffleMode(int mode);
  Q_INVOKABLE void cycleShuffleMode();

  // Transport controls
  Q_INVOKABLE void togglePlayPause();
  Q_INVOKABLE void stop();
  Q_INVOKABLE void nextTrack();
  Q_INVOKABLE void previousTrack();
  Q_INVOKABLE void seek(qint64 posMs);
  Q_INVOKABLE void setVolume(float volume);
  Q_INVOKABLE void toggleMute();
  Q_INVOKABLE void playAll();
  Q_INVOKABLE void shuffleAll();

  // Utilities
  Q_INVOKABLE void showInFileManager(const QString &filePath);
  Q_INVOKABLE void copyToClipboard(const QString &text);
  Q_INVOKABLE void seekToLyric(int lineIndex);
  Q_INVOKABLE void reloadLyrics();

  // Desktop Menu & File Operations
  Q_INVOKABLE void openAudioFile(const QString &filePath);
  Q_INVOKABLE void openFolder(const QString &folderPath, bool enqueue = false);
  Q_INVOKABLE void openCueSheet(const QString &cueFilePath);
  Q_INVOKABLE void openNetworkStream(const QString &streamUrl,
                                     const QString &streamName = QString());
  Q_INVOKABLE bool exportActiveView(const QString &filePath,
                                    const QString &format = QString());

  // Drag and Drop Operations
  Q_INVOKABLE void handleDropOnTracks(const QList<QUrl> &urls,
                                      bool playImmediately = true);
  Q_INVOKABLE void handleDropOnQueue(const QList<QUrl> &urls);
  Q_INVOKABLE void
  handleDropOnPlaylist(const QList<QUrl> &urls,
                       const QString &targetPlaylist = QString());
  Q_INVOKABLE void handleDropOnExplorer(const QList<QUrl> &urls);
  [[nodiscard]] static QStringList
  extractAudioFilesFromUrls(const QList<QUrl> &urls);

  // Edit & Undo Operations
  Q_INVOKABLE void undo();
  Q_INVOKABLE void redo();
  void pushUndoCommand(const QString &name, std::function<void()> undoFn,
                       std::function<void()> redoFn);
  Q_INVOKABLE bool updateTrackTags(const QString &trackId,
                                   const QVariantMap &tags);
  Q_INVOKABLE bool updateMultipleTrackTags(const QStringList &trackIds,
                                           const QVariantMap &tags);
  Q_INVOKABLE void deleteSelectedTracks(const QStringList &trackIds);
  Q_INVOKABLE bool deleteFilesFromDisk(const QStringList &trackIds);
  Q_INVOKABLE void setTracksRating(const QStringList &trackIds, int rating);
  [[nodiscard]] Q_INVOKABLE int getTrackRating(const QString &trackId) const;
  Q_INVOKABLE void toggleTracksFavorite(const QStringList &trackIds);
  [[nodiscard]] Q_INVOKABLE bool isTrackFavorite(const QString &trackId) const;
  Q_INVOKABLE void refreshHistory();

  // Playback & DSP Settings
  Q_INVOKABLE void setGaplessPlayback(bool enable);
  Q_INVOKABLE void setCrossfadeEnabled(bool enable);
  Q_INVOKABLE void setCrossfadeDurationSec(qreal sec);
  Q_INVOKABLE void setCrossfadeCurve(const QString &curve);
  Q_INVOKABLE void setReplayGainMode(const QString &mode);
  Q_INVOKABLE void setReplayGainPreampDb(int db);
  Q_INVOKABLE void setReplayGainPreampWithoutGainDb(int db);
  Q_INVOKABLE void setTruePeakLimiter(bool enable);
  Q_INVOKABLE void setShortSeekStepSec(int sec);
  Q_INVOKABLE void setLongSeekStepSec(int sec);
  Q_INVOKABLE void setStopAfterCurrentTrack(bool enable);
  Q_INVOKABLE void toggleStopAfterCurrentTrack() {
    setStopAfterCurrentTrack(!m_stopAfterCurrentTrack);
  }
  Q_INVOKABLE void setPlaybackRate(qreal rate);
  [[nodiscard]] qreal playbackRate() const { return m_playbackRate; }
  Q_INVOKABLE void setLoopPointA();
  Q_INVOKABLE void setLoopPointB();
  Q_INVOKABLE void clearLoop();
  [[nodiscard]] qint64 loopPointA() const { return m_loopPointA; }
  [[nodiscard]] qint64 loopPointB() const { return m_loopPointB; }
  [[nodiscard]] bool isLoopActive() const {
    return m_loopPointA >= 0 && m_loopPointB > m_loopPointA;
  }
  Q_INVOKABLE void startSleepTimer(int minutes);
  Q_INVOKABLE void cancelSleepTimer();
  [[nodiscard]] bool isSleepTimerActive() const { return m_sleepTimerActive; }
  [[nodiscard]] int sleepTimerRemainingSec() const {
    return m_sleepTimerRemainingSec;
  }

  // Queue Settings
  Q_INVOKABLE void setDoubleClickAction(const QString &action);
  Q_INVOKABLE void setMiddleClickAction(const QString &action);
  Q_INVOKABLE void setQueueAutoFillMode(const QString &mode);
  Q_INVOKABLE void setHistoryRetentionLimit(int limit);
  Q_INVOKABLE void clearPlaybackHistory();

  // Library & Folders Settings
  Q_INVOKABLE void addMonitoredFolder(const QString &folderPath);
  Q_INVOKABLE void removeMonitoredFolder(int index);
  Q_INVOKABLE void removeMonitoredFolderPath(const QString &folderPath);
  Q_INVOKABLE void rescanMonitoredFolder(const QString &folderPath);
  Q_INVOKABLE void rescanAllMonitoredFolders();
  Q_INVOKABLE void incrementalQuickScan();
  Q_INVOKABLE void setFilesystemWatcher(bool enable);
  Q_INVOKABLE void setAutoScanOnStartup(bool enable);
  Q_INVOKABLE void setFormatFilters(const QStringList &formats);
  Q_INVOKABLE void setFormatFilterEnabled(const QString &format, bool enabled);
  [[nodiscard]] Q_INVOKABLE bool
  isFormatFilterEnabled(const QString &format) const;
  Q_INVOKABLE void setExcludeFolders(const QString &patterns);
  Q_INVOKABLE void setArtworkPriority(const QString &priority);
  Q_INVOKABLE void setGroupAlbumsByTitle(bool enabled);
  Q_INVOKABLE void exportDatabaseBackup(const QString &targetFilePath);
  Q_INVOKABLE void optimizeDatabase();
  Q_INVOKABLE bool createPlaylist(const QString &name);
  Q_INVOKABLE bool createSmartPlaylist(const QString &name,
                                       const QString &rulesJson);
  Q_INVOKABLE bool importPlaylist(const QString &filePath);
  Q_INVOKABLE bool addTracksToPlaylist(const QString &playlistName,
                                       const QVariantList &tracks);
  Q_INVOKABLE bool removeTracksFromPlaylist(const QString &playlistName,
                                            const QVariantList &tracks);
  Q_INVOKABLE void openPlaylist(const QString &playlistName);
  Q_INVOKABLE void playPlaylist(const QString &playlistName);
  Q_INVOKABLE void playPlaylistNext(const QString &playlistName);
  Q_INVOKABLE void queuePlaylistLast(const QString &playlistName);
  Q_INVOKABLE bool renamePlaylist(const QString &oldName,
                                  const QString &newName);
  Q_INVOKABLE bool duplicatePlaylist(const QString &playlistName);
  Q_INVOKABLE bool exportPlaylistFile(const QString &playlistName,
                                      const QString &targetFilePath);
  Q_INVOKABLE bool deletePlaylist(const QString &playlistName);
  [[nodiscard]] Q_INVOKABLE bool
  isSmartPlaylist(const QString &playlistName) const;
  [[nodiscard]] Q_INVOKABLE QString
  getSmartPlaylistRules(const QString &playlistName) const;
  Q_INVOKABLE bool updateSmartPlaylistRules(const QString &playlistName,
                                            const QString &rulesJson);
  Q_INVOKABLE int deduplicateTracks();
  Q_INVOKABLE bool clearCoverArtCache() {
    clearArtCache();
    return true;
  }
  Q_INVOKABLE QString getLogFilePath() const;
  [[nodiscard]] QVariantMap audioPipelineDiagnostics() const;

  // Table Columns Management (Issue #11)
  [[nodiscard]] QVariantMap tableColumns() const;
  [[nodiscard]] QStringList tableColumnKeys() const;
  [[nodiscard]] Q_INVOKABLE bool isColumnVisible(const QString &colKey) const;
  [[nodiscard]] Q_INVOKABLE int getColumnWidth(const QString &colKey) const;
  Q_INVOKABLE void setColumnVisible(const QString &colKey, bool visible);
  Q_INVOKABLE void toggleColumnVisible(const QString &colKey);
  Q_INVOKABLE void setColumnWidth(const QString &colKey, int width);
  Q_INVOKABLE void autoSizeColumn(const QString &colKey);
  Q_INVOKABLE void autoSizeAllColumns();
  Q_INVOKABLE void resetTableColumns();
  Q_INVOKABLE void moveColumn(int fromIndex, int toIndex);
  Q_INVOKABLE void setColumnOrder(const QStringList &orderKeys);
  [[nodiscard]] Q_INVOKABLE QStringList tableColumnOrder() const;

  // Window Geometry & Layout (Issue #14)
  Q_INVOKABLE void saveWindowGeometry(int x, int y, int width, int height,
                                      bool isMaximized);
  [[nodiscard]] Q_INVOKABLE QVariantMap
  loadWindowGeometry(int defaultWidth = 1400, int defaultHeight = 840);

  [[nodiscard]] int leftPanelWidth() const { return m_leftPanelWidth; }
  Q_INVOKABLE void setLeftPanelWidth(int width);
  [[nodiscard]] bool showLeftPanel() const { return m_showLeftPanel; }
  Q_INVOKABLE void setShowLeftPanel(bool show);
  [[nodiscard]] int rightPanelWidth() const { return m_rightPanelWidth; }
  Q_INVOKABLE void setRightPanelWidth(int width);
  [[nodiscard]] bool showRightPanel() const { return m_showRightPanel; }
  Q_INVOKABLE void setShowRightPanel(bool show);
  [[nodiscard]] int columnBrowserHeight() const {
    return m_columnBrowserHeight;
  }
  Q_INVOKABLE void setColumnBrowserHeight(int height);
  [[nodiscard]] bool showColumnBrowser() const { return m_showColumnBrowser; }
  Q_INVOKABLE void setShowColumnBrowser(bool show);
  [[nodiscard]] int activeWorkspaceViewMode() const {
    return m_activeWorkspaceViewMode;
  }
  Q_INVOKABLE void setActiveWorkspaceViewMode(int mode);
  [[nodiscard]] int activeNavSection() const { return m_activeNavSection; }
  Q_INVOKABLE void setActiveNavSection(int nav);
  [[nodiscard]] QString activePlaylistName() const {
    return m_activePlaylistName;
  }
  Q_INVOKABLE void setActivePlaylistName(const QString &name);

  // Playback Session Restore (Issue #15)
  Q_INVOKABLE void savePlaybackSession();
  Q_INVOKABLE void restorePlaybackSession();

  // General Settings
  Q_INVOKABLE void setLanguage(const QString &lang);
  Q_INVOKABLE void setStartupAction(const QString &action);
  Q_INVOKABLE void setCloseAction(const QString &action);
  Q_INVOKABLE void setMinimizeAction(const QString &action);
  Q_INVOKABLE void setSingleInstance(bool enable);
  Q_INVOKABLE void setUpdateCheckInterval(const QString &interval);
  Q_INVOKABLE void setShowNotifications(bool enable);
  Q_INVOKABLE void setNotificationDurationSec(int sec);
  Q_INVOKABLE void setSuppressNotificationsWhenFocused(bool enable);

  // Appearance Settings
  Q_INVOKABLE void setFontFamily(const QString &font);
  Q_INVOKABLE void setBaseFontSize(int size);
  Q_INVOKABLE void setWaveformSeekbar(bool enable);
  Q_INVOKABLE void setTableRowHeight(int height);
  Q_INVOKABLE void setTableAlternatingRows(bool enable);
  Q_INVOKABLE void setArtThumbnailQuality(const QString &quality);
  Q_INVOKABLE void setArtCacheLimitMb(int limitMb);
  Q_INVOKABLE void setShowStatusBar(bool enable);

  // Audio Output Settings
  Q_INVOKABLE void setAudioBackend(const QString &backend);
  Q_INVOKABLE void setCurrentAudioDevice(const QString &device);
  Q_INVOKABLE void setBitPerfectExclusive(bool exclusive);
  Q_INVOKABLE void setBufferLatencyMs(int ms);
  Q_INVOKABLE void setResamplerQuality(const QString &quality);
  Q_INVOKABLE void setDitherMode(const QString &mode);
  Q_INVOKABLE void setChannelProcessing(const QString &mode);

  // Hotkeys & Accelerators Settings
  /**
   * @brief Sets whether global media keys hook is enabled.
   * @param enable True to capture media keys globally.
   */
  Q_INVOKABLE void setGlobalMediaKeysEnabled(bool enable);

  /**
   * @brief Rebinds a shortcut key sequence for a specific action.
   * @param actionId Unique action identifier.
   * @param keySequence New key sequence string.
   */
  Q_INVOKABLE void setHotkey(const QString &actionId,
                             const QString &keySequence);

  /**
   * @brief Restores all hotkeys and keyboard accelerators to default sequences.
   */
  Q_INVOKABLE void restoreDefaultHotkeys();

  /**
   * @brief Checks if a proposed key sequence conflicts with an existing hotkey.
   * @param actionId Action being edited (ignored during check).
   * @param keySequence Proposed shortcut key sequence.
   * @return Conflicting action name, or empty string if no conflict.
   */
  [[nodiscard]] Q_INVOKABLE QString checkHotkeyConflict(
      const QString &actionId, const QString &keySequence) const;

  /**
   * @brief Retrieves active key sequence bound to an action ID.
   * @param actionId Action identifier.
   * @return Key sequence string, or empty string.
   */
  [[nodiscard]] Q_INVOKABLE QString
  getHotkeySequence(const QString &actionId) const;

  /**
   * @brief Exports current hotkey configuration to a JSON keymap file.
   * @param filePath Target JSON file path.
   * @return True if exported successfully.
   */
  [[nodiscard]] Q_INVOKABLE bool exportHotkeys(const QString &filePath) const;

  /**
   * @brief Imports hotkey configuration from a JSON keymap file.
   * @param filePath Source JSON file path.
   * @return True if imported successfully.
   */
  Q_INVOKABLE bool importHotkeys(const QString &filePath);

  /**
   * @brief Dispatches an in-app non-blocking toast notification banner.
   * @param message Banner text to display.
   * @param type Notification variant ("info", "success", "warning", "error").
   */
  Q_INVOKABLE void showToast(const QString &message,
                             const QString &type = QStringLiteral("info"));

  // Internationalization & Locale
  [[nodiscard]] Q_INVOKABLE QStringList availableLanguages() const;
  void setQmlEngine(class QQmlApplicationEngine *engine);

  // Metadata & Scrobbling Settings
  /**
   * @brief Sets whether Last.fm scrobbling integration is enabled.
   * @param enable True to enable Last.fm.
   */
  Q_INVOKABLE void setLastfmEnabled(bool enable);

  /**
   * @brief Sets the Last.fm username.
   * @param user Last.fm account username.
   */
  Q_INVOKABLE void setLastfmUsername(const QString &user);

  /**
   * @brief Sets the Last.fm session key for authenticated scrobbling.
   * @param key Authenticated session key.
   */
  Q_INVOKABLE void setLastfmSessionKey(const QString &key);

  /**
   * @brief Sets whether ListenBrainz scrobbling integration is enabled.
   * @param enable True to enable ListenBrainz.
   */
  Q_INVOKABLE void setListenbrainzEnabled(bool enable);

  /**
   * @brief Sets the user API token for ListenBrainz.
   * @param token User API token.
   */
  Q_INVOKABLE void setListenbrainzToken(const QString &token);

  /**
   * @brief Sets the custom API URL for ListenBrainz instance.
   * @param url ListenBrainz API root URL.
   */
  Q_INVOKABLE void setListenbrainzApiUrl(const QString &url);

  /**
   * @brief Sets the scrobble threshold percentage (50% - 100%).
   * @param percent Track playback percentage before scrobble is submitted.
   */
  Q_INVOKABLE void setScrobbleThresholdPercent(int percent);

  /**
   * @brief Sets the scrobble threshold duration in seconds (30s - 300s).
   * @param sec Maximum playback duration in seconds required before scrobble.
   */
  Q_INVOKABLE void setScrobbleThresholdTimeSec(int sec);

  /**
   * @brief Sets whether offline scrobbles are cached and synced upon reconnect.
   * @param enable True to cache scrobbles offline.
   */
  Q_INVOKABLE void setOfflineScrobbleCache(bool enable);

  /**
   * @brief Sets the preferred priority order for synchronized lyrics lookup.
   * @param order Priority mode name.
   */
  Q_INVOKABLE void setLyricsProviderOrder(const QString &order);

  /**
   * @brief Sets whether missing lyrics are fetched automatically online.
   * @param enable True to auto-fetch missing lyrics.
   */
  Q_INVOKABLE void setAutoFetchLyrics(bool enable);

  // Diagnostics & Advanced Settings
  /**
   * @brief Sets the diagnostics logging verbosity level.
   * @param level Verbosity string ("Trace", "Debug", "Info", "Warning",
   * "Error").
   */
  Q_INVOKABLE void setLoggingVerbosity(const QString &level);

  /**
   * @brief Opens the system file manager to the application logs directory.
   */
  Q_INVOKABLE void openLogDirectory();

  /**
   * @brief Exports a comprehensive diagnostic report to the specified file
   * path.
   * @param targetFilePath Destination file path or file URL.
   */
  Q_INVOKABLE void exportDiagnosticsReport(const QString &targetFilePath);

  /**
   * @brief Emits a request to open the real-time audio stream diagnostics view.
   */
  Q_INVOKABLE void openAudioDiagnostics();

  /**
   * @brief Runs an SQLite database integrity check and returns verification
   * status.
   * @return Status message describing database health.
   */
  Q_INVOKABLE QString checkDatabaseIntegrity();

  /**
   * @brief Resets all user settings and preferences to factory default values.
   */
  Q_INVOKABLE void resetAllSettingsToDefaults();

  Q_INVOKABLE void setShowRemainingTime(bool enable);
  Q_INVOKABLE void setVolumeLocked(bool locked);

  // Maintenance Utilities
  Q_INVOKABLE void clearArtCache();
  Q_INVOKABLE void checkForUpdates();
  Q_INVOKABLE void refreshAudioDevices();

signals:
  void playbackChanged();
  void positionChanged();
  void volumeChanged();
  void libraryStatsChanged();
  void scanningChanged();
  void selectedAlbumChanged();
  void scanFinished(int count);
  void playbackModeChanged();
  void queueChanged();
  void settingsChanged();
  void playbackSettingsChanged();
  void queueSettingsChanged();
  void librarySettingsChanged();
  void audioSettingsChanged();
  void audioDevicesChanged();
  void hotkeysChanged();
  void metadataSettingsChanged();
  void diagnosticsSettingsChanged();
  void audioDiagnosticsRequested();
  void undoRedoChanged();
  void playbackRateChanged();
  void loopPointsChanged();
  void sleepTimerChanged();
  void playlistsChanged();
  void ratingsChanged();
  void tableColumnsChanged();
  void viewLayoutChanged();
  void volumeLockedChanged();
  void showRemainingTimeChanged();
  void lyricsChanged();
  void activeLyricIndexChanged();
  void visualizerDataChanged();
  void toastRequested(const QString &message, const QString &type);

private:
  struct LyricLine {
    qint64 timeMs{-1};
    QString timeStr;
    QString text;
  };

  void setupPositionTimer();
  void setupVisualizerTimer();
  void updateVisualizerData();
  void updatePlaybackState(const core::Track &track);
  void loadLyricsForTrack(const QString &filePath);
  void parseLrcContent(const QString &content);
  void updateActiveLyricIndex();
  void refreshLibraryStats();
  void syncQueueModel();
  void initDefaultHotkeys();
  void loadSettings();
  void saveSettings();
  static QString formatTime(qint64 ms);
  static QString formatBytes(uint64_t bytes);

  struct UndoCommand {
    QString name;
    std::function<void()> undoFn;
    std::function<void()> redoFn;
  };

  core::PlayerService &m_player;
  core::LibraryService &m_library;
  adapters::ThemeLoader *m_themeLoader{nullptr};

  AlbumListModel *m_albumModel{nullptr};
  TrackListModel *m_trackModel{nullptr};
  TrackListModel *m_albumDetailTrackModel{nullptr};
  TrackListModel *m_queueTrackModel{nullptr};
  TrackListModel *m_historyTrackModel{nullptr};

  std::vector<UndoCommand> m_undoStack;
  std::vector<UndoCommand> m_redoStack;

  // Playback state
  QString m_currentTrackTitle{""};
  QString m_currentArtist{""};
  QString m_currentAlbum{""};
  QString m_currentYear{""};
  QString m_currentGenre{""};
  QString m_currentCodec{""};
  QString m_currentAudioSpecs{""};
  QString m_currentArtHash{""};
  QString m_currentFilePath{""};
  QString m_currentFileSizeStr{""};
  int m_currentBitrate{0};
  int m_currentSampleRate{0};
  int m_currentBitDepth{0};
  int m_currentChannels{0};
  bool m_isPlaying{false};
  qint64 m_positionMs{0};
  qint64 m_durationMs{0};
  QString m_positionStr{"00:00"};
  QString m_durationStr{"00:00"};
  float m_volume{0.8f};
  float m_prevVolume{0.8f};
  bool m_isMuted{false};
  bool m_volumeLocked{false};
  bool m_showRemainingTime{false};

  // Lyrics state
  QString m_currentLyrics{""};
  bool m_hasLyrics{false};
  bool m_isLyricsSynchronized{false};
  QVector<LyricLine> m_lyricsList;
  QVariantList m_currentLyricsLines;
  int m_activeLyricIndex{-1};

  // Scanning state
  std::atomic<bool> m_isScanning{false};
  QString m_scanStatusText{""};
  int m_scanScanned{0};
  int m_scanTotal{0};

  // Filter lists
  QStringList m_genresList;
  QStringList m_artistsList;

  // Selected album info
  QString m_selectedAlbumTitle;
  QString m_selectedAlbumArtist;
  QString m_selectedAlbumYear;
  QString m_selectedAlbumGenre;
  QString m_selectedAlbumQuality;
  QString m_selectedAlbumDuration;
  int m_selectedAlbumTrackCount{0};
  QString m_selectedAlbumArtHash;

  // Playback & DSP Settings
  bool m_gaplessPlayback{true};
  bool m_crossfadeEnabled{false};
  qreal m_crossfadeDurationSec{2.0};
  QString m_crossfadeCurve{"Equal Power (Constant Volume)"};
  QString m_replayGainMode{"Smart Gain (Auto Track/Album)"};
  int m_replayGainPreampDb{0};
  int m_replayGainPreampWithoutGainDb{-6};
  bool m_truePeakLimiter{true};
  int m_shortSeekStepSec{5};
  int m_longSeekStepSec{30};
  bool m_stopAfterCurrentTrack{false};
  qreal m_playbackRate{1.0};
  qint64 m_loopPointA{-1};
  qint64 m_loopPointB{-1};
  bool m_sleepTimerActive{false};
  int m_sleepTimerRemainingSec{0};
  QTimer *m_sleepTimer{nullptr};

  // Queue Ergonomics Settings
  QString m_doubleClickAction{"Play Now"};
  QString m_middleClickAction{"Queue Last"};
  QString m_queueAutoFillMode{"Loop Context"};
  int m_historyRetentionLimit{200};

  // Library & Folders Settings
  QStringList m_monitoredFolders;
  bool m_filesystemWatcher{true};
  bool m_autoScanOnStartup{false};
  QStringList m_formatFilters{"FLAC", "WAV", "ALAC", "AIFF", "DSD (DSF/DFF)",
                              "MP3",  "AAC", "M4A",  "OGG",  "OPUS"};
  QString m_excludeFolders{
      ".*, node_modules, temp, @eaDir, System Volume Information"};
  QString m_artworkPriority{"Embedded Tags First"};
  bool m_groupAlbumsByTitle{true};

  // General Settings
  QString m_language{"System Default"};
  QString m_startupAction{"Restore Session"};
  QString m_closeAction{"Exit Application"};
  QString m_minimizeAction{"Minimize to Taskbar"};
  bool m_singleInstance{true};
  QString m_updateCheckInterval{"Weekly"};
  bool m_showNotifications{true};
  int m_notificationDurationSec{4};
  bool m_suppressNotificationsWhenFocused{true};

  // Appearance Settings
  QString m_fontFamily{"Inter, sans-serif"};
  int m_baseFontSize{11};
  bool m_waveformSeekbar{true};
  int m_tableRowHeight{32};
  bool m_tableAlternatingRows{false};
  QString m_artThumbnailQuality{"Smooth (High Quality)"};
  int m_artCacheLimitMb{1024};
  bool m_showStatusBar{true};

  // Audio Output & Pipeline Settings
  QString m_audioBackend{"Linux PipeWire Lock-Free Client"};
  QStringList m_availableAudioDevices{"System Default Output"};
  QString m_currentAudioDevice{"System Default Output"};
  bool m_bitPerfectExclusive{true};
  int m_bufferLatencyMs{50};
  QString m_resamplerQuality{"SoX Resampler High Quality"};
  QString m_ditherMode{"Flat TPDF (Triangular)"};
  QString m_channelProcessing{"Stereo Passthrough"};

  // Hotkeys Settings
  bool m_globalMediaKeysEnabled{true};
  std::vector<HotkeyEntry> m_hotkeys;

  // Metadata & Scrobbling Settings
  bool m_lastfmEnabled{false};
  QString m_lastfmUsername{""};
  QString m_lastfmSessionKey{""};
  bool m_listenbrainzEnabled{false};
  QString m_listenbrainzToken{""};
  QString m_listenbrainzApiUrl{"https://api.listenbrainz.org/1/"};
  int m_scrobbleThresholdPercent{50};
  int m_scrobbleThresholdTimeSec{240};
  bool m_offlineScrobbleCache{true};
  QString m_lyricsProviderOrder{"Local .lrc sidecar first"};
  bool m_autoFetchLyrics{true};

  // Diagnostics Settings
  QString m_loggingVerbosity{"Info"};

  // View Layout state (Issue #14)
  int m_leftPanelWidth{210};
  bool m_showLeftPanel{true};
  int m_rightPanelWidth{270};
  bool m_showRightPanel{true};
  int m_columnBrowserHeight{121};
  bool m_showColumnBrowser{false};
  int m_activeWorkspaceViewMode{1};
  int m_activeNavSection{0};
  QString m_activePlaylistName{""};

  void loadViewLayoutSettings();
  void saveViewLayoutSettings();

  struct ColumnMeta {
    QString key;
    QString title;
    QString header;
    bool visible;
    int width;
    int defaultWidth;
    bool defaultVisible;
  };

  void initTableColumns();
  void loadColumnSettings();
  void saveColumnSettings();

  std::vector<ColumnMeta> m_columns;

  QMediaDevices *m_mediaDevices{nullptr};
  QTimer *m_positionTimer{nullptr};
  QTimer *m_visualizerTimer{nullptr};
  QVariantList m_visualizerBands;
  QVariantList m_visualizerWaveform;
  float m_visualizerLevel{0.0f};

  class QQmlApplicationEngine *m_qmlEngine{nullptr};
  class QTranslator *m_translator{nullptr};
  void applyLanguage(const QString &lang);
};
