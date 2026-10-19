#include "SqliteDatabaseAdapter.hpp"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <unordered_map>

namespace adapters {

namespace {

std::string extractBaseArtist(const std::string &artist) {
  if (artist.empty()) {
    return "";
  }
  std::string lower = artist;
  std::transform(
      lower.begin(), lower.end(), lower.begin(),
      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

  const std::vector<std::string> delimiters = {
      " feat. ",  " ft. ",    " featuring ", " feat ", " ft ",
      " (feat. ", " [feat. ", " (ft. ",      " [ft. ", " with ",
      " vs. ",    " vs ",     " & ",         " / ",    " and "};

  size_t earliestPos = std::string::npos;
  for (const auto &delim : delimiters) {
    size_t pos = lower.find(delim);
    if (pos != std::string::npos && pos < earliestPos) {
      earliestPos = pos;
    }
  }

  if (earliestPos != std::string::npos && earliestPos > 0) {
    std::string base = artist.substr(0, earliestPos);
    while (!base.empty() &&
           std::isspace(static_cast<unsigned char>(base.back()))) {
      base.pop_back();
    }
    if (!base.empty()) {
      return base;
    }
  }

  return artist;
}

} // namespace

SqliteDatabaseAdapter::SqliteDatabaseAdapter() = default;

SqliteDatabaseAdapter::~SqliteDatabaseAdapter() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_db) {
    sqlite3_close(m_db);
    m_db = nullptr;
  }
}

bool SqliteDatabaseAdapter::initialize(const std::string &dbPath) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_dbPath = dbPath;

  if (m_db) {
    sqlite3_close(m_db);
    m_db = nullptr;
  }

  int rc = sqlite3_open(dbPath.c_str(), &m_db);
  if (rc != SQLITE_OK) {
    std::cerr << "SqliteDatabaseAdapter: Failed to open DB " << dbPath << ": "
              << sqlite3_errmsg(m_db) << std::endl;
    return false;
  }

  // Set performance pragmas for fast local indexing
  sqlite3_exec(m_db, "PRAGMA journal_mode = WAL;", nullptr, nullptr, nullptr);
  sqlite3_exec(m_db, "PRAGMA synchronous = NORMAL;", nullptr, nullptr, nullptr);
  sqlite3_exec(m_db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);

  return createTables();
}

bool SqliteDatabaseAdapter::createTables() {
  const char *schema = R"(
        CREATE TABLE IF NOT EXISTS tracks (
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

        CREATE INDEX IF NOT EXISTS idx_tracks_album ON tracks(album, album_artist);
        CREATE INDEX IF NOT EXISTS idx_tracks_artist ON tracks(artist);
        CREATE INDEX IF NOT EXISTS idx_tracks_genre ON tracks(genre);
        CREATE INDEX IF NOT EXISTS idx_tracks_path ON tracks(file_path);
    )";

  char *errMsg = nullptr;
  int rc = sqlite3_exec(m_db, schema, nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK) {
    std::cerr << "SqliteDatabaseAdapter: Schema creation failed: "
              << (errMsg ? errMsg : "") << std::endl;
    sqlite3_free(errMsg);
    return false;
  }

  return true;
}

