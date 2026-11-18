#include "adapters/audio/CueSheetParser.hpp"
#include "adapters/metadata/TagLibMetadataAdapter.hpp"
#include "adapters/storage/SqliteDatabaseAdapter.hpp"
#include "adapters/ui_qt/models/TrackListModel.hpp"
#include "adapters/ui_qt/providers/AlbumArtImageProvider.hpp"
#include "adapters/ui_qt/providers/ArtistArtImageProvider.hpp"
#include "adapters/ui_qt/providers/ArtistImageManager.hpp"
#include "adapters/ui_qt/providers/DeezerArtistImageService.hpp"
#include "core/services/PlayerService.hpp"
#include "mocks/MockAudioEnginePort.hpp"
#include <QBuffer>
#include <QImage>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace fs = std::filesystem;

class MetadataArtTest : public ::testing::Test {
protected:
  fs::path m_testDir;

  void SetUp() override {
    m_testDir = fs::temp_directory_path() / "picaudio_test_art";
    fs::remove_all(m_testDir);
    fs::create_directories(m_testDir);
  }

  void TearDown() override { fs::remove_all(m_testDir); }

  void writeDummyFile(const fs::path &path, const std::vector<uint8_t> &data) {
    fs::create_directories(path.parent_path());
    std::ofstream ofs(path, std::ios::binary);
    ofs.write(reinterpret_cast<const char *>(data.data()), data.size());
  }

  static std::vector<uint8_t> createValidImageBytes() {
    QImage img(4, 4, QImage::Format_RGB32);
    img.fill(Qt::red);
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, "PNG");
    return std::vector<uint8_t>(bytes.begin(), bytes.end());
  }
};

TEST_F(MetadataArtTest, DetectsCommonMimeTypesFromMagicBytes) {
  std::vector<uint8_t> jpegHeader = {0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10};
  EXPECT_EQ(adapters::TagLibMetadataAdapter::detectMimeType(jpegHeader.data(),
                                                            jpegHeader.size()),
            "image/jpeg");

  std::vector<uint8_t> pngHeader = {0x89, 0x50, 0x4E, 0x47,
                                    0x0D, 0x0A, 0x1A, 0x0A};
  EXPECT_EQ(adapters::TagLibMetadataAdapter::detectMimeType(pngHeader.data(),
                                                            pngHeader.size()),
            "image/png");

  std::vector<uint8_t> webpHeader = {'R', 'I', 'F', 'F', 0,   0,
                                     0,   0,   'W', 'E', 'B', 'P'};
  EXPECT_EQ(adapters::TagLibMetadataAdapter::detectMimeType(webpHeader.data(),
                                                            webpHeader.size()),
            "image/webp");

  std::vector<uint8_t> gifHeader = {'G', 'I', 'F', '8', '9', 'a'};
  EXPECT_EQ(adapters::TagLibMetadataAdapter::detectMimeType(gifHeader.data(),
                                                            gifHeader.size()),
            "image/gif");

  std::vector<uint8_t> bmpHeader = {'B', 'M', 0, 0};
  EXPECT_EQ(adapters::TagLibMetadataAdapter::detectMimeType(bmpHeader.data(),
                                                            bmpHeader.size()),
            "image/bmp");
}

