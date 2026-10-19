#include "QtBridge.hpp"
#include "core/ports/IMetadataExtractor.hpp"
#include "core/ports/IMusicDatabasePort.hpp"
#include "core/services/LibraryService.hpp"
#include "core/services/PlayerService.hpp"
#include "mocks/MockAudioEnginePort.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <gtest/gtest.h>

namespace {

class MockDb : public core::IMusicDatabasePort {
public:
  std::vector<core::Track> tracks;
  bool initialize(const std::string &) override { return true; }
  bool saveTrack(const core::Track &t) override {
    tracks.push_back(t);
    return true;
  }
  bool saveTracks(const std::vector<core::Track> &ts) override {
    tracks.insert(tracks.end(), ts.begin(), ts.end());
    return true;
  }
  std::optional<core::Track> getTrackById(const std::string &id) override {
    for (const auto &t : tracks) {
      if (t.id == id)
        return t;
    }
    return std::nullopt;
  }
  std::optional<core::Track> getTrackByPath(const std::string &path) override {
    for (const auto &t : tracks) {
      if (t.filePath == path)
        return t;
    }
    return std::nullopt;
  }
  std::vector<core::Track> getAllTracks() override { return tracks; }
  std::vector<core::Track>
  getTracksByAlbum(const std::string &albumTitle,
                   const std::string &albumArtist) override {
    std::vector<core::Track> res;
    for (const auto &t : tracks) {
      if (t.album == albumTitle &&
          (albumArtist.empty() || t.artist == albumArtist)) {
        res.push_back(t);
      }
    }
    return res;
  }
  bool deleteTrack(const std::string &id) override {
    std::erase_if(tracks, [&](const auto &t) { return t.id == id; });
    return true;
  }
  std::vector<core::Album> getAllAlbums() override { return {}; }
  std::optional<core::Album> getAlbumById(const std::string &) override {
    return std::nullopt;
  }
  void setGroupAlbumsByTitle(bool enabled) override {
    m_groupAlbumsByTitle = enabled;
  }
  bool groupAlbumsByTitle() const override { return m_groupAlbumsByTitle; }
  bool clearLibrary() override {
    tracks.clear();
    return true;
  }
  bool optimizeDatabase() override { return true; }
  size_t getTrackCount() override { return tracks.size(); }
  size_t getAlbumCount() override { return 0; }

private:
  bool m_groupAlbumsByTitle{true};
};

class MockExtractor : public core::IMetadataExtractor {
public:
  std::optional<core::Track> extract(const std::string &) override {
    return std::nullopt;
  }
  std::optional<core::ExtractedArtwork>
  extractArtwork(const std::string &) override {
    return std::nullopt;
  }
  bool supportsFormat(const std::string &) const override { return true; }
};

} // namespace

class SettingsTest : public ::testing::Test {
protected:
  static void SetUpTestSuite() {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    int argc = 1;
    static char appName[] = "picaudio_tests";
    static char *argv[] = {appName, nullptr};
    if (!QCoreApplication::instance()) {
      new QGuiApplication(argc, argv);
    }
    QCoreApplication::setOrganizationName("PicAudio");
    QCoreApplication::setApplicationName("PicAudioTest");
  }

  void SetUp() override {
    QSettings s1("PicAudio", "PicAudioTest");
    s1.clear();
    QSettings s2("PicAudio", "PicAudio");
    s2.clear();
  }

  void TearDown() override {
    QSettings s1("PicAudio", "PicAudioTest");
    s1.clear();
    QSettings s2("PicAudio", "PicAudio");
    s2.clear();
  }
};

TEST_F(SettingsTest, GeneralSettingsGettersAndSetters) {
  auto audio = std::make_shared<tests::MockAudioEnginePort>();
  MockDb db;
  MockExtractor extractor;
  core::LibraryService library(db, extractor);
  core::PlayerService player(audio);
  adapters::ThemeLoader themeLoader;

  QtBridge bridge(player, library, &themeLoader);

  // Language
  bridge.setLanguage("Deutsch");
  EXPECT_EQ(bridge.language(), "Deutsch");

  // Startup Action
  bridge.setStartupAction("Play Immediately");
  EXPECT_EQ(bridge.startupAction(), "Play Immediately");

  // Close & Minimize
  bridge.setCloseAction("Minimize to System Tray");
  EXPECT_EQ(bridge.closeAction(), "Minimize to System Tray");
  bridge.setMinimizeAction("Minimize to System Tray");
  EXPECT_EQ(bridge.minimizeAction(), "Minimize to System Tray");

  // Single-instance
  bridge.setSingleInstance(false);
  EXPECT_FALSE(bridge.singleInstance());
  bridge.setSingleInstance(true);
  EXPECT_TRUE(bridge.singleInstance());

  // Software updates
  bridge.setUpdateCheckInterval("Daily");
  EXPECT_EQ(bridge.updateCheckInterval(), "Daily");

  // Notifications
  bridge.setShowNotifications(false);
  EXPECT_FALSE(bridge.showNotifications());
  bridge.setNotificationDurationSec(8);
  EXPECT_EQ(bridge.notificationDurationSec(), 8);
  bridge.setSuppressNotificationsWhenFocused(false);
  EXPECT_FALSE(bridge.suppressNotificationsWhenFocused());
}

TEST_F(SettingsTest, AppearanceSettingsGettersAndSetters) {
  auto audio = std::make_shared<tests::MockAudioEnginePort>();
  MockDb db;
  MockExtractor extractor;
  core::LibraryService library(db, extractor);
  core::PlayerService player(audio);
  adapters::ThemeLoader themeLoader;

  QtBridge bridge(player, library, &themeLoader);

  // Typography
  bridge.setFontFamily("Roboto, sans-serif");
  EXPECT_EQ(bridge.fontFamily(), "Roboto, sans-serif");
  bridge.setBaseFontSize(14);
  EXPECT_EQ(bridge.baseFontSize(), 14);

  // Table & Layout
  bridge.setTableRowHeight(40);
  EXPECT_EQ(bridge.tableRowHeight(), 40);
  bridge.setTableAlternatingRows(true);
  EXPECT_TRUE(bridge.tableAlternatingRows());
  bridge.setWaveformSeekbar(false);
  EXPECT_FALSE(bridge.waveformSeekbar());
  bridge.setShowStatusBar(false);
  EXPECT_FALSE(bridge.showStatusBar());

  // Cover art
  bridge.setArtThumbnailQuality("Balanced (Bilinear)");
  EXPECT_EQ(bridge.artThumbnailQuality(), "Balanced (Bilinear)");
  bridge.setArtCacheLimitMb(2048);
  EXPECT_EQ(bridge.artCacheLimitMb(), 2048);
}

TEST_F(SettingsTest, AudioSettingsGettersAndSetters) {
  auto audio = std::make_shared<tests::MockAudioEnginePort>();
  MockDb db;
  MockExtractor extractor;
  core::LibraryService library(db, extractor);
  core::PlayerService player(audio);
  adapters::ThemeLoader themeLoader;

  QtBridge bridge(player, library, &themeLoader);

  // Backend
  bridge.setAudioBackend("ALSA Direct Hardware (hw:)");
  EXPECT_EQ(bridge.audioBackend(), "ALSA Direct Hardware (hw:)");

  // Device
  EXPECT_FALSE(bridge.availableAudioDevices().isEmpty());
  bridge.setCurrentAudioDevice("Custom DAC");
  EXPECT_EQ(bridge.currentAudioDevice(), "Custom DAC");

  // Exclusive mode & Latency
  bridge.setBitPerfectExclusive(false);
  EXPECT_FALSE(bridge.bitPerfectExclusive());
  bridge.setBufferLatencyMs(20);
  EXPECT_EQ(bridge.bufferLatencyMs(), 20);

  // DSP
  bridge.setResamplerQuality("Speex Resampler");
  EXPECT_EQ(bridge.resamplerQuality(), "Speex Resampler");
  bridge.setDitherMode("High-Pass TPDF");
  EXPECT_EQ(bridge.ditherMode(), "High-Pass TPDF");
  bridge.setChannelProcessing("Mono Downmix");
  EXPECT_EQ(bridge.channelProcessing(), "Mono Downmix");
}