core::Track SqliteDatabaseAdapter::parseTrackRow(sqlite3_stmt *stmt) {
  core::Track t;
  t.id = reinterpret_cast<const char *>(
      sqlite3_column_text(stmt, 0)
          ? sqlite3_column_text(stmt, 0)
          : reinterpret_cast<const unsigned char *>(""));
  t.filePath = reinterpret_cast<const char *>(
      sqlite3_column_text(stmt, 1)
          ? sqlite3_column_text(stmt, 1)
          : reinterpret_cast<const unsigned char *>(""));
  t.title = reinterpret_cast<const char *>(
      sqlite3_column_text(stmt, 2)
          ? sqlite3_column_text(stmt, 2)
          : reinterpret_cast<const unsigned char *>(""));
  t.artist = reinterpret_cast<const char *>(
      sqlite3_column_text(stmt, 3)
          ? sqlite3_column_text(stmt, 3)
          : reinterpret_cast<const unsigned char *>(""));
  t.album = reinterpret_cast<const char *>(
      sqlite3_column_text(stmt, 4)
          ? sqlite3_column_text(stmt, 4)
          : reinterpret_cast<const unsigned char *>(""));
  t.albumArtist = reinterpret_cast<const char *>(
      sqlite3_column_text(stmt, 5)
          ? sqlite3_column_text(stmt, 5)
          : reinterpret_cast<const unsigned char *>(""));
  t.genre = reinterpret_cast<const char *>(
      sqlite3_column_text(stmt, 6)
          ? sqlite3_column_text(stmt, 6)
          : reinterpret_cast<const unsigned char *>(""));
  t.year = static_cast<uint32_t>(sqlite3_column_int(stmt, 7));
  t.trackNumber = static_cast<uint32_t>(sqlite3_column_int(stmt, 8));
  t.discNumber = static_cast<uint32_t>(sqlite3_column_int(stmt, 9));
  t.durationMs = static_cast<uint64_t>(sqlite3_column_int64(stmt, 10));
  t.sampleRate = static_cast<uint32_t>(sqlite3_column_int(stmt, 11));
  t.bitDepth = static_cast<uint8_t>(sqlite3_column_int(stmt, 12));
  t.channels = static_cast<uint8_t>(sqlite3_column_int(stmt, 13));
  t.bitrate = static_cast<uint32_t>(sqlite3_column_int(stmt, 14));
  t.codec = reinterpret_cast<const char *>(
      sqlite3_column_text(stmt, 15)
          ? sqlite3_column_text(stmt, 15)
          : reinterpret_cast<const unsigned char *>("FLAC"));
  t.artHash = reinterpret_cast<const char *>(
      sqlite3_column_text(stmt, 16)
          ? sqlite3_column_text(stmt, 16)
          : reinterpret_cast<const unsigned char *>(""));
  t.fileSizeBytes = static_cast<uint64_t>(sqlite3_column_int64(stmt, 17));
  t.dateAdded = static_cast<uint64_t>(sqlite3_column_int64(stmt, 18));
  return t;
}

bool SqliteDatabaseAdapter::saveTrack(const core::Track &track) {
  return saveTracks({track});
}

bool SqliteDatabaseAdapter::saveTracks(const std::vector<core::Track> &tracks) {
  if (tracks.empty())
    return true;

  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_db)
    return false;

  sqlite3_exec(m_db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

  const char *sql = R"(
        INSERT INTO tracks (
            id, file_path, title, artist, album, album_artist, genre,
            year, track_number, disc_number, duration_ms, sample_rate,
            bit_depth, channels, bitrate, codec, art_hash, file_size, date_added
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(file_path) DO UPDATE SET
            title=excluded.title,
            artist=excluded.artist,
            album=excluded.album,
            album_artist=excluded.album_artist,
            genre=excluded.genre,
            year=excluded.year,
            track_number=excluded.track_number,
            disc_number=excluded.disc_number,
            duration_ms=excluded.duration_ms,
            sample_rate=excluded.sample_rate,
            bit_depth=excluded.bit_depth,
            channels=excluded.channels,
            bitrate=excluded.bitrate,
            codec=excluded.codec,
            art_hash=excluded.art_hash,
            file_size=excluded.file_size;
    )";

  sqlite3_stmt *stmt = nullptr;
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
    return false;
  }

  for (const auto &t : tracks) {
    sqlite3_reset(stmt);
    sqlite3_bind_text(stmt, 1, t.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, t.filePath.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, t.title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, t.artist.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, t.album.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, t.albumArtist.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, t.genre.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 8, static_cast<int>(t.year));
    sqlite3_bind_int(stmt, 9, static_cast<int>(t.trackNumber));
    sqlite3_bind_int(stmt, 10, static_cast<int>(t.discNumber));
    sqlite3_bind_int64(stmt, 11, static_cast<sqlite3_int64>(t.durationMs));
    sqlite3_bind_int(stmt, 12, static_cast<int>(t.sampleRate));
    sqlite3_bind_int(stmt, 13, static_cast<int>(t.bitDepth));
    sqlite3_bind_int(stmt, 14, static_cast<int>(t.channels));
    sqlite3_bind_int(stmt, 15, static_cast<int>(t.bitrate));
    sqlite3_bind_text(stmt, 16, t.codec.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 17, t.artHash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 18, static_cast<sqlite3_int64>(t.fileSizeBytes));
    sqlite3_bind_int64(stmt, 19, static_cast<sqlite3_int64>(t.dateAdded));

    sqlite3_step(stmt);
  }

  sqlite3_finalize(stmt);
  sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, nullptr);
  return true;
}

