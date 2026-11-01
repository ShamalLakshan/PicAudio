#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <gtest/gtest.h>
#include <vector>

#include "plugin/PluginManager.hpp"
#include "theming/ThemeLoader.hpp"
#include <picaudio/plugin/Defines.hpp>

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