TEST_F(SettingsTest, PlaybackDSPSettingsGettersAndSetters) {
  auto audio = std::make_shared<tests::MockAudioEnginePort>();
  MockDb db;
  MockExtractor extractor;
  core::LibraryService library(db, extractor);
  core::PlayerService player(audio);
  adapters::ThemeLoader themeLoader;

  QtBridge bridge(player, library, &themeLoader);

  // Gapless & Crossfade
  bridge.setGaplessPlayback(false);
  EXPECT_FALSE(bridge.gaplessPlayback());
  bridge.setGaplessPlayback(true);
  EXPECT_TRUE(bridge.gaplessPlayback());

  bridge.setCrossfadeEnabled(true);
  EXPECT_TRUE(bridge.crossfadeEnabled());
  bridge.setCrossfadeDurationSec(3.5);
  EXPECT_DOUBLE_EQ(bridge.crossfadeDurationSec(), 3.5);
  bridge.setCrossfadeCurve("Logarithmic Fade");
  EXPECT_EQ(bridge.crossfadeCurve(), "Logarithmic Fade");

  // ReplayGain
  bridge.setReplayGainMode("Album Gain (Preserves Album Dynamics)");
  EXPECT_EQ(bridge.replayGainMode(), "Album Gain (Preserves Album Dynamics)");
  bridge.setReplayGainPreampDb(3);
  EXPECT_EQ(bridge.replayGainPreampDb(), 3);
  bridge.setReplayGainPreampWithoutGainDb(-3);
  EXPECT_EQ(bridge.replayGainPreampWithoutGainDb(), -3);
  bridge.setTruePeakLimiter(false);
  EXPECT_FALSE(bridge.truePeakLimiter());

  // Seek & Stop
  bridge.setShortSeekStepSec(3);
  EXPECT_EQ(bridge.shortSeekStepSec(), 3);
  bridge.setLongSeekStepSec(45);
  EXPECT_EQ(bridge.longSeekStepSec(), 45);
  bridge.setStopAfterCurrentTrack(true);
  EXPECT_TRUE(bridge.stopAfterCurrentTrack());
}

TEST_F(SettingsTest, QueueSettingsGettersAndSetters) {
  auto audio = std::make_shared<tests::MockAudioEnginePort>();
  MockDb db;
  MockExtractor extractor;
  core::LibraryService library(db, extractor);
  core::PlayerService player(audio);
  adapters::ThemeLoader themeLoader;

  QtBridge bridge(player, library, &themeLoader);

  // Click Actions
  bridge.setDoubleClickAction("Play Next");
  EXPECT_EQ(bridge.doubleClickAction(), "Play Next");
  bridge.setMiddleClickAction("Play Now");
  EXPECT_EQ(bridge.middleClickAction(), "Play Now");

  // Queue End Behavior
  bridge.setQueueAutoFillMode("Smart Autoplay (Similar Tracks)");
  EXPECT_EQ(bridge.queueAutoFillMode(), "Smart Autoplay (Similar Tracks)");

  // Shuffle Strategy
  bridge.setShuffleMode(2);
  EXPECT_EQ(bridge.shuffleMode(), 2);

  // History
  bridge.setHistoryRetentionLimit(500);
  EXPECT_EQ(bridge.historyRetentionLimit(), 500);
  EXPECT_EQ(player.getHistoryLimit(), 500);

  bridge.clearPlaybackHistory();
  EXPECT_TRUE(player.getQueueService().getHistory().empty());
}

TEST_F(SettingsTest, LibrarySettingsGettersAndSetters) {
  auto audio = std::make_shared<tests::MockAudioEnginePort>();
  MockDb db;
  MockExtractor extractor;
  core::LibraryService library(db, extractor);
  core::PlayerService player(audio);
  adapters::ThemeLoader themeLoader;

  QtBridge bridge(player, library, &themeLoader);

  // Monitored Folders & Watcher
  bridge.setFilesystemWatcher(false);
  EXPECT_FALSE(bridge.filesystemWatcher());
  bridge.setAutoScanOnStartup(true);
  EXPECT_TRUE(bridge.autoScanOnStartup());

  // Format Filters
  QStringList customFormats = {"FLAC", "DSD (DSF/DFF)", "OPUS"};
  bridge.setFormatFilters(customFormats);
  EXPECT_EQ(bridge.formatFilters(), customFormats);
  EXPECT_TRUE(bridge.isFormatFilterEnabled("FLAC"));
  EXPECT_FALSE(bridge.isFormatFilterEnabled("MP3"));

  bridge.setFormatFilterEnabled("MP3", true);
  EXPECT_TRUE(bridge.isFormatFilterEnabled("MP3"));
  bridge.setFormatFilterEnabled("FLAC", false);
  EXPECT_FALSE(bridge.isFormatFilterEnabled("FLAC"));

  // Exclude Folders & Artwork Priority
  bridge.setExcludeFolders(".*, temp, build");
  EXPECT_EQ(bridge.excludeFolders(), ".*, temp, build");
  bridge.setArtworkPriority("External Folder Art First");
  EXPECT_EQ(bridge.artworkPriority(), "External Folder Art First");

  // Album Grouping By Title
  EXPECT_TRUE(bridge.groupAlbumsByTitle());
  bridge.setGroupAlbumsByTitle(false);
  EXPECT_FALSE(bridge.groupAlbumsByTitle());
  EXPECT_FALSE(db.groupAlbumsByTitle());
  bridge.setGroupAlbumsByTitle(true);
  EXPECT_TRUE(bridge.groupAlbumsByTitle());
  EXPECT_TRUE(db.groupAlbumsByTitle());

  // Database maintenance
  bridge.optimizeDatabase();
}

TEST_F(SettingsTest, HotkeysSettingsAndConflictDetection) {
  auto audio = std::make_shared<tests::MockAudioEnginePort>();
  MockDb db;
  MockExtractor extractor;
  core::LibraryService library(db, extractor);
  core::PlayerService player(audio);
  adapters::ThemeLoader themeLoader;

  QtBridge bridge(player, library, &themeLoader);

  // Global media keys
  EXPECT_TRUE(bridge.globalMediaKeysEnabled());
  bridge.setGlobalMediaKeysEnabled(false);
  EXPECT_FALSE(bridge.globalMediaKeysEnabled());

  // Hotkeys model
  auto model = bridge.hotkeysModel();
  EXPECT_FALSE(model.isEmpty());

  // Conflict detection
  EXPECT_EQ(bridge.checkHotkeyConflict("custom_action", "Space"),
            "Play / Pause playback");
  EXPECT_EQ(bridge.checkHotkeyConflict("play_pause", "Space"), "");
  EXPECT_EQ(bridge.checkHotkeyConflict("custom_action", "Ctrl+Shift+Alt+Z"),
            "");

  // Rebind and restore
  bridge.setHotkey("play_pause", "Ctrl+P");
  EXPECT_EQ(bridge.checkHotkeyConflict("custom_action", "Ctrl+P"),
            "Play / Pause playback");

  bridge.restoreDefaultHotkeys();
  EXPECT_EQ(bridge.checkHotkeyConflict("custom_action", "Space"),
            "Play / Pause playback");
}