std::optional<core::Track>
SqliteDatabaseAdapter::getTrackById(const std::string &id) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_db)
    return std::nullopt;

  const char *sql =
      "SELECT id, file_path, title, artist, album, album_artist, genre, year, "
      "track_number, disc_number, duration_ms, sample_rate, bit_depth, "
      "channels, bitrate, codec, art_hash, file_size, date_added FROM tracks "
      "WHERE id = ? LIMIT 1;";
  sqlite3_stmt *stmt = nullptr;
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return std::nullopt;

  sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);

  std::optional<core::Track> result = std::nullopt;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    result = parseTrackRow(stmt);
  }

  sqlite3_finalize(stmt);
  return result;
}

std::optional<core::Track>
SqliteDatabaseAdapter::getTrackByPath(const std::string &path) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_db)
    return std::nullopt;

  const char *sql =
      "SELECT id, file_path, title, artist, album, album_artist, genre, year, "
      "track_number, disc_number, duration_ms, sample_rate, bit_depth, "
      "channels, bitrate, codec, art_hash, file_size, date_added FROM tracks "
      "WHERE file_path = ? LIMIT 1;";
  sqlite3_stmt *stmt = nullptr;
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return std::nullopt;

  sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_TRANSIENT);

  std::optional<core::Track> result = std::nullopt;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    result = parseTrackRow(stmt);
  }

  sqlite3_finalize(stmt);
  return result;
}

std::vector<core::Track> SqliteDatabaseAdapter::getAllTracks() {
  std::vector<core::Track> list;
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_db)
    return list;

  const char *sql =
      "SELECT id, file_path, title, artist, album, album_artist, genre, year, "
      "track_number, disc_number, duration_ms, sample_rate, bit_depth, "
      "channels, bitrate, codec, art_hash, file_size, date_added FROM tracks "
      "ORDER BY album ASC, disc_number ASC, track_number ASC, title ASC;";
  sqlite3_stmt *stmt = nullptr;
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return list;

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    list.push_back(parseTrackRow(stmt));
  }

  sqlite3_finalize(stmt);
  return list;
}

