#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <gtest/gtest.h>
#include <vector>

#include "plugin/PluginManager.hpp"
#include "theming/ThemeLoader.hpp"
#include <picaudio/plugin/Defines.hpp>

#include <archive.h>
#include <archive_entry.h>

class PluginManagerTest : public ::testing::Test {
protected:
  void SetUp() override {
    m_tempDir = std::make_unique<QTemporaryDir>();
    ASSERT_TRUE(m_tempDir->isValid());
  }

  void TearDown() override { m_tempDir.reset(); }

  std::unique_ptr<QTemporaryDir> m_tempDir;
};

TEST_F(PluginManagerTest, InitializesAndConfiguresPaths) {
  adapters::PluginManager manager;
  QString customPath = m_tempDir->path() + "/custom_plugins";
  QDir().mkpath(customPath);

  manager.initialize({customPath});

  QStringList paths = manager.getPluginDirectories();
  EXPECT_TRUE(paths.contains(customPath));
  EXPECT_FALSE(manager.getPrimaryPluginDirectory().isEmpty());
}

TEST_F(PluginManagerTest, RejectsInvalidManifest) {
  adapters::PluginManager manager;
  QString pluginFolder = m_tempDir->path() + "/bad_plugin";
  QDir().mkpath(pluginFolder);

  // Create manifest missing required fields
  QFile file(pluginFolder + "/plugin.json");
  ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
  file.write("{\"invalid_json\": true}");
  file.close();

  manager.initialize({m_tempDir->path()});
  manager.scanAndLoadPlugins();

  auto plugins = manager.getDiscoveredPlugins();
  EXPECT_TRUE(plugins.isEmpty());
}

TEST_F(PluginManagerTest, RejectsIncompatibleApiMajorVersion) {
  adapters::PluginManager manager;
  QString pluginFolder = m_tempDir->path() + "/future_plugin";
  QDir().mkpath(pluginFolder);

  QFile file(pluginFolder + "/plugin.json");
  ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
  file.write("{\n"
             "  \"id\": \"com.test.future\",\n"
             "  \"name\": \"Future Plugin\",\n"
             "  \"version\": \"1.0.0\",\n"
             "  \"author\": \"Test Author\",\n"
             "  \"description\": \"Incompatible API test\",\n"
             "  \"targetApiVersion\": \"99.0.0\",\n"
             "  \"capabilities\": [\"visualizer\"]\n"
             "}");
  file.close();

  manager.initialize({m_tempDir->path()});
  manager.scanAndLoadPlugins();

  auto plugins = manager.getDiscoveredPlugins();
  ASSERT_EQ(plugins.size(), 1);
  EXPECT_EQ(plugins[0].id, "com.test.future");
  EXPECT_FALSE(plugins[0].loaded);
  EXPECT_TRUE(
      plugins[0].loadError.contains("Incompatible SDK API major version"));
}

TEST_F(PluginManagerTest, DiscoversAndLoadsBuiltReferencePlugins) {
  adapters::PluginManager manager;

  // Scan cmake build plugin directories
  QString buildPluginsDir = QStringLiteral(PICAUDIO_BUILD_DIR) + "/plugins";
  manager.initialize({buildPluginsDir});
  manager.scanAndLoadPlugins();

  auto plugins = manager.getDiscoveredPlugins();
  bool foundSpectrum = false;
  bool foundLyrics = false;

  for (const auto &p : plugins) {
    if (p.id == "com.picaudio.spectrum") {
      foundSpectrum = true;
      EXPECT_TRUE(p.loaded);
      EXPECT_TRUE(p.capabilities.contains("visualizer"));
      EXPECT_EQ(p.targetApiVersion, "1.0.0");
    } else if (p.id == "com.picaudio.lyrics_scrobbler") {
      foundLyrics = true;
      EXPECT_TRUE(p.loaded);
      EXPECT_TRUE(p.capabilities.contains("lyrics"));
      EXPECT_TRUE(p.capabilities.contains("scrobbler"));
    }
  }

  EXPECT_TRUE(foundSpectrum);
  EXPECT_TRUE(foundLyrics);
}