TEST_F(SettingsTest, MetadataAndScrobblingSettings) {
  auto audio = std::make_shared<tests::MockAudioEnginePort>();
  MockDb db;
  MockExtractor extractor;
  core::LibraryService library(db, extractor);
  core::PlayerService player(audio);
  adapters::ThemeLoader themeLoader;

  QtBridge bridge(player, library, &themeLoader);

  // Last.fm
  bridge.setLastfmEnabled(true);
  EXPECT_TRUE(bridge.lastfmEnabled());
  bridge.setLastfmUsername("audiophile_user");
  EXPECT_EQ(bridge.lastfmUsername(), "audiophile_user");
  bridge.setLastfmSessionKey("session_token_12345");
  EXPECT_EQ(bridge.lastfmSessionKey(), "session_token_12345");

  // ListenBrainz
  bridge.setListenbrainzEnabled(true);
  EXPECT_TRUE(bridge.listenbrainzEnabled());
  bridge.setListenbrainzToken("lb_api_key_abc");
  EXPECT_EQ(bridge.listenbrainzToken(), "lb_api_key_abc");
  bridge.setListenbrainzApiUrl("https://custom.listenbrainz.instance/api/");
  EXPECT_EQ(bridge.listenbrainzApiUrl(),
            "https://custom.listenbrainz.instance/api/");

  // Thresholds & Cache
  bridge.setScrobbleThresholdPercent(75);
  EXPECT_EQ(bridge.scrobbleThresholdPercent(), 75);
  bridge.setScrobbleThresholdTimeSec(180);
  EXPECT_EQ(bridge.scrobbleThresholdTimeSec(), 180);
  bridge.setOfflineScrobbleCache(false);
  EXPECT_FALSE(bridge.offlineScrobbleCache());

  // Lyrics
  bridge.setLyricsProviderOrder("Embedded USLT/SYLT tags first");
  EXPECT_EQ(bridge.lyricsProviderOrder(), "Embedded USLT/SYLT tags first");
  bridge.setAutoFetchLyrics(false);
  EXPECT_FALSE(bridge.autoFetchLyrics());
}

TEST_F(SettingsTest, DiagnosticsAndFactoryReset) {
  auto audio = std::make_shared<tests::MockAudioEnginePort>();
  MockDb db;
  MockExtractor extractor;
  core::LibraryService library(db, extractor);
  core::PlayerService player(audio);
  adapters::ThemeLoader themeLoader;

  QtBridge bridge(player, library, &themeLoader);

  // Logging verbosity
  bridge.setLoggingVerbosity("Debug");
  EXPECT_EQ(bridge.loggingVerbosity(), "Debug");

  // Integrity check
  QString integrityStatus = bridge.checkDatabaseIntegrity();
  EXPECT_FALSE(integrityStatus.isEmpty());
  EXPECT_TRUE(integrityStatus.contains("Integrity check passed"));

  // Diagnostics report export
  QString reportPath = QDir::tempPath() + "/picaudio_diag_test.txt";
  bridge.exportDiagnosticsReport(reportPath);
  EXPECT_TRUE(QFile::exists(reportPath));
  QFile::remove(reportPath);

  // Reset to defaults
  bridge.setLanguage("Japanese");
  bridge.setBaseFontSize(18);
  bridge.setLastfmEnabled(true);
  bridge.setLoggingVerbosity("Error");

  bridge.resetAllSettingsToDefaults();

  EXPECT_EQ(bridge.language(), "System Default");
  EXPECT_EQ(bridge.baseFontSize(), 11);
  EXPECT_FALSE(bridge.lastfmEnabled());
  EXPECT_EQ(bridge.loggingVerbosity(), "Info");
  EXPECT_TRUE(bridge.globalMediaKeysEnabled());
}

TEST_F(SettingsTest, TrackListModelSelection) {
  MockDb db;
  TrackListModel model(&db);

  core::Track t1;
  t1.id = "t1";
  t1.title = "Song 1";
  t1.artist = "Artist 1";
  t1.durationMs = 180000;

  core::Track t2;
  t2.id = "t2";
  t2.title = "Song 2";
  t2.artist = "Artist 2";
  t2.durationMs = 240000;

  core::Track t3;
  t3.id = "t3";
  t3.title = "Song 3";
  t3.artist = "Artist 3";
  t3.durationMs = 200000;

  model.setTracks({t1, t2, t3});
  EXPECT_EQ(model.rowCount(), 3);
  EXPECT_EQ(model.selectedCount(), 0);

  // Toggle selection
  model.toggleSelection(0);
  EXPECT_TRUE(model.isSelected(0));
  EXPECT_FALSE(model.isSelected(1));
  EXPECT_EQ(model.selectedCount(), 1);

  // Set row selected
  model.setRowSelected(1, true);
  EXPECT_EQ(model.selectedCount(), 2);
  EXPECT_TRUE(model.isSelected(1));

  // Invert selection
  model.invertSelection();
  EXPECT_EQ(model.selectedCount(), 1);
  EXPECT_FALSE(model.isSelected(0));
  EXPECT_FALSE(model.isSelected(1));
  EXPECT_TRUE(model.isSelected(2));

  // Select all
  model.selectAll();
  EXPECT_EQ(model.selectedCount(), 3);
  EXPECT_TRUE(model.isSelected(0));
  EXPECT_TRUE(model.isSelected(1));
  EXPECT_TRUE(model.isSelected(2));

  // Clear selection
  model.clearSelection();
  EXPECT_EQ(model.selectedCount(), 0);
  EXPECT_FALSE(model.isSelected(0));

  // Select range
  model.selectRange(0, 1);
  EXPECT_EQ(model.selectedCount(), 2);
  EXPECT_TRUE(model.isSelected(0));
  EXPECT_TRUE(model.isSelected(1));
  EXPECT_FALSE(model.isSelected(2));
  model.clearSelection();

  // Get selected tracks and remove
  model.setRowSelected(1, true);
  auto selectedIds = model.getSelectedTrackIds();
  EXPECT_EQ(selectedIds.size(), 1);
  EXPECT_EQ(selectedIds[0].toString(), "t2");

  model.removeSelected();
  EXPECT_EQ(model.rowCount(), 2);
  EXPECT_EQ(model.selectedCount(), 0);
}