TEST_F(MetadataArtTest, SupportsAllCommonAudioExtensionsCaseInsensitively) {
  adapters::TagLibMetadataAdapter adapter;
  EXPECT_TRUE(adapter.supportsFormat(".flac"));
  EXPECT_TRUE(adapter.supportsFormat(".FLAC"));
  EXPECT_TRUE(adapter.supportsFormat(".mp3"));
  EXPECT_TRUE(adapter.supportsFormat(".MP3"));
  EXPECT_TRUE(adapter.supportsFormat(".m4a"));
  EXPECT_TRUE(adapter.supportsFormat(".M4A"));
  EXPECT_TRUE(adapter.supportsFormat(".alac"));
  EXPECT_TRUE(adapter.supportsFormat(".wav"));
  EXPECT_TRUE(adapter.supportsFormat(".aiff"));
  EXPECT_TRUE(adapter.supportsFormat(".ogg"));
  EXPECT_TRUE(adapter.supportsFormat(".oga"));
  EXPECT_TRUE(adapter.supportsFormat(".opus"));
  EXPECT_TRUE(adapter.supportsFormat(".spx"));
  EXPECT_TRUE(adapter.supportsFormat(".dsf"));
  EXPECT_TRUE(adapter.supportsFormat(".dff"));
  EXPECT_TRUE(adapter.supportsFormat(".ape"));
  EXPECT_TRUE(adapter.supportsFormat(".wv"));
  EXPECT_TRUE(adapter.supportsFormat(".wma"));
  EXPECT_FALSE(adapter.supportsFormat(".txt"));
  EXPECT_FALSE(adapter.supportsFormat(".pdf"));
}

TEST_F(MetadataArtTest, DiscoversMixedCaseFolderArtwork) {
  adapters::TagLibMetadataAdapter adapter;
  fs::path albumDir = m_testDir / "Pink Floyd - Dark Side";
  fs::path dummyTrack = albumDir / "01 - Speak to Me.flac";
  fs::path coverFile = albumDir / "Cover.JPG";

  std::vector<uint8_t> jpegData = {0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10,
                                   'J',  'F',  'I',  'F',  0x00};
  writeDummyFile(dummyTrack, {0x66, 0x4C, 0x61, 0x43}); // "fLaC" dummy
  writeDummyFile(coverFile, jpegData);

  auto artOpt = adapter.extractArtwork(dummyTrack.string());
  ASSERT_TRUE(artOpt.has_value());
  EXPECT_EQ(artOpt->mimeType, "image/jpeg");
  EXPECT_FALSE(artOpt->artHash.empty());
  EXPECT_EQ(artOpt->data, jpegData);
}

TEST_F(MetadataArtTest, DiscoversArtworkFromParentOfDiscSubdirectory) {
  adapters::TagLibMetadataAdapter adapter;
  fs::path albumRoot = m_testDir / "The Wall";
  fs::path cd1Dir = albumRoot / "CD1";
  fs::path trackInCd1 = cd1Dir / "01 - In The Flesh.flac";
  fs::path rootCover = albumRoot / "folder.png";

  std::vector<uint8_t> pngData = {0x89, 0x50, 0x4E, 0x47, 0x0D,
                                  0x0A, 0x1A, 0x0A, 0x00};
  writeDummyFile(trackInCd1, {0x66, 0x4C, 0x61, 0x43});
  writeDummyFile(rootCover, pngData);

  auto artOpt = adapter.extractArtwork(trackInCd1.string());
  ASSERT_TRUE(artOpt.has_value());
  EXPECT_EQ(artOpt->mimeType, "image/png");
  EXPECT_EQ(artOpt->data, pngData);
}

TEST_F(MetadataArtTest, DatabaseResolvesTrackArtistWhenAlbumArtistMissing) {
  adapters::SqliteDatabaseAdapter db;
  std::string dbPath = (m_testDir / "test_music.db").string();
  ASSERT_TRUE(db.initialize(dbPath));

  core::Track t1;
  t1.id = "trk_1";
  t1.filePath = "/music/Radiohead/Kid A/01.flac";
  t1.title = "Everything in Its Right Place";
  t1.artist = "Radiohead";
  t1.album = "Kid A";
  t1.albumArtist = ""; // Missing album artist tag
  t1.genre = "Electronic";
  t1.year = 2000;
  t1.trackNumber = 1;
  t1.durationMs = 250000;
  t1.codec = "FLAC";

  ASSERT_TRUE(db.saveTrack(t1));

  auto albums = db.getAllAlbums();
  ASSERT_EQ(albums.size(), 1u);
  EXPECT_EQ(albums[0].title, "Kid A");
  EXPECT_EQ(albums[0].artist, "Radiohead");
}