TEST_F(PluginManagerTest, RoutesAudioBufferToVisualizer) {
  adapters::PluginManager manager;
  QString buildPluginsDir = QStringLiteral(PICAUDIO_BUILD_DIR) + "/plugins";
  manager.initialize({buildPluginsDir});
  manager.scanAndLoadPlugins();

  // Create synthetic sine wave buffer
  const size_t frames = 1024;
  std::vector<float> pcm(frames * 2, 0.5f);

  manager.routeAudioBuffer(pcm.data(), frames, 48000, 2);

  float bands[64] = {0.0f};
  size_t count = manager.getFrequencyBands(bands, 64);
  EXPECT_GT(count, 0u);

  // Verify values normalized within [0.0f, 1.0f]
  for (size_t i = 0; i < count; ++i) {
    EXPECT_GE(bands[i], 0.0f);
    EXPECT_LE(bands[i], 1.0f);
  }
}

TEST_F(PluginManagerTest, RoutesLyricsAndScrobblerEvents) {
  adapters::PluginManager manager;
  QString buildPluginsDir = QStringLiteral(PICAUDIO_BUILD_DIR) + "/plugins";
  manager.initialize({buildPluginsDir});
  manager.scanAndLoadPlugins();

  picaudio::plugin::PicAudioTrackInfo track{};
  track.id = "track_123";
  track.title = "Sample Song";
  track.artist = "Sample Artist";
  track.album = "Sample Album";
  track.filePath = "/tmp/non_existent_audio.flac";
  track.durationMs = 210000;

  manager.routeTrackStarted(track);
  manager.routePlaybackStateChanged(
      picaudio::plugin::PicAudioPlaybackState::Playing);

  // Fetch fallback lyrics
  std::string lyrics = manager.routeFetchLyrics("Sample Artist", "Sample Song");
  EXPECT_FALSE(lyrics.empty());
  EXPECT_NE(lyrics.find("Sample Song"), std::string::npos);

  // Dispatch scrobble event
  EXPECT_NO_THROW({
    manager.routeTrackScrobbled(track, 1700000000ULL);
    manager.routeTrackIndexed(track);
  });
}

TEST_F(PluginManagerTest, TogglesPluginEnabledState) {
  adapters::PluginManager manager;
  QString buildPluginsDir = QStringLiteral(PICAUDIO_BUILD_DIR) + "/plugins";
  manager.initialize({buildPluginsDir});
  manager.scanAndLoadPlugins();

  EXPECT_TRUE(manager.setPluginEnabled("com.picaudio.spectrum", false));

  auto plugins = manager.getDiscoveredPlugins();
  for (const auto &p : plugins) {
    if (p.id == "com.picaudio.spectrum") {
      EXPECT_FALSE(p.enabled);
      EXPECT_FALSE(p.loaded);
    }
  }

  // While disabled, visualizer returns 0 bands
  float bands[32] = {0.0f};
  size_t count = manager.getFrequencyBands(bands, 32);
  EXPECT_EQ(count, 0u);

  // Re-enable
  EXPECT_TRUE(manager.setPluginEnabled("com.picaudio.spectrum", true));
  plugins = manager.getDiscoveredPlugins();
  for (const auto &p : plugins) {
    if (p.id == "com.picaudio.spectrum") {
      EXPECT_TRUE(p.enabled);
      EXPECT_TRUE(p.loaded);
    }
  }
}

