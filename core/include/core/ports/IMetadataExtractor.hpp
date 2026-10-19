#pragma once

#include "core/entities/Track.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace core {

/**
 * @brief Raw embedded album art binary buffer.
 */
struct ExtractedArtwork {
  std::vector<uint8_t> data;
  std::string mimeType{"image/jpeg"};
  std::string artHash;
};

/**
 * @brief Tag reader interface for extracting track metadata and cover art.
 */
class IMetadataExtractor {
public:
  virtual ~IMetadataExtractor() = default;

  /** @brief Reads audio tags and stream properties from a file. */
  virtual std::optional<Track> extract(const std::string &filePath) = 0;

  /** @brief Extracts embedded cover art from a file if present. */
  virtual std::optional<ExtractedArtwork>
  extractArtwork(const std::string &filePath) = 0;

  /** @brief Checks if a file extension (e.g. ".flac", ".wav") is supported. */
  virtual bool supportsFormat(const std::string &extension) const = 0;

  /** @brief Extracts embedded lyrics from an audio file if present. */
  virtual std::string extractLyrics(const std::string &filePath) { return ""; }

  /**
   * @brief Configures artwork discovery priority between embedded tags and
   * folder files.
   * @param preferEmbedded If true, searches embedded tags first; otherwise
   * folder images.
   */
  virtual void setArtworkPriority(bool preferEmbedded) { (void)preferEmbedded; }
};

} // namespace core