std::vector<core::Track>
SqliteDatabaseAdapter::getTracksByAlbum(const std::string &albumTitle,
                                        const std::string &albumArtist) {
  std::vector<core::Track> list;
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_db)
    return list;

  std::string sql;
  bool bindArtist = false;

  if (m_groupAlbumsByTitle) {
    if (albumTitle.empty() || albumTitle == "Unknown Album") {
      sql = "SELECT id, file_path, title, artist, album, album_artist, genre, "
            "year, track_number, disc_number, duration_ms, sample_rate, "
            "bit_depth, channels, bitrate, codec, art_hash, file_size, "
            "date_added FROM tracks WHERE (album = '' OR album = 'Unknown "
            "Album') "
            "AND artist = ? ORDER BY disc_number ASC, track_number ASC, title "
            "ASC;";
      bindArtist = true;
    } else {
      sql = "SELECT id, file_path, title, artist, album, album_artist, genre, "
            "year, track_number, disc_number, duration_ms, sample_rate, "
            "bit_depth, channels, bitrate, codec, art_hash, file_size, "
            "date_added FROM tracks WHERE album = ? ORDER BY disc_number ASC, "
            "track_number ASC, title ASC;";
    }
  } else {
    if (!albumArtist.empty() && albumArtist != "Unknown Artist" &&
        albumArtist != "Various Artists") {
      sql =
          "SELECT id, file_path, title, artist, album, album_artist, genre, "
          "year, track_number, disc_number, duration_ms, sample_rate, "
          "bit_depth, channels, bitrate, codec, art_hash, file_size, "
          "date_added FROM tracks WHERE album = ? AND (album_artist = ? OR "
          "artist = ?) ORDER BY disc_number ASC, track_number ASC, title ASC;";
      bindArtist = true;
    } else {
      sql = "SELECT id, file_path, title, artist, album, album_artist, genre, "
            "year, track_number, disc_number, duration_ms, sample_rate, "
            "bit_depth, channels, bitrate, codec, art_hash, file_size, "
            "date_added FROM tracks WHERE album = ? ORDER BY disc_number ASC, "
            "track_number ASC, title ASC;";
    }
  }

  sqlite3_stmt *stmt = nullptr;
  if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    return list;

  if (m_groupAlbumsByTitle) {
    if (bindArtist) {
      sqlite3_bind_text(stmt, 1, albumArtist.c_str(), -1, SQLITE_TRANSIENT);
    } else {
      sqlite3_bind_text(stmt, 1, albumTitle.c_str(), -1, SQLITE_TRANSIENT);
    }
  } else {
    sqlite3_bind_text(stmt, 1, albumTitle.c_str(), -1, SQLITE_TRANSIENT);
    if (bindArtist) {
      sqlite3_bind_text(stmt, 2, albumArtist.c_str(), -1, SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 3, albumArtist.c_str(), -1, SQLITE_TRANSIENT);
    }
  }

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    list.push_back(parseTrackRow(stmt));
  }

  sqlite3_finalize(stmt);
  return list;
}

bool SqliteDatabaseAdapter::deleteTrack(const std::string &id) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_db)
    return false;

  const char *sql = "DELETE FROM tracks WHERE id = ?;";
  sqlite3_stmt *stmt = nullptr;
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;

  sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
  int rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return (rc == SQLITE_DONE);
}