TEST_F(PluginManagerTest, AppliesDynamicThemeOverrides) {
  adapters::ThemeLoader themeLoader;
  adapters::PluginManager manager;

  manager.setThemeLoader(&themeLoader);

  // Initial accent color
  QString initialAccent = themeLoader.accent();

  // Apply custom color overrides (simulating Pywal or dynamic theme plugin)
  themeLoader.setCustomColorToken("accent", "#e06c75");
  themeLoader.setCustomColorToken("background", "#1e1e1e");

  EXPECT_EQ(themeLoader.accent(), "#e06c75");
  EXPECT_EQ(themeLoader.background(), "#1e1e1e");
  EXPECT_NE(themeLoader.accent(), initialAccent);
}

TEST_F(PluginManagerTest, ParsesManifestCustomActions) {
  adapters::PluginManager manager;
  QString pluginFolder = m_tempDir->path() + "/custom_action_plugin";
  QDir().mkpath(pluginFolder);

  QFile file(pluginFolder + "/plugin.json");
  ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
  file.write("{\n"
             "  \"id\": \"com.test.actions\",\n"
             "  \"name\": \"Action Test Plugin\",\n"
             "  \"version\": \"1.0.0\",\n"
             "  \"author\": \"Test Author\",\n"
             "  \"description\": \"Action parsing test\",\n"
             "  \"targetApiVersion\": \"1.0.0\",\n"
             "  \"capabilities\": [\"ui\"],\n"
             "  \"actions\": [\n"
             "    {\n"
             "      \"id\": \"act_popup\",\n"
             "      \"label\": \"Show Dialog\",\n"
             "      \"tooltip\": \"Display popup dialog\",\n"
             "      \"iconName\": \"settings\",\n"
             "      \"isPrimary\": true\n"
             "    }\n"
             "  ]\n"
             "}");
  file.close();

  manager.initialize({m_tempDir->path()});
  manager.scanAndLoadPlugins();

  auto plugins = manager.getDiscoveredPlugins();
  ASSERT_EQ(plugins.size(), 1);
  EXPECT_EQ(plugins[0].id, "com.test.actions");
  ASSERT_EQ(plugins[0].customActions.size(), 1);
  EXPECT_EQ(plugins[0].customActions[0].id, "act_popup");
  EXPECT_EQ(plugins[0].customActions[0].label, "Show Dialog");
  EXPECT_EQ(plugins[0].customActions[0].tooltip, "Display popup dialog");
  EXPECT_EQ(plugins[0].customActions[0].iconName, "settings");
  EXPECT_TRUE(plugins[0].customActions[0].isPrimary);
}

