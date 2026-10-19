#pragma once

#include "core/entities/Album.hpp"
#include "core/entities/Track.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace core {

/**
 * @brief Storage interface for tracks and albums in the music library.
 */
class IMusicDatabasePort {
public:
  virtual ~IMusicDatabasePort() = default;

  /** @brief Opens or creates the database file. */
  virtual bool initialize(const std::string &dbPath) = 0;

  // Tracks
  virtual bool saveTrack(const Track &track) = 0;
  virtual bool saveTracks(const std::vector<Track> &tracks) = 0;
  virtual std::optional<Track> getTrackById(const std::string &id) = 0;
  virtual std::optional<Track> getTrackByPath(const std::string &path) = 0;
  virtual std::vector<Track> getAllTracks() = 0;
  virtual std::vector<Track>
  getTracksByAlbum(const std::string &albumTitle,
                   const std::string &albumArtist) = 0;
  virtual bool deleteTrack(const std::string &id) = 0;

  // Albums
  virtual std::vector<Album> getAllAlbums() = 0;
  virtual std::optional<Album> getAlbumById(const std::string &id) = 0;
  virtual void setGroupAlbumsByTitle(bool enabled) = 0;
  virtual bool groupAlbumsByTitle() const = 0;

  // Library Maintenance
  virtual bool clearLibrary() = 0;
  virtual bool optimizeDatabase() = 0;
  virtual size_t getTrackCount() = 0;
  virtual size_t getAlbumCount() = 0;
};

} // namespace core