std::vector<core::Album> SqliteDatabaseAdapter::getAllAlbums() {
  std::vector<core::Album> albums;
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_db)
    return albums;

  const char *sql = nullptr;
  if (m_groupAlbumsByTitle) {
    sql = R"(
        SELECT 
            album,
            MAX(NULLIF(NULLIF(album_artist, ''), 'Unknown Artist')) AS explicit_album_artist,
            COUNT(DISTINCT artist) AS distinct_artist_count,
            MAX(artist) AS sample_artist,
            MAX(year) AS release_year,
            MAX(genre) AS main_genre,
            COUNT(*) AS track_count,
            SUM(duration_ms) AS total_duration,
            MAX(art_hash) AS cover_art,
            MAX(sample_rate) AS max_sr,
            MAX(bit_depth) AS max_bd,
            MAX(codec) AS main_codec
        FROM tracks
        GROUP BY album, (CASE WHEN album = '' OR album = 'Unknown Album' THEN artist ELSE '' END)
        ORDER BY album ASC;
    )";
  } else {
    sql = R"(
        SELECT 
            album,
            CASE 
                WHEN NULLIF(NULLIF(album_artist, ''), 'Unknown Artist') IS NOT NULL 
                    THEN album_artist
                WHEN COUNT(DISTINCT artist) > 1 
                    THEN 'Various Artists'
                ELSE MAX(artist)
            END AS effective_artist,
            1 AS distinct_artist_count,
            MAX(artist) AS sample_artist,
            MAX(year) AS release_year,
            MAX(genre) AS main_genre,
            COUNT(*) AS track_count,
            SUM(duration_ms) AS total_duration,
            MAX(art_hash) AS cover_art,
            MAX(sample_rate) AS max_sr,
            MAX(bit_depth) AS max_bd,
            MAX(codec) AS main_codec
        FROM tracks
        GROUP BY album, COALESCE(NULLIF(NULLIF(album_artist, ''), 'Unknown Artist'), artist)
        ORDER BY album ASC;
    )";
  }

  sqlite3_stmt *stmt = nullptr;
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return albums;

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    core::Album a;
    a.title = reinterpret_cast<const char *>(
        sqlite3_column_text(stmt, 0)
            ? sqlite3_column_text(stmt, 0)
            : reinterpret_cast<const unsigned char *>(""));

    std::string effectiveArtist;
    if (!m_groupAlbumsByTitle) {
      effectiveArtist = reinterpret_cast<const char *>(
          sqlite3_column_text(stmt, 1)
              ? sqlite3_column_text(stmt, 1)
              : reinterpret_cast<const unsigned char *>(""));
    } else {
      const char *expAlbumArtist =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
      int distinctArtistCount = sqlite3_column_int(stmt, 2);
      const char *sampleArtist = reinterpret_cast<const char *>(
          sqlite3_column_text(stmt, 3)
              ? sqlite3_column_text(stmt, 3)
              : reinterpret_cast<const unsigned char *>(""));

      if (expAlbumArtist && expAlbumArtist[0] != '\0') {
        effectiveArtist = expAlbumArtist;
      } else if (distinctArtistCount <= 1) {
        effectiveArtist = sampleArtist ? sampleArtist : "";
      } else {
        const char *artSql = "SELECT artist, COUNT(*) FROM tracks WHERE album "
                             "= ? GROUP BY artist ORDER BY COUNT(*) DESC;";
        sqlite3_stmt *artStmt = nullptr;
        std::unordered_map<std::string, int> baseArtistCounts;
        std::string topExactArtist;
        int topExactCount = 0;
        int totalAlbumTracks = 0;

        if (sqlite3_prepare_v2(m_db, artSql, -1, &artStmt, nullptr) ==
            SQLITE_OK) {
          sqlite3_bind_text(artStmt, 1, a.title.c_str(), -1, SQLITE_TRANSIENT);
          while (sqlite3_step(artStmt) == SQLITE_ROW) {
            const char *aText =
                reinterpret_cast<const char *>(sqlite3_column_text(artStmt, 0));
            int count = sqlite3_column_int(artStmt, 1);
            totalAlbumTracks += count;
            if (aText) {
              std::string exactArtist(aText);
              if (topExactArtist.empty()) {
                topExactArtist = exactArtist;
                topExactCount = count;
              }
              std::string base = extractBaseArtist(exactArtist);
              baseArtistCounts[base] += count;
            }
          }
          sqlite3_finalize(artStmt);
        }

        std::string bestBaseArtist;
        int bestBaseCount = 0;
        for (const auto &[base, cnt] : baseArtistCounts) {
          if (cnt > bestBaseCount) {
            bestBaseCount = cnt;
            bestBaseArtist = base;
          }
        }

        if (!bestBaseArtist.empty() && (bestBaseCount * 2 >= totalAlbumTracks ||
                                        baseArtistCounts.size() == 1)) {
          effectiveArtist = bestBaseArtist;
        } else if (topExactCount * 2 > totalAlbumTracks) {
          effectiveArtist = topExactArtist;
        } else {
          effectiveArtist = "Various Artists";
        }
      }
    }

    a.artist = effectiveArtist;
    a.id = "alb_" + a.title + "_" + a.artist;
    a.year = static_cast<uint32_t>(sqlite3_column_int(stmt, 4));
    a.genre = reinterpret_cast<const char *>(
        sqlite3_column_text(stmt, 5)
            ? sqlite3_column_text(stmt, 5)
            : reinterpret_cast<const unsigned char *>(""));
    a.trackCount = static_cast<uint32_t>(sqlite3_column_int(stmt, 6));
    a.totalDurationMs = static_cast<uint64_t>(sqlite3_column_int64(stmt, 7));
    a.artHash = reinterpret_cast<const char *>(
        sqlite3_column_text(stmt, 8)
            ? sqlite3_column_text(stmt, 8)
            : reinterpret_cast<const unsigned char *>(""));
    a.maxSampleRate = static_cast<uint32_t>(sqlite3_column_int(stmt, 9));
    a.maxBitDepth = static_cast<uint8_t>(sqlite3_column_int(stmt, 10));
    a.primaryCodec = reinterpret_cast<const char *>(
        sqlite3_column_text(stmt, 11)
            ? sqlite3_column_text(stmt, 11)
            : reinterpret_cast<const unsigned char *>("FLAC"));

    if (a.maxSampleRate >= 88200 || a.maxBitDepth >= 24) {
      a.qualityBadge = "Hi-Res Lossless";
    } else if (a.primaryCodec == "DSD") {
      a.qualityBadge = "DSD";
    } else if (a.primaryCodec == "FLAC" || a.primaryCodec == "WAV" ||
               a.primaryCodec == "ALAC" || a.primaryCodec == "AIFF") {
      a.qualityBadge = "Lossless";
    } else {
      a.qualityBadge = "Standard";
    }

    albums.push_back(a);
  }

  sqlite3_finalize(stmt);
  return albums;
}