TEST_F(PluginManagerTest, RoutesUiCustomActionsAndWindowLifecycle) {
  adapters::PluginManager manager;
  QString buildPluginsDir = QStringLiteral(PICAUDIO_BUILD_DIR) + "/plugins";
  manager.initialize({buildPluginsDir});

  int navigatedView = -1;
  manager.setNavigateCallback(
      [&navigatedView](int view) { navigatedView = view; });

  QString toastMsg;
  QString toastLvl;
  manager.setToastCallback(
      [&toastMsg, &toastLvl](const QString &msg, const QString &lvl) {
        toastMsg = msg;
        toastLvl = lvl;
      });

  manager.scanAndLoadPlugins();

  auto plugins = manager.getDiscoveredPlugins();
  bool spectrumFound = false;
  bool lyricsFound = false;

  for (const auto &p : plugins) {
    if (p.id == "com.picaudio.spectrum") {
      spectrumFound = true;
      EXPECT_TRUE(p.loaded);
      EXPECT_TRUE(p.capabilities.contains("ui"));
      bool hasOpenVisualizerAction = false;
      for (const auto &act : p.customActions) {
        if (act.id == "open_visualizer") {
          hasOpenVisualizerAction = true;
          EXPECT_EQ(act.label, "Open Visualizer View");
          EXPECT_TRUE(act.isPrimary);
        }
      }
      EXPECT_TRUE(hasOpenVisualizerAction);
    } else if (p.id == "com.picaudio.lyrics_scrobbler") {
      lyricsFound = true;
      EXPECT_TRUE(p.loaded);
      EXPECT_TRUE(p.capabilities.contains("ui"));
      bool hasScrobbleAction = false;
      for (const auto &act : p.customActions) {
        if (act.id == "scrobble_status") {
          hasScrobbleAction = true;
          EXPECT_EQ(act.label, "Scrobble Status");
        }
      }
      EXPECT_TRUE(hasScrobbleAction);
    }
  }

  ASSERT_TRUE(spectrumFound);
  ASSERT_TRUE(lyricsFound);

  // Trigger spectrum visualizer custom action -> navigate to view 4
  EXPECT_TRUE(
      manager.triggerPluginAction("com.picaudio.spectrum", "open_visualizer"));
  EXPECT_EQ(navigatedView, 4);

  // Trigger lyrics scrobbler custom action -> toast notification
  EXPECT_TRUE(manager.triggerPluginAction("com.picaudio.lyrics_scrobbler",
                                          "scrobble_status"));
  EXPECT_TRUE(toastMsg.contains("Scrobbler queue"));
  EXPECT_EQ(toastLvl, "info");

  // Test popup window toggle on lyrics scrobbler plugin
  EXPECT_FALSE(manager.isPluginWindowVisible("com.picaudio.lyrics_scrobbler"));
  EXPECT_TRUE(manager.showPluginWindow("com.picaudio.lyrics_scrobbler"));
  EXPECT_TRUE(manager.isPluginWindowVisible("com.picaudio.lyrics_scrobbler"));
  EXPECT_TRUE(manager.hidePluginWindow("com.picaudio.lyrics_scrobbler"));
  EXPECT_FALSE(manager.isPluginWindowVisible("com.picaudio.lyrics_scrobbler"));
}

namespace {
void createTestZip(const QString &zipPath,
                   const std::vector<std::pair<QString, QByteArray>> &entries) {
  archive *a = archive_write_new();
  archive_write_set_format_zip(a);
  archive_write_open_filename(a, zipPath.toUtf8().constData());
  for (const auto &[name, data] : entries) {
    archive_entry *entry = archive_entry_new();
    archive_entry_set_pathname(entry, name.toUtf8().constData());
    archive_entry_set_size(entry, data.size());
    archive_entry_set_filetype(entry, AE_IFREG);
    archive_entry_set_perm(entry, 0644);
    archive_write_header(a, entry);
    archive_write_data(a, data.constData(), data.size());
    archive_entry_free(entry);
  }
  archive_write_close(a);
  archive_write_free(a);
}
} // namespace

TEST_F(PluginManagerTest, ReturnsCurrentPlatformTag) {
  const QString platform = adapters::PluginInstaller::currentPlatformTag();
  EXPECT_FALSE(platform.isEmpty());
  EXPECT_TRUE(platform.contains("x64") || platform.contains("arm64"));
}

TEST_F(PluginManagerTest, RejectsCorruptBundleArchive) {
  adapters::PluginInstallResult result;
  // Non-existent file
  EXPECT_FALSE(adapters::PluginInstaller::installBundle(
      "/non/existent/path/plugin.paplugin", m_tempDir->path(), result));
  EXPECT_EQ(result.status, adapters::PluginInstallStatus::FileNotFound);

  // Corrupt non-zip file
  const QString badFile = m_tempDir->path() + "/corrupt.paplugin";
  QFile f(badFile);
  ASSERT_TRUE(f.open(QIODevice::WriteOnly));
  f.write("This is not a zip archive");
  f.close();

  EXPECT_FALSE(adapters::PluginInstaller::installBundle(
      badFile, m_tempDir->path(), result));
  EXPECT_EQ(result.status, adapters::PluginInstallStatus::InvalidArchive);
}