TEST_F(MetadataArtTest, DatabaseResolvesLegacyUnknownArtistToTrackArtist) {
  adapters::SqliteDatabaseAdapter db;
  std::string dbPath = (m_testDir / "test_legacy.db").string();
  ASSERT_TRUE(db.initialize(dbPath));

  core::Track t1;
  t1.id = "trk_legacy";
  t1.filePath = "/music/Daft Punk/Discovery/01.flac";
  t1.title = "One More Time";
  t1.artist = "Daft Punk";
  t1.album = "Discovery";
  t1.albumArtist = "Unknown Artist"; // Legacy default before fix
  t1.durationMs = 320000;
  t1.codec = "FLAC";

  ASSERT_TRUE(db.saveTrack(t1));

  auto albums = db.getAllAlbums();
  ASSERT_EQ(albums.size(), 1u);
  EXPECT_EQ(albums[0].title, "Discovery");
  EXPECT_EQ(albums[0].artist, "Daft Punk");
}

TEST_F(MetadataArtTest, DatabaseResolvesCompilationWithAlbumArtist) {
  adapters::SqliteDatabaseAdapter db;
  std::string dbPath = (m_testDir / "test_comp.db").string();
  ASSERT_TRUE(db.initialize(dbPath));

  core::Track t1;
  t1.id = "trk_c1";
  t1.filePath = "/music/Comp/01.flac";
  t1.title = "Song A";
  t1.artist = "Artist One";
  t1.album = "Summer Hits";
  t1.albumArtist = "Various Artists";
  t1.durationMs = 180000;

  core::Track t2;
  t2.id = "trk_c2";
  t2.filePath = "/music/Comp/02.flac";
  t2.title = "Song B";
  t2.artist = "Artist Two";
  t2.album = "Summer Hits";
  t2.albumArtist = "Various Artists";
  t2.durationMs = 200000;

  ASSERT_TRUE(db.saveTrack(t1));
  ASSERT_TRUE(db.saveTrack(t2));

  auto albums = db.getAllAlbums();
  ASSERT_EQ(albums.size(), 1u);
  EXPECT_EQ(albums[0].title, "Summer Hits");
  EXPECT_EQ(albums[0].artist, "Various Artists");

  // Querying tracks for album by Various Artists returns all compilation tracks
  auto tracks = db.getTracksByAlbum("Summer Hits", "Various Artists");
  EXPECT_EQ(tracks.size(), 2u);
}

TEST_F(MetadataArtTest, TrackListModelResolvesArtistForAlbumArtistRole) {
  TrackListModel model;

  core::Track t1;
  t1.id = "t1";
  t1.title = "Track 1";
  t1.artist = "The Beatles";
  t1.album = "Abbey Road";
  t1.albumArtist = ""; // Empty

  core::Track t2;
  t2.id = "t2";
  t2.title = "Track 2";
  t2.artist = "The Beatles";
  t2.album = "Abbey Road";
  t2.albumArtist = "Unknown Artist"; // Legacy placeholder

  core::Track t3;
  t3.id = "t3";
  t3.title = "Track 3";
  t3.artist = "Featuring Artist";
  t3.album = "Solo Album";
  t3.albumArtist = "Primary Artist"; // Explicit album artist

  model.setTracks({t1, t2, t3});

  QModelIndex idx1 = model.index(0, 0);
  QModelIndex idx2 = model.index(1, 0);
  QModelIndex idx3 = model.index(2, 0);

  EXPECT_EQ(model.data(idx1, TrackListModel::AlbumArtistRole).toString(),
            "The Beatles");
  EXPECT_EQ(model.data(idx2, TrackListModel::AlbumArtistRole).toString(),
            "The Beatles");
  EXPECT_EQ(model.data(idx3, TrackListModel::AlbumArtistRole).toString(),
            "Primary Artist");
}

