#pragma once

#include "core/ports/IMetadataExtractor.hpp"
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace TagLib {
namespace ID3v2 {
class Tag;
}
namespace Ogg {
class XiphComment;
}
namespace APE {
class Tag;
}
} // namespace TagLib

namespace adapters {

/**
 * @brief TagLib adapter for reading FLAC/WAV/MP3/M4A/OGG/OPUS/DSF tags and
 * embedded/folder artwork.
 */
class TagLibMetadataAdapter : public core::IMetadataExtractor {
public:
  TagLibMetadataAdapter();
  ~TagLibMetadataAdapter() override = default;

  std::optional<core::Track> extract(const std::string &filePath) override;
  std::optional<core::ExtractedArtwork>
  extractArtwork(const std::string &filePath) override;
  bool supportsFormat(const std::string &extension) const override;
  std::string extractLyrics(const std::string &filePath) override;
  void setArtworkPriority(bool preferEmbedded) override;

  /**
   * @brief Computes 64-bit FNV-1a hash over raw binary buffer.
   */
  static std::string computeHash(const uint8_t *data, size_t size);

  /**
   * @brief Identifies image MIME type from magic header bytes.
   */
  static std::string detectMimeType(const uint8_t *data, size_t size);

private:
  std::string getFileExtension(const std::string &filePath) const;
  static std::string toLower(std::string s);

  std::optional<core::ExtractedArtwork>
  extractEmbeddedArtwork(const std::string &filePath);

  std::optional<core::ExtractedArtwork>
  findFolderArtwork(const std::string &filePath);

  std::optional<core::ExtractedArtwork>
  extractFromId3v2(TagLib::ID3v2::Tag *id3v2);

  std::optional<core::ExtractedArtwork>
  extractFromXiphComment(TagLib::Ogg::XiphComment *comment);

  std::optional<core::ExtractedArtwork>
  extractFromApe(TagLib::APE::Tag *apeTag);

  bool m_preferEmbedded{true};
  std::mutex m_cacheMutex;
  std::unordered_map<std::string, std::optional<core::ExtractedArtwork>>
      m_folderArtCache;
};

} // namespace adapters