TEST_F(PluginManagerTest, RejectsBundleWithMaliciousPathTraversal) {
  const QString zipPath = m_tempDir->path() + "/malicious.paplugin";
  createTestZip(zipPath, {{"../escape.txt", "Malicious content"}});

  adapters::PluginInstallResult result;
  EXPECT_FALSE(adapters::PluginInstaller::installBundle(
      zipPath, m_tempDir->path(), result));
  EXPECT_EQ(result.status, adapters::PluginInstallStatus::InvalidArchive);
  EXPECT_TRUE(result.message.contains("Malicious or unsafe path"));
}

TEST_F(PluginManagerTest,
       InstallsValidPluginBundleAndExtractsMatchingPlatform) {
  const QString platform = adapters::PluginInstaller::currentPlatformTag();
  const QString zipPath = m_tempDir->path() + "/sample.paplugin";
  const QString destDir = m_tempDir->path() + "/installed_plugins";

  const QByteArray manifest = "{\n"
                              "  \"id\": \"com.test.samplebundle\",\n"
                              "  \"name\": \"Sample Bundle Plugin\",\n"
                              "  \"version\": \"1.0.0\",\n"
                              "  \"author\": \"Test Suite\",\n"
                              "  \"targetApiVersion\": \"1.0.0\",\n"
                              "  \"entryPoint\": \"sample.so\",\n"
                              "  \"capabilities\": [\"visualizer\"]\n"
                              "}";

  createTestZip(
      zipPath,
      {{"plugin.json", manifest},
       {platform + "/sample.so", QByteArray("MOCK_BINARY_DATA_FOR_PLATFORM")},
       {"windows-x64/sample.dll", QByteArray("MOCK_WINDOWS_BINARY")},
       {"README.md", QByteArray("Plugin Readme")}});

  adapters::PluginInstallResult result;
  EXPECT_TRUE(
      adapters::PluginInstaller::installBundle(zipPath, destDir, result));
  EXPECT_TRUE(result.isSuccess());
  EXPECT_EQ(result.pluginId, "com.test.samplebundle");
  EXPECT_EQ(result.pluginName, "Sample Bundle Plugin");
  EXPECT_EQ(result.version, "1.0.0");

  const QString pluginDir = destDir + "/com.test.samplebundle";
  EXPECT_TRUE(QFile::exists(pluginDir + "/plugin.json"));
  EXPECT_TRUE(QFile::exists(pluginDir + "/sample.so"));
  EXPECT_TRUE(QFile::exists(pluginDir + "/README.md"));
  // Incompatible platform binary must NOT be extracted
  EXPECT_FALSE(QFile::exists(pluginDir + "/sample.dll"));
}

TEST_F(PluginManagerTest, RemovesPluginAndCleansStagedPendingRemovals) {
  const QString pluginsDir = m_tempDir->path() + "/removal_test_plugins";
  const QString pluginId = "com.test.removable";
  const QString targetFolder = pluginsDir + "/" + pluginId;
  QDir().mkpath(targetFolder);

  QFile f(targetFolder + "/plugin.json");
  ASSERT_TRUE(f.open(QIODevice::WriteOnly));
  f.write("{}");
  f.close();

  EXPECT_TRUE(QDir(targetFolder).exists());

  QString err;
  EXPECT_TRUE(
      adapters::PluginInstaller::removePlugin(pluginId, targetFolder, err));
  EXPECT_FALSE(QDir(targetFolder).exists());
  EXPECT_FALSE(adapters::PluginInstaller::isPendingRemoval(pluginId));

  // Test staging removal
  const QString stagedFolder = pluginsDir + "/com.test.staged";
  QDir().mkpath(stagedFolder);
  QSettings settings(QStringLiteral("PicAudio"), QStringLiteral("PicAudio"));
  settings.setValue(QStringLiteral("plugins/pendingRemoval"),
                    QStringList{"com.test.staged"});

  EXPECT_TRUE(adapters::PluginInstaller::isPendingRemoval("com.test.staged"));
  const int processed =
      adapters::PluginInstaller::processPendingRemovals(pluginsDir);
  EXPECT_EQ(processed, 1);
  EXPECT_FALSE(QDir(stagedFolder).exists());
  EXPECT_FALSE(adapters::PluginInstaller::isPendingRemoval("com.test.staged"));
}