TEST_F(MetadataArtTest, AlbumGroupingMergesFeaturingArtistsWhenEnabled) {
  adapters::SqliteDatabaseAdapter db;
  std::string dbPath = (m_testDir / "test_grouping_enabled.db").string();
  ASSERT_TRUE(db.initialize(dbPath));

  core::Track t1;
  t1.id = "trk_dre_1";
  t1.filePath = "/music/2001/01.flac";
  t1.title = "The Watcher";
  t1.artist = "Dr. Dre";
  t1.album = "2001";
  t1.albumArtist = "";
  t1.durationMs = 200000;

  core::Track t2;
  t2.id = "trk_dre_2";
  t2.filePath = "/music/2001/02.flac";
  t2.title = "Still D.R.E.";
  t2.artist = "Dr. Dre feat. Snoop Dogg";
  t2.album = "2001";
  t2.albumArtist = "";
  t2.durationMs = 270000;

  core::Track t3;
  t3.id = "trk_dre_3";
  t3.filePath = "/music/2001/03.flac";
  t3.title = "Forgot About Dre";
  t3.artist = "Dr. Dre feat. Eminem";
  t3.album = "2001";
  t3.albumArtist = "";
  t3.durationMs = 220000;

  ASSERT_TRUE(db.saveTrack(t1));
  ASSERT_TRUE(db.saveTrack(t2));
  ASSERT_TRUE(db.saveTrack(t3));

  db.setGroupAlbumsByTitle(true);
  EXPECT_TRUE(db.groupAlbumsByTitle());

  auto albums = db.getAllAlbums();
  ASSERT_EQ(albums.size(), 1u);
  EXPECT_EQ(albums[0].title, "2001");
  EXPECT_EQ(albums[0].artist, "Dr. Dre");
  EXPECT_EQ(albums[0].trackCount, 3u);
  EXPECT_EQ(db.getAlbumCount(), 1u);

  auto tracks = db.getTracksByAlbum("2001", "Dr. Dre");
  EXPECT_EQ(tracks.size(), 3u);
}

TEST_F(MetadataArtTest, AlbumGroupingSplitsFeaturingArtistsWhenDisabled) {
  adapters::SqliteDatabaseAdapter db;
  std::string dbPath = (m_testDir / "test_grouping_disabled.db").string();
  ASSERT_TRUE(db.initialize(dbPath));

  core::Track t1;
  t1.id = "trk_dre_1";
  t1.filePath = "/music/2001/01.flac";
  t1.title = "The Watcher";
  t1.artist = "Dr. Dre";
  t1.album = "2001";
  t1.albumArtist = "";
  t1.durationMs = 200000;

  core::Track t2;
  t2.id = "trk_dre_2";
  t2.filePath = "/music/2001/02.flac";
  t2.title = "Still D.R.E.";
  t2.artist = "Dr. Dre feat. Snoop Dogg";
  t2.album = "2001";
  t2.albumArtist = "";
  t2.durationMs = 270000;

  core::Track t3;
  t3.id = "trk_dre_3";
  t3.filePath = "/music/2001/03.flac";
  t3.title = "Forgot About Dre";
  t3.artist = "Dr. Dre feat. Eminem";
  t3.album = "2001";
  t3.albumArtist = "";
  t3.durationMs = 220000;

  ASSERT_TRUE(db.saveTrack(t1));
  ASSERT_TRUE(db.saveTrack(t2));
  ASSERT_TRUE(db.saveTrack(t3));

  db.setGroupAlbumsByTitle(false);
  EXPECT_FALSE(db.groupAlbumsByTitle());

  auto albums = db.getAllAlbums();
  ASSERT_EQ(albums.size(), 3u);
  EXPECT_EQ(db.getAlbumCount(), 3u);

  auto tracksSolo = db.getTracksByAlbum("2001", "Dr. Dre");
  EXPECT_EQ(tracksSolo.size(), 1u);
  EXPECT_EQ(tracksSolo[0].title, "The Watcher");

  auto tracksSnoop = db.getTracksByAlbum("2001", "Dr. Dre feat. Snoop Dogg");
  EXPECT_EQ(tracksSnoop.size(), 1u);
  EXPECT_EQ(tracksSnoop[0].title, "Still D.R.E.");
}

