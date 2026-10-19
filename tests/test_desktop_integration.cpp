#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QTemporaryDir>
#include <QUrl>

#include "QtBridge.hpp"
#include "desktop/DesktopNotificationAdapter.hpp"
#include "desktop/DesktopTrayAdapter.hpp"
#include "desktop/MprisAdapter.hpp"
#include "desktop/SingleInstanceManager.hpp"

namespace {

void ensureGuiApp() {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  int argc = 1;
  static char appName[] = "picaudio_tests";
  static char *argv[] = {appName, nullptr};
  if (!QCoreApplication::instance()) {
    new QGuiApplication(argc, argv);
  }
}

TEST(SingleInstanceManagerTest, ParseRemotePlaybackCommands) {
  ensureGuiApp();
  adapters::CliCommand cmdPlay =
      adapters::SingleInstanceManager::parseArguments(
          {QStringLiteral("--play")});
  EXPECT_EQ(cmdPlay.type, adapters::CliCommand::Type::Play);

  adapters::CliCommand cmdPause =
      adapters::SingleInstanceManager::parseArguments(
          {QStringLiteral("--pause")});
  EXPECT_EQ(cmdPause.type, adapters::CliCommand::Type::Pause);

  adapters::CliCommand cmdToggle =
      adapters::SingleInstanceManager::parseArguments(
          {QStringLiteral("--toggle")});
  EXPECT_EQ(cmdToggle.type, adapters::CliCommand::Type::Toggle);

  adapters::CliCommand cmdStop =
      adapters::SingleInstanceManager::parseArguments(
          {QStringLiteral("--stop")});
  EXPECT_EQ(cmdStop.type, adapters::CliCommand::Type::Stop);

  adapters::CliCommand cmdNext =
      adapters::SingleInstanceManager::parseArguments(
          {QStringLiteral("--next")});
  EXPECT_EQ(cmdNext.type, adapters::CliCommand::Type::Next);

  adapters::CliCommand cmdPrev =
      adapters::SingleInstanceManager::parseArguments(
          {QStringLiteral("--prev")});
  EXPECT_EQ(cmdPrev.type, adapters::CliCommand::Type::Previous);

  adapters::CliCommand cmdRaise =
      adapters::SingleInstanceManager::parseArguments(
          {QStringLiteral("--raise")});
  EXPECT_EQ(cmdRaise.type, adapters::CliCommand::Type::RaiseWindow);
}

TEST(SingleInstanceManagerTest, ParseVolumeArgument) {
  adapters::CliCommand cmd = adapters::SingleInstanceManager::parseArguments(
      {QStringLiteral("--volume"), QStringLiteral("75")});
  EXPECT_EQ(cmd.type, adapters::CliCommand::Type::SetVolume);
  EXPECT_FLOAT_EQ(cmd.volume, 0.75f);

  adapters::CliCommand cmdZero =
      adapters::SingleInstanceManager::parseArguments(
          {QStringLiteral("--volume"), QStringLiteral("0")});
  EXPECT_EQ(cmdZero.type, adapters::CliCommand::Type::SetVolume);
  EXPECT_FLOAT_EQ(cmdZero.volume, 0.0f);

  adapters::CliCommand cmdMax = adapters::SingleInstanceManager::parseArguments(
      {QStringLiteral("--volume"), QStringLiteral("120")}); // clamped to 100
  EXPECT_EQ(cmdMax.type, adapters::CliCommand::Type::SetVolume);
  EXPECT_FLOAT_EQ(cmdMax.volume, 1.0f);
}

TEST(SingleInstanceManagerTest, ParseFileAndEnqueueArguments) {
  QTemporaryDir tempDir;
  ASSERT_TRUE(tempDir.isValid());

  QString testFile = tempDir.filePath(QStringLiteral("song.flac"));
  QFile file(testFile);
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  file.write("dummy");
  file.close();

  // Play file directly
  adapters::CliCommand cmdPlay =
      adapters::SingleInstanceManager::parseArguments({testFile},
                                                      tempDir.path());
  EXPECT_EQ(cmdPlay.type, adapters::CliCommand::Type::PlayFiles);
  ASSERT_EQ(cmdPlay.paths.size(), 1);
  EXPECT_EQ(cmdPlay.paths[0], testFile);

  // Enqueue file
  adapters::CliCommand cmdEnq = adapters::SingleInstanceManager::parseArguments(
      {QStringLiteral("--enqueue"), testFile}, tempDir.path());
  EXPECT_EQ(cmdEnq.type, adapters::CliCommand::Type::EnqueueFiles);
  ASSERT_EQ(cmdEnq.paths.size(), 1);
  EXPECT_EQ(cmdEnq.paths[0], testFile);
}

TEST(SingleInstanceManagerTest, SocketServerNameIsNonEmpty) {
  QString name = adapters::SingleInstanceManager::socketServerName();
  EXPECT_FALSE(name.isEmpty());
  EXPECT_TRUE(name.startsWith(QStringLiteral("PicAudio_SingleInstance_")));
}

TEST(DesktopTrayAdapterTest, FormattedTooltipOutput) {
  adapters::DesktopTrayAdapter tray;
  // Initially stopped
  EXPECT_EQ(tray.formattedTooltip(), QStringLiteral("PicAudio\nStopped"));

  // Track playing
  tray.updateTrackInfo(QStringLiteral("Subdivisions"), QStringLiteral("Rush"),
                       QStringLiteral("05:35"));
  tray.updatePosition(QStringLiteral("01:23"));
  tray.updatePlaybackState(true);

  QString expected =
      QStringLiteral("PicAudio\nRush - Subdivisions\n[01:23 / 05:35]");
  EXPECT_EQ(tray.formattedTooltip(), expected);
}

TEST(DesktopNotificationAdapterTest, SuppressionLogic) {
  adapters::DesktopNotificationAdapter notif;

  // Defaults: enabled, suppressWhenFocused = true
  EXPECT_TRUE(notif.isEnabled());
  EXPECT_TRUE(notif.suppressWhenFocused());
  EXPECT_FALSE(notif.shouldSuppressNotification());

  // Window focused -> should suppress
  notif.setWindowFocused(true);
  EXPECT_TRUE(notif.shouldSuppressNotification());

  // Window loses focus -> should not suppress
  notif.setWindowFocused(false);
  EXPECT_FALSE(notif.shouldSuppressNotification());

  // DND active -> should suppress
  notif.setDndActive(true);
  EXPECT_TRUE(notif.shouldSuppressNotification());
  notif.setDndActive(false);

  // Disabled in preferences -> should suppress
  notif.setEnabled(false);
  EXPECT_TRUE(notif.shouldSuppressNotification());
}

TEST(DesktopNotificationAdapterTest, DurationClamping) {
  adapters::DesktopNotificationAdapter notif;

  notif.setDurationSec(1); // Below min 2
  EXPECT_EQ(notif.durationSec(), 2);

  notif.setDurationSec(15); // Above max 10
  EXPECT_EQ(notif.durationSec(), 10);

  notif.setDurationSec(5);
  EXPECT_EQ(notif.durationSec(), 5);
}

TEST(MprisAdapterTest, MetadataAndPropertiesFormatting) {
  adapters::MprisAdapter mpris;

  EXPECT_EQ(mpris.playbackStatus(), QStringLiteral("Stopped"));

  mpris.updatePlaybackState(true, 12000);
  EXPECT_EQ(mpris.playbackStatus(), QStringLiteral("Playing"));

  mpris.updatePlaybackState(false, 12000);
  EXPECT_EQ(mpris.playbackStatus(), QStringLiteral("Paused"));

  mpris.updateModes(1, 1); // Repeat playlist, Shuffle on
  EXPECT_EQ(mpris.loopStatus(), QStringLiteral("Playlist"));
  EXPECT_TRUE(mpris.shuffle());

  mpris.updateModes(2, 0); // Repeat track, Shuffle off
  EXPECT_EQ(mpris.loopStatus(), QStringLiteral("Track"));
  EXPECT_FALSE(mpris.shuffle());

  mpris.updateMetadata(QStringLiteral("Tom Sawyer"), QStringLiteral("Rush"),
                       QStringLiteral("Moving Pictures"),
                       QStringLiteral("/path/cover.jpg"), 274000, 1,
                       QStringLiteral("Progressive Rock"),
                       QStringLiteral("/music/rush/01.flac"));

  QVariantMap meta = mpris.metadata();
  EXPECT_EQ(meta.value(QStringLiteral("xesam:title")).toString(),
            QStringLiteral("Tom Sawyer"));
  EXPECT_EQ(meta.value(QStringLiteral("xesam:album")).toString(),
            QStringLiteral("Moving Pictures"));
  EXPECT_EQ(meta.value(QStringLiteral("xesam:artist")).toStringList(),
            QStringList{QStringLiteral("Rush")});
  EXPECT_EQ(meta.value(QStringLiteral("xesam:trackNumber")).toInt(), 1);
  // Length in microseconds: 274000 ms * 1000 = 274000000 us
  EXPECT_EQ(meta.value(QStringLiteral("mpris:length")).toLongLong(),
            274000000LL);
}

TEST(DragAndDropTest, ExtractAudioFilesFiltersNonAudio) {
  QTemporaryDir tempDir;
  ASSERT_TRUE(tempDir.isValid());

  QString audioFile = tempDir.filePath(QStringLiteral("test.flac"));
  QFile f1(audioFile);
  ASSERT_TRUE(f1.open(QIODevice::WriteOnly));
  f1.write("flac");
  f1.close();

  QString txtFile = tempDir.filePath(QStringLiteral("notes.txt"));
  QFile f2(txtFile);
  ASSERT_TRUE(f2.open(QIODevice::WriteOnly));
  f2.write("notes");
  f2.close();

  QList<QUrl> urls{QUrl::fromLocalFile(audioFile),
                   QUrl::fromLocalFile(txtFile)};
  QStringList audioFiles = QtBridge::extractAudioFilesFromUrls(urls);

  EXPECT_EQ(audioFiles.size(), 1);
  EXPECT_EQ(audioFiles[0], audioFile);
}

} // namespace