TEST_F(PluginManagerTest, PluginManagerInstallAndRemoveLifecycle) {
  adapters::PluginManager manager;
  const QString customPluginsDir = m_tempDir->path() + "/mgr_plugins";
  QDir().mkpath(customPluginsDir);
  manager.initialize({customPluginsDir});

  const QString platform = adapters::PluginInstaller::currentPlatformTag();
  const QString zipPath = m_tempDir->path() + "/lifecycle.paplugin";

  const QByteArray manifest = "{\n"
                              "  \"id\": \"com.test.lifecycle\",\n"
                              "  \"name\": \"Lifecycle Plugin\",\n"
                              "  \"version\": \"2.1.0\",\n"
                              "  \"author\": \"QA\",\n"
                              "  \"targetApiVersion\": \"1.0.0\",\n"
                              "  \"entryPoint\": \"plugin.so\",\n"
                              "  \"capabilities\": [\"lyrics\"]\n"
                              "}";

  createTestZip(zipPath,
                {{"plugin.json", manifest},
                 {platform + "/plugin.so", QByteArray("MOCK_DYNLIB_BYTES")}});

  adapters::PluginInstallResult result;
  EXPECT_TRUE(manager.installPluginBundle(zipPath, result));
  EXPECT_TRUE(result.isSuccess());

  auto plugins = manager.getDiscoveredPlugins();
  bool found = false;
  for (const auto &p : plugins) {
    if (p.id == "com.test.lifecycle") {
      found = true;
      EXPECT_EQ(p.version, "2.1.0");
      EXPECT_EQ(p.name, "Lifecycle Plugin");
    }
  }
  EXPECT_TRUE(found);

  // Now remove plugin
  QString err;
  EXPECT_TRUE(manager.removePlugin("com.test.lifecycle", err));
  plugins = manager.getDiscoveredPlugins();
  for (const auto &p : plugins) {
    EXPECT_NE(p.id, "com.test.lifecycle");
  }
}

TEST_F(PluginManagerTest, ReentrantPluginsChangedDoesNotDeadlock) {
  adapters::PluginManager manager;
  const QString customPluginsDir = m_tempDir->path() + "/reentrant_plugins";
  QDir().mkpath(customPluginsDir);
  manager.initialize({customPluginsDir});

  int reentrantQueries = 0;
  QObject::connect(&manager, &adapters::PluginManager::pluginsChanged, [&]() {
    // Re-entrant call on the same thread inside pluginsChanged emission
    auto list = manager.getDiscoveredPlugins();
    (void)list;
    ++reentrantQueries;
  });

  const QString platform = adapters::PluginInstaller::currentPlatformTag();
  const QString zipPath = m_tempDir->path() + "/reentrant.paplugin";

  const QByteArray manifest = "{\n"
                              "  \"id\": \"com.test.reentrant\",\n"
                              "  \"name\": \"Reentrant Test Plugin\",\n"
                              "  \"version\": \"1.0.0\",\n"
                              "  \"author\": \"QA\",\n"
                              "  \"targetApiVersion\": \"1.0.0\",\n"
                              "  \"capabilities\": [\"visualizer\"]\n"
                              "}";

  createTestZip(zipPath, {{"plugin.json", manifest},
                          {platform + "/plugin.so", QByteArray("MOCK_BYTES")}});

  adapters::PluginInstallResult result;
  EXPECT_TRUE(manager.installPluginBundle(zipPath, result));
  EXPECT_GT(reentrantQueries, 0);

  EXPECT_TRUE(manager.setPluginEnabled("com.test.reentrant", false));
  EXPECT_GT(reentrantQueries, 1);

  QString err;
  EXPECT_TRUE(manager.removePlugin("com.test.reentrant", err));
  EXPECT_GT(reentrantQueries, 2);
}