TEST_F(MetadataArtTest, ArtworkPriorityHonorsPreference) {
  adapters::TagLibMetadataAdapter adapter;
  fs::path albumDir = m_testDir / "PriorityAlbum";
  fs::path dummyTrack = albumDir / "01.flac";
  fs::path coverFile = albumDir / "cover.jpg";

  std::vector<uint8_t> jpegData = {0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10,
                                   'J',  'F',  'I',  'F',  0x00};
  writeDummyFile(dummyTrack, {0x66, 0x4C, 0x61, 0x43}); // dummy file
  writeDummyFile(coverFile, jpegData);

  // External Folder Art First
  adapter.setArtworkPriority(false);
  auto artFolder = adapter.extractArtwork(dummyTrack.string());
  ASSERT_TRUE(artFolder.has_value());
  EXPECT_EQ(artFolder->data, jpegData);

  // Embedded Tags First (falls back to folder art when embedded missing)
  adapter.setArtworkPriority(true);
  auto artEmbeddedFallback = adapter.extractArtwork(dummyTrack.string());
  ASSERT_TRUE(artEmbeddedFallback.has_value());
  EXPECT_EQ(artEmbeddedFallback->data, jpegData);
}

TEST_F(MetadataArtTest, ImageProviderCachesBothAlbumIdAndArtHash) {
  adapters::SqliteDatabaseAdapter db;
  std::string dbPath = (m_testDir / "test_provider.db").string();
  ASSERT_TRUE(db.initialize(dbPath));

  fs::path albumDir = m_testDir / "AlbumProvider";
  fs::path trackFile = albumDir / "track.flac";
  fs::path coverFile = albumDir / "cover.jpg";
  fs::path cacheDir = m_testDir / "covers_cache";

  std::vector<uint8_t> imgData = createValidImageBytes();
  writeDummyFile(trackFile, {0x66, 0x4C, 0x61, 0x43});
  writeDummyFile(coverFile, imgData);

  adapters::TagLibMetadataAdapter adapter;
  auto artOpt = adapter.extractArtwork(trackFile.string());
  ASSERT_TRUE(artOpt.has_value());
  std::string hash = artOpt->artHash;
  ASSERT_FALSE(hash.empty());

  core::Track t;
  t.id = "trk_provider";
  t.filePath = trackFile.string();
  t.title = "Sample Title";
  t.artist = "Sample Artist";
  t.album = "Sample Album";
  t.albumArtist = "Sample Artist";
  t.artHash = hash;
  ASSERT_TRUE(db.saveTrack(t));

  AlbumArtImageProvider provider(&adapter, &db,
                                 QString::fromStdString(cacheDir.string()));

  // Request using album ID
  QSize sz;
  provider.requestPixmap("alb_Sample Album_Sample Artist", &sz,
                         QSize(200, 200));

  // Both alb_Sample Album_Sample Artist.jpg and <hash>.jpg should be written
  EXPECT_TRUE(fs::exists(cacheDir / "alb_Sample Album_Sample Artist.jpg"));
  EXPECT_TRUE(fs::exists(cacheDir / (hash + ".jpg")));
}

