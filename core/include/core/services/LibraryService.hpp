#pragma once

#include "core/entities/Album.hpp"
#include "core/entities/Track.hpp"
#include "core/ports/IMetadataExtractor.hpp"
#include "core/ports/IMusicDatabasePort.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace core {

/**
 * @brief Manages music library scanning, demo seeding, and track retrieval.
 */
class LibraryService {
public:
  LibraryService(IMusicDatabasePort &dbPort,
                 IMetadataExtractor &metadataExtractor);
  ~LibraryService() = default;

  /**
   * @brief Recursively scans a directory for music files and adds them to DB.
   * @param directoryPath Path on local filesystem.
   * @param progressCallback Optional callback with (scanned, total).
   * @return Number of tracks indexed.
   */
  size_t scanDirectory(const std::string &directoryPath,
                       std::function<void(size_t scanned, size_t total)>
                           progressCallback = nullptr);

  /**
   * @brief Incrementally scans a directory, skipping files already indexed in
   * DB.
   * @param directoryPath Path on local filesystem.
   * @param progressCallback Optional callback with (scanned, total).
   * @return Number of newly added tracks.
   */
  size_t incrementalQuickScan(const std::string &directoryPath,
                              std::function<void(size_t scanned, size_t total)>
                                  progressCallback = nullptr);

  /** @brief Seeds demo tracks (deprecated, no-op). */
  void seedSampleAudiophileLibrary();

  /** @brief Removes any tracks from the database whose files no longer exist on
   * disk. */
  size_t purgeNonExistentTracks();

  /** @brief Clears all indexed tracks from the database. */
  bool clearLibrary();

  /** @brief Optimizes storage structures and indexes. */
  bool optimizeDatabase();

  // Query helpers
  std::vector<Album> getAlbums();
  std::vector<Track> getTracks();
  std::vector<Track> getTracksForAlbum(const std::string &albumTitle,
                                       const std::string &albumArtist);
  std::optional<Track> getTrackById(const std::string &id);
  std::optional<Album> getAlbumById(const std::string &id);
  void setGroupAlbumsByTitle(bool enabled);
  bool groupAlbumsByTitle() const;

  // Phase 4 Extensions
  bool updateTrackRating(const std::string &id, uint8_t rating);
  bool updateTrackFavorite(const std::string &id, bool isFavorite);
  bool recordTrackPlayed(const std::string &id, uint64_t timestamp);
  bool recordTrackSkipped(const std::string &id);
  std::vector<std::pair<Track, uint64_t>>
  getListeningHistory(size_t limit = 200);
  bool clearListeningHistory();
  std::vector<Track> getMissingTracks();
  size_t relocateTracks(const std::string &oldPrefix,
                        const std::string &newPrefix);
  std::vector<Track> queryTracksDynamic(const std::string &whereClause,
                                        const std::string &orderBy = "",
                                        size_t limit = 0);

  IMusicDatabasePort &getDatabasePort() { return m_dbPort; }
  IMetadataExtractor &getMetadataExtractor() { return m_metadataExtractor; }

private:
  IMusicDatabasePort &m_dbPort;
  IMetadataExtractor &m_metadataExtractor;
};

} // namespace core
