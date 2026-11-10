#include "core/services/PlaylistService.hpp"
#include "metadata/TagLibMetadataAdapter.hpp"
#include "models/AlbumListModel.hpp"
#include "models/TrackListModel.hpp"
#include "storage/SqliteDatabaseAdapter.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace fs = std::filesystem;

class Phase4LibraryTest : public ::testing::Test {
protected:
  std::string testDir;
  std::string dbPath;

  void SetUp() override {
    testDir = (fs::temp_directory_path() / "picaudio_phase4_test").string();
    fs::create_directories(testDir);
    dbPath = (fs::path(testDir) / "test_picaudio.db").string();
  }

  void TearDown() override {
    std::error_code ec;
    fs::remove_all(testDir, ec);
  }
};

TEST_F(Phase4LibraryTest, PlaylistService_M3U_And_M3U8_ImportExport) {
  core::StaticPlaylist pl;
  pl.id = "pl-1";
  pl.name = "Rock Classics";

  core::PlaylistItem item1;
  item1.filePath = (fs::path(testDir) / "track1.mp3").string();
  item1.title = "Bohemian Rhapsody";
  item1.artist = "Queen";
  item1.durationMs = 354000;
  pl.items.push_back(item1);

  core::PlaylistItem item2;
  item2.filePath = (fs::path(testDir) / "track2.flac").string();
  item2.title = "Hotel California";
  item2.artist = "Eagles";
  item2.durationMs = 391000;
  pl.items.push_back(item2);

  std::string m3u8Path = (fs::path(testDir) / "test.m3u8").string();
  EXPECT_TRUE(core::PlaylistService::exportPlaylist(
      pl, m3u8Path, core::PlaylistFormat::M3U8, false));
  EXPECT_TRUE(fs::exists(m3u8Path));

  auto importedOpt = core::PlaylistService::importPlaylist(m3u8Path);
  ASSERT_TRUE(importedOpt.has_value());
  EXPECT_EQ(importedOpt->items.size(), 2);
  EXPECT_EQ(importedOpt->items[0].title, "Bohemian Rhapsody");
  EXPECT_EQ(importedOpt->items[0].artist, "Queen");
  EXPECT_EQ(importedOpt->items[1].title, "Hotel California");

  // Test moveItem
  EXPECT_TRUE(core::PlaylistService::moveItem(*importedOpt, 0, 1));
  EXPECT_EQ(importedOpt->items[0].title, "Hotel California");
  EXPECT_EQ(importedOpt->items[1].title, "Bohemian Rhapsody");
}

TEST_F(Phase4LibraryTest, PlaylistService_PLS_ImportExport) {
  core::StaticPlaylist pl;
  pl.name = "PLS Playlist";

  core::PlaylistItem item;
  item.filePath = (fs::path(testDir) / "song.mp3").string();
  item.title = "Stairway to Heaven";
  item.artist = "Led Zeppelin";
  item.durationMs = 482000;
  pl.items.push_back(item);

  std::string plsPath = (fs::path(testDir) / "test.pls").string();
  EXPECT_TRUE(core::PlaylistService::exportPlaylist(
      pl, plsPath, core::PlaylistFormat::PLS, false));
  EXPECT_TRUE(fs::exists(plsPath));

  auto importedOpt = core::PlaylistService::importPlaylist(plsPath);
  ASSERT_TRUE(importedOpt.has_value());
  ASSERT_EQ(importedOpt->items.size(), 1);
  EXPECT_EQ(importedOpt->items[0].title, "Stairway to Heaven");
  EXPECT_EQ(importedOpt->items[0].artist, "Led Zeppelin");
  EXPECT_EQ(importedOpt->items[0].durationMs, 482000);
}

TEST_F(Phase4LibraryTest, PlaylistService_XSPF_ImportExport) {
  core::StaticPlaylist pl;
  pl.name = "XSPF Playlist";

  core::PlaylistItem item;
  item.filePath = (fs::path(testDir) / "track.wav").string();
  item.title = "Comfortably Numb";
  item.artist = "Pink Floyd";
  item.album = "The Wall";
  item.durationMs = 382000;
  pl.items.push_back(item);

  std::string xspfPath = (fs::path(testDir) / "test.xspf").string();
  EXPECT_TRUE(core::PlaylistService::exportPlaylist(
      pl, xspfPath, core::PlaylistFormat::XSPF, false));
  EXPECT_TRUE(fs::exists(xspfPath));

  auto importedOpt = core::PlaylistService::importPlaylist(xspfPath);
  ASSERT_TRUE(importedOpt.has_value());
  ASSERT_EQ(importedOpt->items.size(), 1);
  EXPECT_EQ(importedOpt->items[0].title, "Comfortably Numb");
  EXPECT_EQ(importedOpt->items[0].artist, "Pink Floyd");
  EXPECT_EQ(importedOpt->items[0].durationMs, 382000);
}