TEST_F(MetadataArtTest,
       ImageProviderResolvesLegacyTrackArtistWhenMatchingAlbumId) {
  adapters::SqliteDatabaseAdapter db;
  std::string dbPath = (m_testDir / "test_legacy_match.db").string();
  ASSERT_TRUE(db.initialize(dbPath));

  fs::path albumDir = m_testDir / "LegacyAlbum";
  fs::path trackFile = albumDir / "01.flac";
  fs::path coverFile = albumDir / "cover.jpg";
  fs::path cacheDir = m_testDir / "covers_legacy_cache";

  std::vector<uint8_t> imgData = createValidImageBytes();
  writeDummyFile(trackFile, {0x66, 0x4C, 0x61, 0x43});
  writeDummyFile(coverFile, imgData);

  adapters::TagLibMetadataAdapter adapter;
  auto artOpt = adapter.extractArtwork(trackFile.string());
  ASSERT_TRUE(artOpt.has_value());

  core::Track t;
  t.id = "trk_legacy_match";
  t.filePath = trackFile.string();
  t.title = "Legacy Title";
  t.artist = "Solo Artist";
  t.album = "Legacy Album";
  t.albumArtist = "Unknown Artist"; // Legacy placeholder in database
  t.artHash = artOpt->artHash;
  ASSERT_TRUE(db.saveTrack(t));

  AlbumArtImageProvider provider(&adapter, &db,
                                 QString::fromStdString(cacheDir.string()));

  // Request by effective album card ID (alb_Legacy Album_Solo Artist)
  QSize sz;
  provider.requestPixmap("alb_Legacy Album_Solo Artist", &sz, QSize(200, 200));

  EXPECT_TRUE(fs::exists(cacheDir / "alb_Legacy Album_Solo Artist.jpg"));
  EXPECT_TRUE(fs::exists(cacheDir / (artOpt->artHash + ".jpg")));
}

TEST_F(MetadataArtTest, ExtractsArtworkFromOpusXiphComment) {
  adapters::TagLibMetadataAdapter adapter;
  std::string samithaPath =
      "/home/shamal/Music/newAud/Laa Sanda Aye Payala - Samitha "
      "Mudunkotuwa/Laa Sanda Aye Payala - Samitha Mudunkotuwa.opus";
  if (fs::exists(samithaPath)) {
    auto art = adapter.extractArtwork(samithaPath);
    ASSERT_TRUE(art.has_value());
    EXPECT_EQ(art->data.size(), 61937u);
    EXPECT_EQ(art->mimeType, "image/jpeg");
    EXPECT_EQ(art->artHash, "dec63f7ec72ac9b2");
  }

  std::string sangeethPath =
      "/home/shamal/Music/newAud/Sangeeth Wijesuriya - Sangeeth Medley "
      "(Live)/Sangeeth Medley (Live).opus";
  if (fs::exists(sangeethPath)) {
    auto art = adapter.extractArtwork(sangeethPath);
    ASSERT_TRUE(art.has_value());
    EXPECT_EQ(art->data.size(), 1458244u);
    EXPECT_EQ(art->mimeType, "image/png");
    EXPECT_EQ(art->artHash, "3e7dace7273c5cba");
  }
}

TEST_F(MetadataArtTest, ArtistImageManagerInitializationAndServices) {
  QString cacheDir = QString::fromStdString((m_testDir / "artists").string());
  adapters::ArtistImageManager manager(cacheDir);

  EXPECT_EQ(manager.cacheDir(), cacheDir);
  EXPECT_TRUE(manager.isOnlineFetchingEnabled());

  QStringList services = manager.registeredServiceNames();
  ASSERT_FALSE(services.isEmpty());
  EXPECT_TRUE(services.contains("Deezer"));
}

TEST_F(MetadataArtTest, ArtistImageManagerDiskAndMemoryCaching) {
  QString cacheDir = QString::fromStdString((m_testDir / "artists").string());
  adapters::ArtistImageManager manager(cacheDir);

  QString artistName = "Queen";
  EXPECT_FALSE(manager.hasCachedImage(artistName));
  EXPECT_TRUE(manager.getCachedDiskPath(artistName).isEmpty());
  EXPECT_FALSE(manager.getCachedPixmap(artistName).has_value());

  // Create test image data
  auto imgBytes = createValidImageBytes();
  QByteArray qdata(reinterpret_cast<const char *>(imgBytes.data()),
                   static_cast<int>(imgBytes.size()));

  manager.cacheArtistImage(artistName, qdata);

  EXPECT_TRUE(manager.hasCachedImage(artistName));
  EXPECT_FALSE(manager.getCachedDiskPath(artistName).isEmpty());
  auto pixOpt = manager.getCachedPixmap(artistName);
  ASSERT_TRUE(pixOpt.has_value());
  EXPECT_FALSE(pixOpt->isNull());

  // Clear cache
  manager.clearCache();
  EXPECT_FALSE(manager.hasCachedImage(artistName));
  EXPECT_TRUE(manager.getCachedDiskPath(artistName).isEmpty());
}