TEST_F(SettingsTest, FileAndEditMenuActions) {
  auto audio = std::make_shared<tests::MockAudioEnginePort>();
  MockDb db;
  MockExtractor extractor;
  core::LibraryService library(db, extractor);
  core::PlayerService player(audio);
  adapters::ThemeLoader themeLoader;

  QtBridge bridge(player, library, &themeLoader);

  // Undo / Redo initial state
  EXPECT_FALSE(bridge.canUndo());
  EXPECT_FALSE(bridge.canRedo());

  // Push custom command
  bool commandExecuted = false;
  bridge.pushUndoCommand(
      "Test Action", [&commandExecuted]() { commandExecuted = false; },
      [&commandExecuted]() { commandExecuted = true; });

  EXPECT_TRUE(bridge.canUndo());
  EXPECT_FALSE(bridge.canRedo());
  EXPECT_EQ(bridge.undoActionName(), "Test Action");

  bridge.undo();
  EXPECT_FALSE(bridge.canUndo());
  EXPECT_TRUE(bridge.canRedo());
  EXPECT_EQ(bridge.redoActionName(), "Test Action");

  bridge.redo();
  EXPECT_TRUE(bridge.canUndo());
  EXPECT_FALSE(bridge.canRedo());

  // Export Active View
  core::Track t;
  t.id = "test_export";
  t.title = "Export Title";
  t.artist = "Export Artist";
  t.album = "Export Album";
  t.genre = "Test";
  t.year = 2024;
  t.trackNumber = 1;
  t.durationMs = 120000;
  t.filePath = "/tmp/test.flac";
  t.codec = "FLAC";
  t.bitrate = 1411;
  bridge.trackModel()->setTracks({t});

  QString m3u8Path = QDir::tempPath() + "/picaudio_test_export.m3u8";
  QString csvPath = QDir::tempPath() + "/picaudio_test_export.csv";
  QString jsonPath = QDir::tempPath() + "/picaudio_test_export.json";

  EXPECT_TRUE(bridge.exportActiveView(m3u8Path, "m3u8"));
  EXPECT_TRUE(QFile::exists(m3u8Path));
  QFile::remove(m3u8Path);

  EXPECT_TRUE(bridge.exportActiveView(csvPath, "csv"));
  EXPECT_TRUE(QFile::exists(csvPath));
  QFile::remove(csvPath);

  EXPECT_TRUE(bridge.exportActiveView(jsonPath, "json"));
  EXPECT_TRUE(QFile::exists(jsonPath));
  QFile::remove(jsonPath);

  // Network stream
  bridge.openNetworkStream("http://stream.example.com/audio.mp3",
                           "Example Radio");
  EXPECT_EQ(bridge.currentTrackTitle(), "Example Radio");

  // History Track Model
  EXPECT_NE(bridge.historyTrackModel(), nullptr);
  bridge.refreshHistory();
}

