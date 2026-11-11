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

  // Ratings, Favorites & Play Statistics (Phase 4)
  bool updateTrackRating(const std::string &id, uint8_t rating) override;
  bool updateTrackFavorite(const std::string &id, bool isFavorite) override;
  bool recordTrackPlayed(const std::string &id, uint64_t timestamp) override;
  bool recordTrackSkipped(const std::string &id) override;
  std::vector<std::pair<core::Track, uint64_t>>
  getListeningHistory(size_t limit = 200) override;
  bool clearListeningHistory() override;

  // Library Health & Relocation (Phase 4)
  std::vector<core::Track> getMissingTracks() override;
  size_t relocateTracks(const std::string &oldPrefix,
                        const std::string &newPrefix) override;

  // Dynamic Rule Query (Phase 4)
  std::vector<core::Track> queryTracksDynamic(const std::string &whereClause,
                                              const std::string &orderBy = "",
                                              size_t limit = 0) override;

private:
  bool createTables();
  core::Track parseTrackRow(sqlite3_stmt *stmt);

  sqlite3 *m_db{nullptr};
  std::mutex m_mutex;
  std::string m_dbPath;
  bool m_groupAlbumsByTitle{true};
};

} // namespace adapters