TEST_F(MetadataArtTest, ArtistImageManagerSafeDiskFileName) {
  EXPECT_EQ(adapters::ArtistImageManager::safeDiskFileName("AC/DC"), "AC_DC");
  EXPECT_EQ(adapters::ArtistImageManager::safeDiskFileName("Guns N' Roses"),
            "Guns N' Roses");
  EXPECT_EQ(adapters::ArtistImageManager::safeDiskFileName("Artist: The Best?"),
            "Artist_ The Best_");
}

TEST_F(MetadataArtTest, ArtistArtImageProviderResolutionAndFallback) {
  QString cacheDir = QString::fromStdString((m_testDir / "artists").string());
  adapters::ArtistImageManager manager(cacheDir);
  manager.setOnlineFetchingEnabled(false); // Offline test mode

  adapters::ArtistArtImageProvider provider(&manager);

  QSize sz;
  // Unknown or uncached artist should return null pixmap (fallback to vector
  // icon)
  QPixmap nullPix =
      provider.requestPixmap("NonExistentArtist", &sz, QSize(100, 100));
  EXPECT_TRUE(nullPix.isNull());

  // Cache an artist image
  auto imgBytes = createValidImageBytes();
  QByteArray qdata(reinterpret_cast<const char *>(imgBytes.data()),
                   static_cast<int>(imgBytes.size()));
  manager.cacheArtistImage("The Beatles", qdata);

  // Request cached artist with query string revision
  QPixmap loadedPix =
      provider.requestPixmap("The%20Beatles?v=2", &sz, QSize(100, 100));
  EXPECT_FALSE(loadedPix.isNull());
}

TEST_F(MetadataArtTest, CueSheetParserTimestampAndRollover) {
  // Test basic timestamp parsing
  EXPECT_EQ(adapters::CueSheetParser::parseIndexTimestampMs("00:00:00"), 0u);
  // 01:23:45 -> (1*60 + 23)*1000 + 45*1000/75 = 83000 + 600 = 83600 ms
  EXPECT_EQ(adapters::CueSheetParser::parseIndexTimestampMs("01:23:45"),
            83600u);

  // Test CUE sheet with rollover past 60 minutes
  std::string cueContent = "PERFORMER \"Live Artist\"\n"
                           "TITLE \"Concert\"\n"
                           "FILE \"concert.flac\" WAVE\n"
                           "  TRACK 01 AUDIO\n"
                           "    TITLE \"Track 1\"\n"
                           "    INDEX 01 55:00:00\n"
                           "  TRACK 02 AUDIO\n"
                           "    TITLE \"Track 2\"\n"
                           "    INDEX 01 05:00:00\n";

  auto tracks = adapters::CueSheetParser::parseString(
      cueContent, "/music/concert.flac", 7200000);
  ASSERT_EQ(tracks.size(), 2u);
  EXPECT_EQ(tracks[0].title, "Track 1");
  EXPECT_EQ(tracks[0].startTimeMs, 55u * 60u * 1000u); // 3,300,000 ms
  EXPECT_EQ(tracks[1].title, "Track 2");
  // Track 2 rolled over 60 mins -> 60*60*1000 + 5*60*1000 = 3,900,000 ms
  EXPECT_EQ(tracks[1].startTimeMs, (60u + 5u) * 60u * 1000u);
  EXPECT_GT(tracks[1].startTimeMs, tracks[0].startTimeMs);
  EXPECT_EQ(tracks[0].durationMs,
            tracks[1].startTimeMs - tracks[0].startTimeMs);
}