TEST_F(SettingsTest, PlaybackAndLibraryMenuActions) {
  auto audio = std::make_shared<tests::MockAudioEnginePort>();
  MockDb db;
  MockExtractor extractor;
  core::LibraryService library(db, extractor);
  core::PlayerService player(audio);
  adapters::ThemeLoader themeLoader;
  QtBridge bridge(player, library, &themeLoader);

  // Playback Rate
  bridge.setPlaybackRate(1.25);
  EXPECT_NEAR(bridge.playbackRate(), 1.25, 0.001);
  bridge.setPlaybackRate(0.5);
  EXPECT_NEAR(bridge.playbackRate(), 0.5, 0.001);
  bridge.setPlaybackRate(1.0);
  EXPECT_NEAR(bridge.playbackRate(), 1.0, 0.001);

  // A-B Looping
  EXPECT_FALSE(bridge.isLoopActive());
  bridge.setLoopPointA();
  EXPECT_GE(bridge.loopPointA(), 0);
  bridge.setLoopPointB();
  bridge.clearLoop();
  EXPECT_FALSE(bridge.isLoopActive());
  EXPECT_EQ(bridge.loopPointA(), -1);
  EXPECT_EQ(bridge.loopPointB(), -1);

  // Stop After Current Track
  EXPECT_FALSE(bridge.stopAfterCurrentTrack());
  bridge.toggleStopAfterCurrentTrack();
  EXPECT_TRUE(bridge.stopAfterCurrentTrack());
  bridge.toggleStopAfterCurrentTrack();
  EXPECT_FALSE(bridge.stopAfterCurrentTrack());

  // Sleep Timer
  EXPECT_FALSE(bridge.isSleepTimerActive());
  bridge.startSleepTimer(15);
  EXPECT_TRUE(bridge.isSleepTimerActive());
  EXPECT_EQ(bridge.sleepTimerRemainingSec(), 15 * 60);
  bridge.cancelSleepTimer();
  EXPECT_FALSE(bridge.isSleepTimerActive());
  EXPECT_EQ(bridge.sleepTimerRemainingSec(), 0);

  // Playlist creation
  EXPECT_TRUE(bridge.createPlaylist("Test_Favorites"));
  QString plPath =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
      "/playlists/Test_Favorites.m3u8";
  EXPECT_TRUE(QFile::exists(plPath));
  QFile::remove(plPath);

  // Smart Playlist creation
  EXPECT_TRUE(
      bridge.createSmartPlaylist("High_Bitrate", "{\"genre\":\"Rock\"}"));
  QString smartPath =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
      "/playlists/High_Bitrate.smart.json";
  EXPECT_TRUE(QFile::exists(smartPath));
  QFile::remove(smartPath);

  // Diagnostics
  auto diag = bridge.audioPipelineDiagnostics();
  EXPECT_TRUE(diag.contains("sampleRate"));
  EXPECT_TRUE(diag.contains("bitDepth"));
  EXPECT_TRUE(diag.contains("codec"));
  EXPECT_TRUE(diag.contains("audioBackend"));
  EXPECT_TRUE(diag.contains("isBitPerfect"));

  // Log file path
  QString logPath = bridge.getLogFilePath();
  EXPECT_TRUE(logPath.endsWith("picaudio.log"));
  EXPECT_TRUE(QFile::exists(logPath));

  // Deduplicate tracks
  core::Track t1;
  t1.id = "dup_1";
  t1.title = "Duplicate Song";
  t1.artist = "Duplicate Artist";
  t1.durationMs = 180000;

  core::Track t2;
  t2.id = "dup_2";
  t2.title = "Duplicate Song";
  t2.artist = "Duplicate Artist";
  t2.durationMs = 180000;

  db.saveTrack(t1);
  db.saveTrack(t2);

  int removed = bridge.deduplicateTracks();
  EXPECT_GE(removed, 1);
  EXPECT_TRUE(bridge.canUndo());
  bridge.undo();
  EXPECT_TRUE(bridge.canRedo());
  bridge.redo();

  // Issue #10: Context Menu Actions & Multi-Select Operations
  core::Track ctx1;
  ctx1.id = "ctx_1";
  ctx1.title = "Context Track 1";
  ctx1.artist = "Context Artist";
  ctx1.album = "Context Album";
  ctx1.filePath = "/tmp/picaudio_test_ctx_1.mp3";
  ctx1.durationMs = 120000;

  core::Track ctx2;
  ctx2.id = "ctx_2";
  ctx2.title = "Context Track 2";
  ctx2.artist = "Context Artist";
  ctx2.album = "Context Album";
  ctx2.filePath = "/tmp/picaudio_test_ctx_2.mp3";
  ctx2.durationMs = 150000;

  core::Track ctx3;
  ctx3.id = "ctx_3";
  ctx3.title = "Context Track 3";
  ctx3.artist = "Other Artist";
  ctx3.album = "Other Album";
  ctx3.filePath = "/tmp/picaudio_test_ctx_3.mp3";
  ctx3.durationMs = 180000;

  db.saveTrack(ctx1);
  db.saveTrack(ctx2);
  db.saveTrack(ctx3);
  bridge.trackModel()->loadAllTracks();

  // Play Now (with selection and context)
  bridge.playSelectionNow(QStringList{"ctx_2"});
  EXPECT_EQ(bridge.currentTrackTitle(), "Context Track 2");
  EXPECT_EQ(bridge.currentArtist(), "Context Artist");

  // Multi-select Play Next
  bridge.playTracksNext(QStringList{"ctx_1", "ctx_3"});
  EXPECT_GE(bridge.upNextCount(), 2);

  // Multi-select Queue Last
  bridge.queueTracksLast(QStringList{"ctx_1"});
  EXPECT_GE(bridge.upNextCount(), 3);

  // Play Next in Album Shuffle
  bridge.playNextInAlbumShuffle(QStringList{"ctx_1"});
  EXPECT_GE(bridge.upNextCount(), 3);

  // Ratings & Favorites
  bridge.setTracksRating(QStringList{"ctx_1", "ctx_2"}, 4);
  EXPECT_EQ(bridge.getTrackRating("ctx_1"), 4);
  EXPECT_EQ(bridge.getTrackRating("ctx_2"), 4);
  bridge.setTracksRating(QStringList{"ctx_1"}, 0);
  EXPECT_EQ(bridge.getTrackRating("ctx_1"), 0);

  EXPECT_FALSE(bridge.isTrackFavorite("ctx_1"));
  bridge.toggleTracksFavorite(QStringList{"ctx_1"});
  EXPECT_TRUE(bridge.isTrackFavorite("ctx_1"));
  bridge.toggleTracksFavorite(QStringList{"ctx_1"});
  EXPECT_FALSE(bridge.isTrackFavorite("ctx_1"));

  // Add / Remove from Playlist
  QVariantList plTracks;
  QVariantMap m1;
  m1["id"] = "ctx_1";
  m1["title"] = "Context Track 1";
  m1["artist"] = "Context Artist";
  m1["filePath"] = "/tmp/picaudio_test_ctx_1.mp3";
  m1["durationMs"] = 120000;
  plTracks.append(m1);

  EXPECT_TRUE(bridge.addTracksToPlaylist("CtxPlaylist", plTracks));
  EXPECT_TRUE(bridge.availablePlaylists().contains("CtxPlaylist"));

  EXPECT_TRUE(bridge.removeTracksFromPlaylist("CtxPlaylist", plTracks));
  QString ctxPlPath =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
      "/playlists/CtxPlaylist.m3u8";
  QFile::remove(ctxPlPath);

  // Delete File from Disk
  QFile dummyFile("/tmp/picaudio_test_delete.mp3");
  EXPECT_TRUE(dummyFile.open(QIODevice::WriteOnly));
  dummyFile.write("dummy audio content");
  dummyFile.close();
  EXPECT_TRUE(QFile::exists("/tmp/picaudio_test_delete.mp3"));

  core::Track delTrack;
  delTrack.id = "del_1";
  delTrack.title = "Delete Me";
  delTrack.filePath = "/tmp/picaudio_test_delete.mp3";
  db.saveTrack(delTrack);

  EXPECT_TRUE(bridge.deleteFilesFromDisk(QStringList{"del_1"}));
  EXPECT_FALSE(QFile::exists("/tmp/picaudio_test_delete.mp3"));
  EXPECT_FALSE(db.getTrackById("del_1").has_value());

  // Issue #11: Column Headers, Widths, Visibility, Sorting & Viewport Actions
  QStringList expectedKeys = {
      "trackNumber", "title",     "artist", "album",     "albumArtist",
      "genre",       "year",      "codec",  "bitrate",   "sampleRate",
      "bitDepth",    "playCount", "rating", "dateAdded", "filePath",
      "fileSize",    "duration"};
  QStringList actualKeys = bridge.tableColumnKeys();
  EXPECT_EQ(actualKeys.size(), 17);
  for (const auto &k : expectedKeys) {
    EXPECT_TRUE(actualKeys.contains(k));
  }

  // Column visibility toggling
  EXPECT_TRUE(bridge.isColumnVisible("title"));
  bridge.setColumnVisible("title", false);
  EXPECT_FALSE(bridge.isColumnVisible("title"));
  bridge.toggleColumnVisible("title");
  EXPECT_TRUE(bridge.isColumnVisible("title"));

  EXPECT_FALSE(bridge.isColumnVisible("sampleRate"));
  bridge.toggleColumnVisible("sampleRate");
  EXPECT_TRUE(bridge.isColumnVisible("sampleRate"));

  // Column width adjustment
  int origWidth = bridge.getColumnWidth("artist");
  bridge.setColumnWidth("artist", 185);
  EXPECT_EQ(bridge.getColumnWidth("artist"), 185);

  // Auto-size selected column & all columns
  bridge.autoSizeColumn("title");
  EXPECT_GE(bridge.getColumnWidth("title"), 120);
  bridge.autoSizeAllColumns();
  EXPECT_GE(bridge.getColumnWidth("artist"), 80);

  // Reset to default columns & widths
  bridge.resetTableColumns();
  EXPECT_TRUE(bridge.isColumnVisible("title"));
  EXPECT_FALSE(bridge.isColumnVisible("sampleRate"));
  EXPECT_EQ(bridge.getColumnWidth("artist"), 130);

  // Sorting ascending / descending in TrackListModel
  core::Track sortA;
  sortA.id = "sort_a";
  sortA.title = "Alpha";
  sortA.artist = "Zeta Artist";
  sortA.album = "B Album";
  sortA.year = 2010;
  sortA.durationMs = 60000;
  sortA.trackNumber = 3;

  core::Track sortB;
  sortB.id = "sort_b";
  sortB.title = "Beta";
  sortB.artist = "Alpha Artist";
  sortB.album = "A Album";
  sortB.year = 2020;
  sortB.durationMs = 180000;
  sortB.trackNumber = 1;

  db.saveTrack(sortA);
  db.saveTrack(sortB);
  bridge.trackModel()->loadAllTracks();

  // Sort by title ascending
  bridge.trackModel()->sortBy("title", true);
  EXPECT_EQ(bridge.trackModel()->sortColumn(), "title");
  EXPECT_TRUE(bridge.trackModel()->sortAscending());
  EXPECT_EQ(bridge.trackModel()->getTrackAt(0)["title"].toString(), "Alpha");

  // Sort by title descending
  bridge.trackModel()->sortBy("title", false);
  EXPECT_FALSE(bridge.trackModel()->sortAscending());
  auto lastTrackTitle = bridge.trackModel()->getTrackAt(0)["title"].toString();
  EXPECT_NE(lastTrackTitle, "Alpha");

  // Toggle sort
  bridge.trackModel()->toggleSort("title");
  EXPECT_TRUE(bridge.trackModel()->sortAscending());
  EXPECT_EQ(bridge.trackModel()->getTrackAt(0)["title"].toString(), "Alpha");

  // Sort by year
  bridge.trackModel()->sortBy("year", true);
  EXPECT_EQ(bridge.trackModel()->sortColumn(), "year");

  // Sort by track number
  bridge.trackModel()->sortBy("trackNumber", true);
  EXPECT_EQ(bridge.trackModel()->sortColumn(), "trackNumber");

  // Clear sort
  bridge.trackModel()->clearSort();
  EXPECT_TRUE(bridge.trackModel()->sortColumn().isEmpty());

  // Check roles and formatting for new columns
  QVariantMap trackData = bridge.trackModel()->getTrackAt(0);
  EXPECT_TRUE(trackData.contains("fileSizeBytes"));
  EXPECT_TRUE(trackData.contains("fileSizeFormatted"));
  EXPECT_TRUE(trackData.contains("dateAdded"));
  EXPECT_TRUE(trackData.contains("dateAddedFormatted"));
  EXPECT_TRUE(trackData.contains("playCount"));
  EXPECT_TRUE(trackData.contains("rating"));
}

