#include "adapters/metadata/TagLibMetadataAdapter.hpp"
#include "adapters/storage/SqliteDatabaseAdapter.hpp"
#include "adapters/ui_qt/models/TrackListModel.hpp"
#include "adapters/ui_qt/providers/AlbumArtImageProvider.hpp"
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