TEST_F(Phase4LibraryTest, Database_RatingsAndFavoritesPersistence) {
  adapters::SqliteDatabaseAdapter db;
  ASSERT_TRUE(db.initialize(dbPath));

  core::Track track;
  track.id = "t1";
  track.title = "Time";
  track.artist = "Pink Floyd";
  track.album = "The Dark Side of the Moon";
  track.filePath = (fs::path(testDir) / "time.flac").string();
  track.rating = 0;
  track.isFavorite = false;
  track.playCount = 0;
  track.skipCount = 0;

  ASSERT_TRUE(db.saveTrack(track));

  // Update rating
  EXPECT_TRUE(db.updateTrackRating("t1", 5));
  auto opt = db.getTrackById("t1");
  ASSERT_TRUE(opt.has_value());
  EXPECT_EQ(opt->rating, 5);

  // Toggle favorite
  EXPECT_TRUE(db.updateTrackFavorite("t1", true));
  opt = db.getTrackById("t1");
  ASSERT_TRUE(opt.has_value());
  EXPECT_TRUE(opt->isFavorite);

  // Record played
  uint64_t now = 1700000000;
  EXPECT_TRUE(db.recordTrackPlayed("t1", now));
  opt = db.getTrackById("t1");
  ASSERT_TRUE(opt.has_value());
  EXPECT_EQ(opt->playCount, 1);
  EXPECT_EQ(opt->lastPlayed, now);

  // Record skipped
  EXPECT_TRUE(db.recordTrackSkipped("t1"));
  opt = db.getTrackById("t1");
  ASSERT_TRUE(opt.has_value());
  EXPECT_EQ(opt->skipCount, 1);

  // Listening History
  auto history = db.getListeningHistory(10);
  ASSERT_FALSE(history.empty());
  EXPECT_EQ(history[0].first.id, "t1");
  EXPECT_EQ(history[0].second, now);

  // Clear history
  EXPECT_TRUE(db.clearListeningHistory());
  history = db.getListeningHistory(10);
  EXPECT_TRUE(history.empty());
}

TEST_F(Phase4LibraryTest, Database_MissingFilesAndRelocation) {
  adapters::SqliteDatabaseAdapter db;
  ASSERT_TRUE(db.initialize(dbPath));

  // Create an existing file
  std::string realFile = (fs::path(testDir) / "existing.mp3").string();
  std::ofstream(realFile) << "dummy";

  // Non-existent file path
  std::string missingFile = "/non/existent/drive/music/missing.mp3";

  core::Track t1;
  t1.id = "real_track";
  t1.title = "Existing Song";
  t1.filePath = realFile;
  db.saveTrack(t1);

  core::Track t2;
  t2.id = "missing_track";
  t2.title = "Missing Song";
  t2.filePath = missingFile;
  db.saveTrack(t2);

  auto missing = db.getMissingTracks();
  ASSERT_EQ(missing.size(), 1);
  EXPECT_EQ(missing[0].id, "missing_track");

  // Create relocation target on disk
  std::string newTargetDir =
      (fs::path(testDir) / "new_drive" / "music").string();
  fs::create_directories(newTargetDir);
  std::string relocatedRealFile =
      (fs::path(newTargetDir) / "missing.mp3").string();
  std::ofstream(relocatedRealFile) << "relocated dummy";

  // Relocate from old prefix "/non/existent/drive/music" to newTargetDir
  size_t relocatedCount =
      db.relocateTracks("/non/existent/drive/music", newTargetDir);
  EXPECT_EQ(relocatedCount, 1);

  auto updatedOpt = db.getTrackById("missing_track");
  ASSERT_TRUE(updatedOpt.has_value());
  EXPECT_EQ(updatedOpt->filePath, relocatedRealFile);

  // After relocation, missing tracks should now be 0
  EXPECT_EQ(db.getMissingTracks().size(), 0);
}

TEST_F(Phase4LibraryTest, Database_DynamicQueryForSmartPlaylists) {
  adapters::SqliteDatabaseAdapter db;
  ASSERT_TRUE(db.initialize(dbPath));

  core::Track t1;
  t1.id = "t1";
  t1.filePath = (fs::path(testDir) / "track1.flac").string();
  t1.title = "A";
  t1.rating = 5;
  t1.playCount = 10;
  t1.bitDepth = 24;
  t1.sampleRate = 96000;
  ASSERT_TRUE(db.saveTrack(t1));

  core::Track t2;
  t2.id = "t2";
  t2.filePath = (fs::path(testDir) / "track2.mp3").string();
  t2.title = "B";
  t2.rating = 2;
  t2.playCount = 1;
  t2.bitDepth = 16;
  t2.sampleRate = 44100;
  ASSERT_TRUE(db.saveTrack(t2));

  // Query top rated
  auto topRated = db.queryTracksDynamic("rating >= 4", "rating DESC", 100);
  ASSERT_EQ(topRated.size(), 1);
  EXPECT_EQ(topRated[0].id, "t1");

  // Query Hi-Res Audio
  auto hiRes = db.queryTracksDynamic("bit_depth >= 24 OR sample_rate >= 88200",
                                     "sample_rate DESC", 100);
  ASSERT_EQ(hiRes.size(), 1);
  EXPECT_EQ(hiRes[0].id, "t1");
}