TEST_F(SettingsTest, AlbumCardAndFolderContextMenuTest) {
  auto audio = std::make_shared<tests::MockAudioEnginePort>();
  MockDb db;
  MockExtractor extractor;
  core::LibraryService library(db, extractor);
  core::PlayerService player(audio);
  adapters::ThemeLoader themeLoader;

  QtBridge bridge(player, library, &themeLoader);

  core::Track t1;
  t1.id = "trk1";
  t1.title = "Comfortably Numb";
  t1.artist = "Pink Floyd";
  t1.album = "The Wall";
  t1.filePath = "/music/pinkfloyd/thewall/01.flac";
  t1.durationMs = 382000;

  core::Track t2;
  t2.id = "trk2";
  t2.title = "Hey You";
  t2.artist = "Pink Floyd";
  t2.album = "The Wall";
  t2.filePath = "/music/pinkfloyd/thewall/02.flac";
  t2.durationMs = 280000;

  db.saveTrack(t1);
  db.saveTrack(t2);

  // 1. getAlbumTracks
  QVariantList albTracks = bridge.getAlbumTracks("The Wall", "Pink Floyd");
  EXPECT_EQ(albTracks.size(), 2);
  EXPECT_EQ(albTracks[0].toMap()["title"].toString(), "Comfortably Numb");
  EXPECT_EQ(albTracks[1].toMap()["title"].toString(), "Hey You");

  // 2. addAlbumToPlaylist
  bridge.addAlbumToPlaylist("The Wall", "Pink Floyd", "FloydClassics");
  EXPECT_TRUE(bridge.availablePlaylists().contains("FloydClassics"));

  // 3. exportAlbumArtwork graceful failure when no artwork
  EXPECT_FALSE(bridge.exportAlbumArtwork("The Wall", "Pink Floyd",
                                         "/tmp/nonexistent/art.jpg"));

  // 4. showAlbumInFileManager and searchAlbumArtworkOnline calls
  bridge.showAlbumInFileManager("The Wall", "Pink Floyd");
  bridge.searchAlbumArtworkOnline("The Wall", "Pink Floyd");

  // 5. Monitored folders context actions
  QString testDir =
      QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
      "/picaudio_monitored_test";
  QDir().mkpath(testDir);
  bridge.addMonitoredFolder(testDir);
  EXPECT_TRUE(bridge.monitoredFolders().contains(testDir));

  bridge.rescanMonitoredFolder(testDir);
  EXPECT_TRUE(bridge.monitoredFolders().contains(testDir));

  bridge.removeMonitoredFolderPath(testDir);
  EXPECT_FALSE(bridge.monitoredFolders().contains(testDir));
  QDir(testDir).removeRecursively();

  // Cleanup FloydClassics playlist
  bridge.deletePlaylist("FloydClassics");
}

TEST_F(SettingsTest, PlaylistAndQueueContextMenuTest) {
  auto audio = std::make_shared<tests::MockAudioEnginePort>();
  MockDb db;
  MockExtractor extractor;
  core::LibraryService library(db, extractor);
  core::PlayerService player(audio);
  adapters::ThemeLoader themeLoader;

  QtBridge bridge(player, library, &themeLoader);

  // 1. Create static playlist
  bridge.createPlaylist("TripHop");
  EXPECT_TRUE(bridge.availablePlaylists().contains("TripHop"));
  EXPECT_FALSE(bridge.isSmartPlaylist("TripHop"));

  // 2. Duplicate playlist
  bridge.duplicatePlaylist("TripHop");
  EXPECT_TRUE(bridge.availablePlaylists().contains("TripHop (Copy)"));

  // 3. Rename playlist
  bridge.renamePlaylist("TripHop", "Downtempo");
  EXPECT_FALSE(bridge.availablePlaylists().contains("TripHop"));
  EXPECT_TRUE(bridge.availablePlaylists().contains("Downtempo"));

  // 4. Smart playlist creation, rule inspection and update
  bridge.createSmartPlaylist("ElectronicFavs", "{\"genre\":\"Electronic\"}");
  EXPECT_TRUE(bridge.availablePlaylists().contains("ElectronicFavs"));
  EXPECT_TRUE(bridge.isSmartPlaylist("ElectronicFavs"));
  EXPECT_TRUE(
      bridge.getSmartPlaylistRules("ElectronicFavs").contains("Electronic"));

  bridge.updateSmartPlaylistRules("ElectronicFavs", "{\"genre\":\"Ambient\"}");
  EXPECT_TRUE(
      bridge.getSmartPlaylistRules("ElectronicFavs").contains("Ambient"));

  // 5. Play / enqueue playlist actions
  bridge.playPlaylist("Downtempo");
  bridge.playPlaylistNext("Downtempo");
  bridge.queuePlaylistLast("Downtempo");

  // 6. Save queue as playlist
  core::Track qTrack;
  qTrack.id = "q1";
  qTrack.title = "Queue Track 1";
  qTrack.artist = "Queue Artist";
  qTrack.filePath = "/music/q1.flac";
  qTrack.durationMs = 120000;
  db.saveTrack(qTrack);

  bridge.playSelectionNow({"q1"});
  bridge.saveQueueAsPlaylist("QueueSnapshot");
  EXPECT_TRUE(bridge.availablePlaylists().contains("QueueSnapshot"));

  // 7. Export playlist
  QString exportDest =
      QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
      "/downtempo_export.m3u8";
  bridge.exportPlaylistFile("Downtempo", exportDest);
  EXPECT_TRUE(QFile::exists(exportDest));
  QFile::remove(exportDest);

  // 8. Delete playlists
  bridge.deletePlaylist("Downtempo");
  bridge.deletePlaylist("TripHop (Copy)");
  bridge.deletePlaylist("ElectronicFavs");
  bridge.deletePlaylist("QueueSnapshot");

  EXPECT_FALSE(bridge.availablePlaylists().contains("Downtempo"));
  EXPECT_FALSE(bridge.availablePlaylists().contains("TripHop (Copy)"));
  EXPECT_FALSE(bridge.availablePlaylists().contains("ElectronicFavs"));
  EXPECT_FALSE(bridge.availablePlaylists().contains("QueueSnapshot"));
}

TEST_F(SettingsTest, TransportDeckAndDeckContextMenuTest) {
  auto audio = std::make_shared<tests::MockAudioEnginePort>();
  MockDb db;
  MockExtractor extractor;
  core::LibraryService library(db, extractor);
  core::PlayerService player(audio);
  adapters::ThemeLoader themeLoader;

  QtBridge bridge(player, library, &themeLoader);

  // 1. Remaining time toggle
  EXPECT_FALSE(bridge.showRemainingTime());
  bridge.setShowRemainingTime(true);
  EXPECT_TRUE(bridge.showRemainingTime());
  bridge.setShowRemainingTime(false);
  EXPECT_FALSE(bridge.showRemainingTime());

  // 2. Volume lock toggle
  EXPECT_FALSE(bridge.volumeLocked());
  bridge.setVolumeLocked(true);
  EXPECT_TRUE(bridge.volumeLocked());
  bridge.setVolumeLocked(false);
  EXPECT_FALSE(bridge.volumeLocked());

  // 3. Duration formatting utility
  EXPECT_EQ(bridge.formatDuration(0), "00:00");
  EXPECT_EQ(bridge.formatDuration(65000), "01:05");
  EXPECT_EQ(bridge.formatDuration(3661000), "1:01:01");

  // 4. Volume presets and mute toggle
  bridge.setVolume(0.25f);
  EXPECT_FLOAT_EQ(bridge.volume(), 0.25f);
  bridge.setVolume(0.50f);
  EXPECT_FLOAT_EQ(bridge.volume(), 0.50f);
  bridge.setVolume(0.75f);
  EXPECT_FLOAT_EQ(bridge.volume(), 0.75f);
  bridge.setVolume(1.00f);
  EXPECT_FLOAT_EQ(bridge.volume(), 1.00f);

  bridge.toggleMute();
  EXPECT_TRUE(bridge.isMuted());
  bridge.toggleMute();
  EXPECT_FALSE(bridge.isMuted());
}