TEST_F(MetadataArtTest, CueSheetParserCompanionResolution) {
  fs::path albumDir = m_testDir / "cue_album";
  fs::create_directories(albumDir);

  fs::path companion = albumDir / "Tribute_Live.m4a";
  std::vector<uint8_t> dummyData = {1, 2, 3, 4};
  writeDummyFile(companion, dummyData);

  fs::path cuePath = albumDir / "Tribute_Live.cue";
  std::string cueContent = "FILE \"output_video.mp4\" WAVE\n"
                           "  TRACK 01 AUDIO\n"
                           "    TITLE \"Opening\"\n"
                           "    INDEX 01 00:00:00\n";
  std::ofstream ofs(cuePath);
  ofs << cueContent;
  ofs.close();

  std::string resolved = adapters::CueSheetParser::resolveAudioFile(
      cuePath.string(), "output_video.mp4");
  EXPECT_EQ(resolved, companion.string());
}

TEST_F(MetadataArtTest, TagLibSupportsCueFormatAndExtractMultiple) {
  adapters::TagLibMetadataAdapter adapter;
  EXPECT_TRUE(adapter.supportsFormat(".cue"));
  EXPECT_TRUE(adapter.supportsFormat("CUE"));
  EXPECT_TRUE(adapter.supportsFormat(".CUE"));

  fs::path albumDir = m_testDir / "cue_multi";
  fs::create_directories(albumDir);
  fs::path audioPath = albumDir / "album.flac";
  std::vector<uint8_t> dummyAudio = {1, 2, 3, 4};
  writeDummyFile(audioPath, dummyAudio);

  fs::path cuePath = albumDir / "album.cue";
  std::string cueContent = "PERFORMER \"Test Artist\"\n"
                           "TITLE \"Test Album\"\n"
                           "FILE \"album.flac\" WAVE\n"
                           "  TRACK 01 AUDIO\n"
                           "    TITLE \"Track 1\"\n"
                           "    INDEX 01 00:00:00\n"
                           "  TRACK 02 AUDIO\n"
                           "    TITLE \"Track 2\"\n"
                           "    INDEX 01 03:00:00\n";
  std::ofstream ofs(cuePath);
  ofs << cueContent;
  ofs.close();

  auto multi = adapter.extractMultiple(cuePath.string());
  ASSERT_EQ(multi.size(), 2u);
  EXPECT_TRUE(multi[0].isVirtualTrack);
  EXPECT_TRUE(multi[1].isVirtualTrack);
  EXPECT_EQ(multi[0].title, "Track 1");
  EXPECT_EQ(multi[1].title, "Track 2");
  EXPECT_EQ(multi[0].artist, "Test Artist");
  EXPECT_EQ(multi[0].album, "Test Album");
}

TEST_F(MetadataArtTest, PlayerServiceVirtualTrackSeekAndPosition) {
  auto mockAudio = std::make_shared<tests::MockAudioEnginePort>();
  core::PlayerService player(mockAudio);

  core::Track virtualTrack;
  virtualTrack.id = "cue_trk_2";
  virtualTrack.filePath = "/music/album.flac#cue:2";
  virtualTrack.title = "Second Track";
  virtualTrack.isVirtualTrack = true;
  virtualTrack.startTimeMs = 60000; // 1:00
  virtualTrack.endTimeMs = 240000;  // 4:00
  virtualTrack.durationMs = 180000; // 3:00

  player.play(virtualTrack);

  // Audio engine should load file and seek to startTimeMs
  EXPECT_EQ(mockAudio->m_loadedFilePath, virtualTrack.filePath);
  EXPECT_EQ(mockAudio->m_positionMs, 60000u);
  EXPECT_TRUE(player.isPlaying());

  // Seeking to relative 30s within track should seek engine to 90s
  player.seek(30000);
  EXPECT_EQ(mockAudio->m_positionMs, 90000u);

  // Position query when engine is at 75s (15s into track)
  mockAudio->m_positionMs = 75000;
  EXPECT_EQ(player.getPositionMs(), 15000u);

  // Track duration should report virtual track duration (180s), not full
  // container
  EXPECT_EQ(player.getDurationMs(), 180000u);
}
