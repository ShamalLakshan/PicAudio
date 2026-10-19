#include "core/services/LibraryService.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace core {

LibraryService::LibraryService(IMusicDatabasePort &dbPort,
                               IMetadataExtractor &metadataExtractor)
    : m_dbPort(dbPort), m_metadataExtractor(metadataExtractor) {}

size_t LibraryService::scanDirectory(
    const std::string &directoryPath,
    std::function<void(size_t, size_t)> progressCallback) {
  if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
    return 0;
  }

  // Collect all supported audio files first
  std::vector<std::string> audioFiles;
  for (const auto &entry : fs::recursive_directory_iterator(
           directoryPath, fs::directory_options::skip_permission_denied)) {
    if (entry.is_regular_file()) {
      std::string ext = entry.path().extension().string();
      if (m_metadataExtractor.supportsFormat(ext)) {
        audioFiles.push_back(entry.path().string());
      }
    }
  }

  const size_t totalFiles = audioFiles.size();
  if (totalFiles == 0) {
    return 0;
  }

  // Extract tags and flush to DB in batches of 50
  std::vector<Track> batch;
  batch.reserve(50);
  size_t scannedCount = 0;

  for (const auto &file : audioFiles) {
    auto trackOpt = m_metadataExtractor.extract(file);
    if (trackOpt.has_value()) {
      batch.push_back(trackOpt.value());
      if (batch.size() >= 50) {
        m_dbPort.saveTracks(batch);
        batch.clear();
      }
    }
    scannedCount++;
    if (progressCallback &&
        (scannedCount % 10 == 0 || scannedCount == totalFiles)) {
      progressCallback(scannedCount, totalFiles);
    }
  }

  if (!batch.empty()) {
    m_dbPort.saveTracks(batch);
  }

  return scannedCount;
}

size_t LibraryService::incrementalQuickScan(
    const std::string &directoryPath,
    std::function<void(size_t, size_t)> progressCallback) {
  if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
    return 0;
  }

  std::vector<std::string> audioFiles;
  for (const auto &entry : fs::recursive_directory_iterator(
           directoryPath, fs::directory_options::skip_permission_denied)) {
    if (entry.is_regular_file()) {
      std::string ext = entry.path().extension().string();
      if (m_metadataExtractor.supportsFormat(ext)) {
        audioFiles.push_back(entry.path().string());
      }
    }
  }

  const size_t totalFiles = audioFiles.size();
  if (totalFiles == 0) {
    return 0;
  }

  std::vector<Track> batch;
  batch.reserve(50);
  size_t scannedCount = 0;
  size_t newlyAdded = 0;

  for (const auto &file : audioFiles) {
    if (!m_dbPort.getTrackByPath(file).has_value()) {
      auto trackOpt = m_metadataExtractor.extract(file);
      if (trackOpt.has_value()) {
        batch.push_back(trackOpt.value());
        if (batch.size() >= 50) {
          m_dbPort.saveTracks(batch);
          batch.clear();
        }
        newlyAdded++;
      }
    }
    scannedCount++;
    if (progressCallback &&
        (scannedCount % 10 == 0 || scannedCount == totalFiles)) {
      progressCallback(scannedCount, totalFiles);
    }
  }

  if (!batch.empty()) {
    m_dbPort.saveTracks(batch);
  }

  return newlyAdded;
}

std::vector<Album> LibraryService::getAlbums() {
  return m_dbPort.getAllAlbums();
}

std::vector<Track> LibraryService::getTracks() {
  return m_dbPort.getAllTracks();
}

std::vector<Track>
LibraryService::getTracksForAlbum(const std::string &albumTitle,
                                  const std::string &albumArtist) {
  return m_dbPort.getTracksByAlbum(albumTitle, albumArtist);
}

std::optional<Track> LibraryService::getTrackById(const std::string &id) {
  return m_dbPort.getTrackById(id);
}

std::optional<Album> LibraryService::getAlbumById(const std::string &id) {
  return m_dbPort.getAlbumById(id);
}

void LibraryService::setGroupAlbumsByTitle(bool enabled) {
  m_dbPort.setGroupAlbumsByTitle(enabled);
}

bool LibraryService::groupAlbumsByTitle() const {
  return m_dbPort.groupAlbumsByTitle();
}

void LibraryService::seedSampleAudiophileLibrary() {
  // Deprecated: No-op. Real music scanning is used.
}

size_t LibraryService::purgeNonExistentTracks() {
  auto allTracks = m_dbPort.getAllTracks();
  size_t purgedCount = 0;
  for (const auto &track : allTracks) {
    if (!fs::exists(track.filePath)) {
      m_dbPort.deleteTrack(track.id);
      purgedCount++;
    }
  }
  return purgedCount;
}

bool LibraryService::clearLibrary() { return m_dbPort.clearLibrary(); }

bool LibraryService::optimizeDatabase() { return m_dbPort.optimizeDatabase(); }

} // namespace core