TEST_F(SettingsTest, NowPlayingAndLyricsTest) {
  auto audio = std::make_shared<tests::MockAudioEnginePort>();
  MockDb db;
  MockExtractor extractor;
  core::LibraryService library(db, extractor);
  core::PlayerService player(audio);
  adapters::ThemeLoader themeLoader;

  QtBridge bridge(player, library, &themeLoader);

  // Initially no lyrics
  EXPECT_FALSE(bridge.hasLyrics());
  EXPECT_TRUE(bridge.currentLyrics().isEmpty());
  EXPECT_EQ(bridge.activeLyricIndex(), -1);

  // Create a temporary .lrc file
  QTemporaryDir tempDir;
  ASSERT_TRUE(tempDir.isValid());
  QString audioFile = tempDir.filePath("test_track.flac");
  QString lrcFile = tempDir.filePath("test_track.lrc");

  QFile file(lrcFile);
  ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
  QTextStream out(&file);
  out << "[ti:Test Track]\n";
  out << "[ar:Test Artist]\n";
  out << "[00:05.00]Line 1 lyrics\n";
  out << "[00:15.50]Line 2 lyrics\n";
  out << "[00:30.00]Line 3 lyrics\n";
  file.close();

  // Create empty dummy audio file
  QFile dummyAudio(audioFile);
  ASSERT_TRUE(dummyAudio.open(QIODevice::WriteOnly));
  dummyAudio.write("dummy");
  dummyAudio.close();

  // Load track into player
  core::Track track;
  track.id = "track_1";
  track.filePath = audioFile.toStdString();
  track.title = "Test Track";
  track.artist = "Test Artist";
  track.album = "Test Album";
  track.durationMs = 60000;
  db.saveTrack(track);

  bridge.playNow("track_1");

  EXPECT_TRUE(bridge.hasLyrics());
  EXPECT_TRUE(bridge.isLyricsSynchronized());
  EXPECT_EQ(bridge.currentLyricsLines().size(), 3);

  // Check seek to lyric and activeLyricIndex
  bridge.seek(6000);
  EXPECT_EQ(bridge.activeLyricIndex(), 0);

  bridge.seek(20000);
  EXPECT_EQ(bridge.activeLyricIndex(), 1);

  bridge.seekToLyric(2);
  EXPECT_EQ(bridge.activeLyricIndex(), 2);
  EXPECT_GE(bridge.positionMs(), 30000);

  // Stop resets lyric index
  bridge.stop();
  EXPECT_EQ(bridge.activeLyricIndex(), -1);
}

TEST_F(SettingsTest, WindowGeometryAndMultiMonitorBounds) {
  auto audio = std::make_shared<tests::MockAudioEnginePort>();
  MockDb db;
  MockExtractor extractor;
  core::LibraryService library(db, extractor);
  core::PlayerService player(audio);
  adapters::ThemeLoader themeLoader;

  QtBridge bridge(player, library, &themeLoader);

  // 1. Initial load when no geometry was saved
  QVariantMap initGeom = bridge.loadWindowGeometry(600, 400);
  EXPECT_FALSE(initGeom["valid"].toBool());
  EXPECT_EQ(initGeom["width"].toInt(), 600);
  EXPECT_EQ(initGeom["height"].toInt(), 400);

  // 2. Save valid geometry and load it back
  bridge.saveWindowGeometry(120, 160, 500, 350, false);
  QVariantMap savedGeom = bridge.loadWindowGeometry(600, 400);
  EXPECT_TRUE(savedGeom["valid"].toBool());
  EXPECT_FALSE(savedGeom["maximized"].toBool());
  EXPECT_EQ(savedGeom["x"].toInt(), 120);
  EXPECT_EQ(savedGeom["y"].toInt(), 160);
  EXPECT_EQ(savedGeom["width"].toInt(), 500);
  EXPECT_EQ(savedGeom["height"].toInt(), 350);

  // 3. Maximized window geometry
  bridge.saveWindowGeometry(50, 50, 500, 350, true);
  QVariantMap maxGeom = bridge.loadWindowGeometry(600, 400);
  EXPECT_TRUE(maxGeom["valid"].toBool());
  EXPECT_TRUE(maxGeom["maximized"].toBool());

  // 4. Safe monitor bounds checking on off-screen / disconnected coordinates
  bridge.saveWindowGeometry(-99999, -99999, 500, 350, false);
  QVariantMap fallbackGeom = bridge.loadWindowGeometry(600, 400);
  EXPECT_TRUE(fallbackGeom["valid"].toBool());
  // Off-screen coordinates must be clamped/re-centered rather than returning
  // -99999
  EXPECT_NE(fallbackGeom["x"].toInt(), -99999);
  EXPECT_NE(fallbackGeom["y"].toInt(), -99999);
  EXPECT_GE(fallbackGeom["x"].toInt(), -500);
  EXPECT_GE(fallbackGeom["y"].toInt(), -500);
}

TEST_F(SettingsTest, ViewLayoutAndPanelDimensionsPersistence) {
  auto audio = std::make_shared<tests::MockAudioEnginePort>();
  MockDb db;
  MockExtractor extractor;
  core::LibraryService library(db, extractor);
  core::PlayerService player(audio);
  adapters::ThemeLoader themeLoader;

  QtBridge bridge(player, library, &themeLoader);

  // Default values
  EXPECT_EQ(bridge.leftPanelWidth(), 210);
  EXPECT_TRUE(bridge.showLeftPanel());
  EXPECT_EQ(bridge.rightPanelWidth(), 270);
  EXPECT_TRUE(bridge.showRightPanel());
  EXPECT_EQ(bridge.columnBrowserHeight(), 121);
  EXPECT_FALSE(bridge.showColumnBrowser());
  EXPECT_EQ(bridge.activeWorkspaceViewMode(), 1);
  EXPECT_EQ(bridge.activeNavSection(), 0);

  // Update layout properties
  bridge.setLeftPanelWidth(290);
  bridge.setShowLeftPanel(false);
  bridge.setRightPanelWidth(375);
  bridge.setShowRightPanel(false);
  bridge.setColumnBrowserHeight(260);
  bridge.setShowColumnBrowser(true);
  bridge.setActiveWorkspaceViewMode(1);
  bridge.setActiveNavSection(2);
  bridge.setActivePlaylistName("Synthwave Favorites");

  EXPECT_EQ(bridge.leftPanelWidth(), 290);
  EXPECT_FALSE(bridge.showLeftPanel());
  EXPECT_EQ(bridge.rightPanelWidth(), 375);
  EXPECT_FALSE(bridge.showRightPanel());
  EXPECT_EQ(bridge.columnBrowserHeight(), 260);
  EXPECT_TRUE(bridge.showColumnBrowser());
  EXPECT_EQ(bridge.activeWorkspaceViewMode(), 1);
  EXPECT_EQ(bridge.activeNavSection(), 2);
  EXPECT_EQ(bridge.activePlaylistName(), "Synthwave Favorites");

  // Verify persistence in a fresh bridge instance
  QtBridge bridge2(player, library, &themeLoader);
  EXPECT_EQ(bridge2.leftPanelWidth(), 290);
  EXPECT_FALSE(bridge2.showLeftPanel());
  EXPECT_EQ(bridge2.rightPanelWidth(), 375);
  EXPECT_FALSE(bridge2.showRightPanel());
  EXPECT_EQ(bridge2.columnBrowserHeight(), 260);
  EXPECT_TRUE(bridge2.showColumnBrowser());
  EXPECT_EQ(bridge2.activeWorkspaceViewMode(), 1);
  EXPECT_EQ(bridge2.activeNavSection(), 2);
  EXPECT_EQ(bridge2.activePlaylistName(), "Synthwave Favorites");
}

