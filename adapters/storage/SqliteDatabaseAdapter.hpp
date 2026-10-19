#pragma once

#include "core/ports/IMusicDatabasePort.hpp"
#include <memory>
#include <mutex>
#include <sqlite3.h>
#include <string>
#include <vector>

namespace adapters {

/**
 * @brief SQLite3 database adapter for storing and querying music library data.
 */
class SqliteDatabaseAdapter : public core::IMusicDatabasePort {
public:
  SqliteDatabaseAdapter();
  ~SqliteDatabaseAdapter() override;

  bool initialize(const std::string &dbPath) override;

  // Tracks
  bool saveTrack(const core::Track &track) override;
  bool saveTracks(const std::vector<core::Track> &tracks) override;
  std::optional<core::Track> getTrackById(const std::string &id) override;
  std::optional<core::Track> getTrackByPath(const std::string &path) override;
  std::vector<core::Track> getAllTracks() override;
  std::vector<core::Track>
  getTracksByAlbum(const std::string &albumTitle,
                   const std::string &albumArtist) override;
  bool deleteTrack(const std::string &id) override;

  // Albums
  std::vector<core::Album> getAllAlbums() override;
  std::optional<core::Album> getAlbumById(const std::string &id) override;
  void setGroupAlbumsByTitle(bool enabled) override;
  bool groupAlbumsByTitle() const override;

  // Library Maintenance
  bool clearLibrary() override;
  bool optimizeDatabase() override;
  size_t getTrackCount() override;
  size_t getAlbumCount() override;

private:
  bool createTables();
  core::Track parseTrackRow(sqlite3_stmt *stmt);

  sqlite3 *m_db{nullptr};
  std::mutex m_mutex;
  std::string m_dbPath;
  bool m_groupAlbumsByTitle{true};
};

} // namespace adapters