std::optional<core::Album>
SqliteDatabaseAdapter::getAlbumById(const std::string &id) {
  auto all = getAllAlbums();
  for (const auto &a : all) {
    if (a.id == id)
      return a;
  }
  for (const auto &a : all) {
    if (id.rfind("alb_" + a.title + "_", 0) == 0 || id == ("alb_" + a.title))
      return a;
  }
  return std::nullopt;
}

bool SqliteDatabaseAdapter::clearLibrary() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_db)
    return false;
  char *err = nullptr;
  int rc = sqlite3_exec(m_db, "DELETE FROM tracks;", nullptr, nullptr, &err);
  if (rc != SQLITE_OK) {
    if (err)
      sqlite3_free(err);
    return false;
  }
  return true;
}

bool SqliteDatabaseAdapter::optimizeDatabase() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_db)
    return false;
  char *err = nullptr;
  int rc = sqlite3_exec(m_db, "VACUUM; REINDEX;", nullptr, nullptr, &err);
  if (rc != SQLITE_OK) {
    if (err)
      sqlite3_free(err);
    return false;
  }
  return true;
}

size_t SqliteDatabaseAdapter::getTrackCount() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_db)
    return 0;
  const char *sql = "SELECT COUNT(*) FROM tracks;";
  sqlite3_stmt *stmt = nullptr;
  if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return 0;
  size_t count = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    count = static_cast<size_t>(sqlite3_column_int64(stmt, 0));
  }
  sqlite3_finalize(stmt);
  return count;
}

size_t SqliteDatabaseAdapter::getAlbumCount() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_db)
    return 0;

  std::string sql;
  if (m_groupAlbumsByTitle) {
    sql = "SELECT COUNT(DISTINCT CASE "
          "WHEN album = '' OR album = 'Unknown Album' THEN (album || '::' || "
          "artist) "
          "ELSE album END) FROM tracks;";
  } else {
    sql = "SELECT COUNT(DISTINCT album || '::' || "
          "COALESCE(NULLIF(NULLIF(album_artist, ''), 'Unknown "
          "Artist'), artist)) FROM tracks;";
  }

  sqlite3_stmt *stmt = nullptr;
  if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    return 0;
  size_t count = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    count = static_cast<size_t>(sqlite3_column_int64(stmt, 0));
  }
  sqlite3_finalize(stmt);
  return count;
}

void SqliteDatabaseAdapter::setGroupAlbumsByTitle(bool enabled) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_groupAlbumsByTitle = enabled;
}

bool SqliteDatabaseAdapter::groupAlbumsByTitle() const {
  return m_groupAlbumsByTitle;
}

} // namespace adapters