TEST_F(SettingsTest, TableColumnOrderAndCustomStatePersistence) {
  auto audio = std::make_shared<tests::MockAudioEnginePort>();
  MockDb db;
  MockExtractor extractor;
  core::LibraryService library(db, extractor);
  core::PlayerService player(audio);
  adapters::ThemeLoader themeLoader;

  QtBridge bridge(player, library, &themeLoader);

  // Initial default order
  QStringList initialOrder = bridge.tableColumnOrder();
  ASSERT_EQ(initialOrder.size(), 17);
  EXPECT_EQ(initialOrder[0], "trackNumber");
  EXPECT_EQ(initialOrder[1], "title");
  EXPECT_EQ(initialOrder[2], "artist");

  // Reorder via moveColumn (move artist at index 2 to index 0)
  bridge.moveColumn(2, 0);
  QStringList reordered = bridge.tableColumnOrder();
  EXPECT_EQ(reordered[0], "artist");
  EXPECT_EQ(reordered[1], "trackNumber");
  EXPECT_EQ(reordered[2], "title");

  // Reorder via setColumnOrder
  QStringList customOrder = initialOrder;
  customOrder.swapItemsAt(0, 3); // swap trackNumber with album
  bridge.setColumnOrder(customOrder);
  EXPECT_EQ(bridge.tableColumnOrder()[0], "album");

  // Column width and sort persistence
  bridge.setColumnWidth("album", 220);
  bridge.setColumnWidth("title", 310);
  bridge.trackModel()->sortBy("artist", false);

  // Reload into fresh bridge instance
  QtBridge bridge2(player, library, &themeLoader);
  EXPECT_EQ(bridge2.tableColumnOrder()[0], "album");
  EXPECT_EQ(bridge2.getColumnWidth("album"), 220);
  EXPECT_EQ(bridge2.getColumnWidth("title"), 310);
  EXPECT_EQ(bridge2.trackModel()->sortColumn(), "artist");
  EXPECT_FALSE(bridge2.trackModel()->sortAscending());

  // Reset columns restored default order
  bridge2.resetTableColumns();
  EXPECT_EQ(bridge2.tableColumnOrder()[0], "trackNumber");
  EXPECT_EQ(bridge2.tableColumnOrder()[1], "title");
}

TEST_F(SettingsTest, PlaybackSessionSaveAndRestoreModes) {
  auto audio = std::make_shared<tests::MockAudioEnginePort>();
  MockDb db;
  MockExtractor extractor;
  core::LibraryService library(db, extractor);
  core::PlayerService player(audio);
  adapters::ThemeLoader themeLoader;

  QTemporaryDir tempDir;
  ASSERT_TRUE(tempDir.isValid());

  QString path1 = tempDir.filePath("sess_1.flac");
  QString path2 = tempDir.filePath("sess_2.flac");
  QString path3 = tempDir.filePath("sess_3.flac");

  for (const QString &p : {path1, path2, path3}) {
    QFile f(p);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("dummy audio data");
    f.close();
  }

  core::Track t1;
  t1.id = "sess_1";
  t1.title = "Session Song 1";
  t1.artist = "Session Artist";
  t1.album = "Session Album";
  t1.filePath = path1.toStdString();
  t1.durationMs = 210000;

  core::Track t2;
  t2.id = "sess_2";
  t2.title = "Session Song 2";
  t2.artist = "Session Artist";
  t2.album = "Session Album";
  t2.filePath = path2.toStdString();
  t2.durationMs = 180000;

  core::Track t3;
  t3.id = "sess_3";
  t3.title = "Session Song 3";
  t3.artist = "Session Artist";
  t3.album = "Session Album";
  t3.filePath = path3.toStdString();
  t3.durationMs = 240000;

  db.saveTrack(t1);
  db.saveTrack(t2);
  db.saveTrack(t3);

  QtBridge bridge(player, library, &themeLoader);
  bridge.trackModel()->loadAllTracks();

  // Set playback attributes
  bridge.setVolume(0.65f);
  bridge.toggleMute(); // mute = true
  bridge.setShuffleMode(1);
  bridge.setRepeatMode(2);

  bridge.playSelectionNow(QStringList{"sess_1"});
  EXPECT_TRUE(player.getCurrentTrack().has_value());
  bridge.playTracksNext(QStringList{"sess_2"});
  bridge.queueTracksLast(QStringList{"sess_3"});
  bridge.seek(42000);

  // Save session
  bridge.savePlaybackSession();

  QSettings s("PicAudio", "PicAudio");
  s.sync();
  EXPECT_TRUE(s.value("session/has_session").toBool());
  EXPECT_EQ(s.value("session/track_id").toString(), "sess_1");
  EXPECT_EQ(s.value("session/position_ms").toInt(), 42000);

  // 1. "Open Empty" mode: does not restore
  auto audioEmpty = std::make_shared<tests::MockAudioEnginePort>();
  core::PlayerService playerEmpty(audioEmpty);
  QtBridge bridgeEmpty(playerEmpty, library, &themeLoader);
  bridgeEmpty.setStartupAction("Open Empty");
  bridgeEmpty.restorePlaybackSession();
  EXPECT_FALSE(playerEmpty.getCurrentTrack().has_value());

  // 2. "Restore Session" mode: restores in paused state
  auto audioRestore = std::make_shared<tests::MockAudioEnginePort>();
  core::PlayerService playerRestore(audioRestore);
  QtBridge bridgeRestore(playerRestore, library, &themeLoader);
  bridgeRestore.setStartupAction("Restore Session");
  bridgeRestore.restorePlaybackSession();
  ASSERT_TRUE(playerRestore.getCurrentTrack().has_value());
  EXPECT_EQ(playerRestore.getCurrentTrack()->id, "sess_1");
  EXPECT_FALSE(playerRestore.isPlaying());
  EXPECT_EQ(bridgeRestore.positionMs(), 42000);
  EXPECT_TRUE(bridgeRestore.isMuted());
  EXPECT_FLOAT_EQ(bridgeRestore.volume(), 0.0f);
  bridgeRestore.toggleMute(); // unmute restores pre-mute volume
  EXPECT_FLOAT_EQ(bridgeRestore.volume(), 0.65f);
  EXPECT_EQ(bridgeRestore.shuffleMode(), 1);
  EXPECT_EQ(bridgeRestore.repeatMode(), 2);
  EXPECT_GE(bridgeRestore.upNextCount(), 2);

  // 3. "Play Immediately" mode: restores in playing state
  auto audioPlay = std::make_shared<tests::MockAudioEnginePort>();
  core::PlayerService playerPlay(audioPlay);
  QtBridge bridgePlay(playerPlay, library, &themeLoader);
  bridgePlay.setStartupAction("Play Immediately");
  bridgePlay.restorePlaybackSession();
  ASSERT_TRUE(playerPlay.getCurrentTrack().has_value());
  EXPECT_EQ(playerPlay.getCurrentTrack()->id, "sess_1");
  EXPECT_TRUE(playerPlay.isPlaying());
}