TEST_F(Phase4LibraryTest, TrackListModel_MultiFacetedFilteringAndHiRes) {
  adapters::SqliteDatabaseAdapter db;
  ASSERT_TRUE(db.initialize(dbPath));

  core::Track t1;
  t1.id = "trk1";
  t1.filePath = "/music/01_song.flac";
  t1.title = "1999 Party";
  t1.artist = "Prince";
  t1.album = "1999";
  t1.genre = "Pop";
  t1.bitDepth = 24;
  t1.sampleRate = 96000;
  ASSERT_TRUE(db.saveTrack(t1));

  core::Track t2;
  t2.id = "trk2";
  t2.filePath = "/music/bohemian.flac";
  t2.title = "Bohemian Rhapsody";
  t2.artist = "Queen";
  t2.album = "A Night at the Opera";
  t2.genre = "Rock";
  t2.bitDepth = 16;
  t2.sampleRate = 44100;
  ASSERT_TRUE(db.saveTrack(t2));

  core::Track t3;
  t3.id = "trk3";
  t3.filePath = "/music/another.flac";
  t3.title = "Another One Bites the Dust";
  t3.artist = "Queen";
  t3.album = "The Game";
  t3.genre = "Rock";
  t3.bitDepth = 24;
  t3.sampleRate = 48000;
  ASSERT_TRUE(db.saveTrack(t3));

  TrackListModel model(&db);
  EXPECT_EQ(model.rowCount(), 3);

  // 1. Hi-Res only filter
  model.setHiResOnly(true);
  EXPECT_EQ(model.rowCount(), 2); // t1 (24/96) and t3 (24/48)

  // 2. Combine with artist filter
  model.setArtistFilter("Queen");
  EXPECT_EQ(model.rowCount(), 1); // only t3
  EXPECT_EQ(model.getTrackAt(0)["title"].toString(),
            "Another One Bites the Dust");

  // 3. Reset filters
  model.resetAllFilters();
  EXPECT_EQ(model.rowCount(), 3);

  // 4. Letter filter: '#' for numbers/non-alpha
  model.setLetterFilter("#");
  EXPECT_EQ(model.rowCount(), 1);
  EXPECT_EQ(model.getTrackAt(0)["title"].toString(), "1999 Party");

  // 5. Letter filter: 'B'
  model.setLetterFilter("B");
  EXPECT_EQ(model.rowCount(), 1);
  EXPECT_EQ(model.getTrackAt(0)["title"].toString(), "Bohemian Rhapsody");

  // 6. Free text search
  model.setLetterFilter("All");
  model.setSearchQuery("Rhapsody");
  EXPECT_EQ(model.rowCount(), 1);
  EXPECT_EQ(model.getTrackAt(0)["id"].toString(), "trk2");

  model.resetAllFilters();
  EXPECT_EQ(model.rowCount(), 3);
}

TEST_F(Phase4LibraryTest, Database_MigrationHandlesLegacyDatabasesGracefully) {
  std::string legacyDbPath = (fs::path(testDir) / "legacy.db").string();
  sqlite3 *rawDb = nullptr;
  ASSERT_EQ(sqlite3_open(legacyDbPath.c_str(), &rawDb), SQLITE_OK);

  // Create an older schema table lacking rating, is_favorite, play_count,
  // comment
  const char *oldSchema = R"(
    CREATE TABLE tracks (
      id TEXT PRIMARY KEY,
      file_path TEXT UNIQUE NOT NULL,
      title TEXT NOT NULL,
      artist TEXT NOT NULL,
      album TEXT NOT NULL,
      album_artist TEXT,
      genre TEXT,
      year INTEGER DEFAULT 0,
      track_number INTEGER DEFAULT 0,
      disc_number INTEGER DEFAULT 1,
      duration_ms INTEGER DEFAULT 0,
      sample_rate INTEGER DEFAULT 44100,
      bit_depth INTEGER DEFAULT 16,
      channels INTEGER DEFAULT 2,
      bitrate INTEGER DEFAULT 0,
      codec TEXT DEFAULT 'FLAC',
      art_hash TEXT,
      file_size INTEGER DEFAULT 0,
      date_added INTEGER DEFAULT 0
    );
  )";
  ASSERT_EQ(sqlite3_exec(rawDb, oldSchema, nullptr, nullptr, nullptr),
            SQLITE_OK);
  sqlite3_close(rawDb);

  // Opening with SqliteDatabaseAdapter must automatically migrate missing
  // columns and create indexes
  adapters::SqliteDatabaseAdapter db;
  ASSERT_TRUE(db.initialize(legacyDbPath));

  // Querying all tracks must succeed without "no such column: rating" error
  auto tracks = db.getAllTracks();
  EXPECT_TRUE(tracks.empty());

  core::Track t;
  t.id = "migrated_1";
  t.filePath = "/path/test.flac";
  t.title = "Migrated Track";
  t.rating = 5;
  t.isFavorite = true;
  EXPECT_TRUE(db.saveTrack(t));

  auto retrieved = db.getTrackById("migrated_1");
  ASSERT_TRUE(retrieved.has_value());
  EXPECT_EQ(retrieved->rating, 5);
  EXPECT_TRUE(retrieved->isFavorite);
}
