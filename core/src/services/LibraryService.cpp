#include "core/services/LibraryService.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <unordered_set>

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

  // Collect all supported audio files and .cue files
  std::vector<std::string> audioFiles;
  std::vector<std::string> cueFiles;
  for (const auto &entry : fs::recursive_directory_iterator(
           directoryPath, fs::directory_options::skip_permission_denied)) {
    if (entry.is_regular_file()) {
      std::string ext = entry.path().extension().string();
      std::string lowerExt = ext;
      std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(),
                     [](unsigned char c) { return std::tolower(c); });
      if (lowerExt == ".cue") {
        cueFiles.push_back(entry.path().string());
      } else if (m_metadataExtractor.supportsFormat(ext)) {
        audioFiles.push_back(entry.path().string());
      }
    }
  }

  const size_t totalFiles = cueFiles.size() + audioFiles.size();
  if (totalFiles == 0) {
    return 0;
  }

  std::vector<Track> batch;
  batch.reserve(50);
  size_t scannedCount = 0;
  std::unordered_set<std::string> coveredAudioFiles;

  // Process CUE sheets first to split monolithic files into virtual tracks
  for (const auto &cueFile : cueFiles) {
    auto cueTracks = m_metadataExtractor.extractMultiple(cueFile);
    if (!cueTracks.empty()) {
      for (const auto &t : cueTracks) {
        batch.push_back(t);
        if (batch.size() >= 50) {
          m_dbPort.saveTracks(batch);
          batch.clear();
        }
      }

      std::string companion = cueTracks.front().filePath;
      auto tagPos = companion.find("#cue:");
      if (tagPos != std::string::npos) {
        companion = companion.substr(0, tagPos);
      }
      coveredAudioFiles.insert(companion);

      auto existing = m_dbPort.getTrackByPath(companion);
      if (existing.has_value() && !existing->isVirtualTrack) {
        m_dbPort.deleteTrack(existing->id);
      }
    }
    scannedCount++;
    if (progressCallback &&
        (scannedCount % 10 == 0 || scannedCount == totalFiles)) {
      progressCallback(scannedCount, totalFiles);
    }
  }

  // Process standalone audio files, skipping those already handled by CUE
  // sheets
  for (const auto &file : audioFiles) {
    if (coveredAudioFiles.find(file) != coveredAudioFiles.end()) {
      scannedCount++;
      continue;
    }
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
  std::vector<std::string> cueFiles;
  for (const auto &entry : fs::recursive_directory_iterator(
           directoryPath, fs::directory_options::skip_permission_denied)) {
    if (entry.is_regular_file()) {
      std::string ext = entry.path().extension().string();
      std::string lowerExt = ext;
      std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(),
                     [](unsigned char c) { return std::tolower(c); });
      if (lowerExt == ".cue") {
        cueFiles.push_back(entry.path().string());
      } else if (m_metadataExtractor.supportsFormat(ext)) {
        audioFiles.push_back(entry.path().string());
      }
    }
  }

  const size_t totalFiles = cueFiles.size() + audioFiles.size();
  if (totalFiles == 0) {
    return 0;
  }

  std::vector<Track> batch;
  batch.reserve(50);
  size_t scannedCount = 0;
  size_t newlyAdded = 0;
  std::unordered_set<std::string> coveredAudioFiles;

  // Process CUE sheets first
  for (const auto &cueFile : cueFiles) {
    auto cueTracks = m_metadataExtractor.extractMultiple(cueFile);
    if (!cueTracks.empty()) {
      std::string companion = cueTracks.front().filePath;
      auto tagPos = companion.find("#cue:");
      if (tagPos != std::string::npos) {
        companion = companion.substr(0, tagPos);
      }
      coveredAudioFiles.insert(companion);

      auto existing = m_dbPort.getTrackByPath(companion);
      if (existing.has_value() && !existing->isVirtualTrack) {
        m_dbPort.deleteTrack(existing->id);
      }

      for (const auto &t : cueTracks) {
        if (!m_dbPort.getTrackByPath(t.filePath).has_value()) {
          batch.push_back(t);
          if (batch.size() >= 50) {
            m_dbPort.saveTracks(batch);
            batch.clear();
          }
          newlyAdded++;
        }
      }
    }
    scannedCount++;
    if (progressCallback &&
        (scannedCount % 10 == 0 || scannedCount == totalFiles)) {
      progressCallback(scannedCount, totalFiles);
    }
  }

  // Process standalone audio files, skipping covered companion files
  for (const auto &file : audioFiles) {
    if (coveredAudioFiles.find(file) != coveredAudioFiles.end()) {
      scannedCount++;
      continue;
    }
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

bool LibraryService::updateTrackRating(const std::string &id, uint8_t rating) {
  return m_dbPort.updateTrackRating(id, rating);
}

bool LibraryService::updateTrackFavorite(const std::string &id,
                                         bool isFavorite) {
  return m_dbPort.updateTrackFavorite(id, isFavorite);
}

bool LibraryService::recordTrackPlayed(const std::string &id,
                                       uint64_t timestamp) {
  return m_dbPort.recordTrackPlayed(id, timestamp);
}

bool LibraryService::recordTrackSkipped(const std::string &id) {
  return m_dbPort.recordTrackSkipped(id);
}

std::vector<std::pair<Track, uint64_t>>
LibraryService::getListeningHistory(size_t limit) {
  return m_dbPort.getListeningHistory(limit);
}

bool LibraryService::clearListeningHistory() {
  return m_dbPort.clearListeningHistory();
}

std::vector<Track> LibraryService::getMissingTracks() {
  return m_dbPort.getMissingTracks();
}

size_t LibraryService::relocateTracks(const std::string &oldPrefix,
                                      const std::string &newPrefix) {
  return m_dbPort.relocateTracks(oldPrefix, newPrefix);
}

std::vector<Track>
LibraryService::queryTracksDynamic(const std::string &whereClause,
                                   const std::string &orderBy, size_t limit) {
  return m_dbPort.queryTracksDynamic(whereClause, orderBy, limit);
}

} // namespace core
