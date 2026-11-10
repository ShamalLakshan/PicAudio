#pragma once

#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

namespace core {

/**
 * @brief Represents an item entry in a static playlist.
 */
struct PlaylistItem {
  std::string filePath;
  std::string title;
  std::string artist;
  std::string album;
  uint64_t durationMs{0};
};

/**
 * @brief Represents a static audio playlist.
 */
struct StaticPlaylist {
  std::string id;
  std::string name;
  std::vector<PlaylistItem> items;
};

/**
 * @brief Supported static playlist file formats.
 */
enum class PlaylistFormat { M3U, M3U8, PLS, XSPF, Unknown };

/**
 * @brief Domain service for parsing, writing, and manipulating static
 * playlists.
 */
class PlaylistService {
public:
  PlaylistService() = default;
  ~PlaylistService() = default;

  /**
   * @brief Detects playlist format by file extension.
   * @param filePath Path or filename.
   * @return Detected PlaylistFormat.
   */
  [[nodiscard]] static PlaylistFormat detectFormat(const std::string &filePath);

  /**
   * @brief Imports a playlist file (M3U, M3U8, PLS, XSPF) from disk.
   * @param filePath Full path to the playlist file.
   * @return Parsed StaticPlaylist or std::nullopt on failure.
   */
  [[nodiscard]] static std::optional<StaticPlaylist>
  importPlaylist(const std::string &filePath);

  /**
   * @brief Exports a playlist to disk in the specified format.
   * @param playlist StaticPlaylist structure to export.
   * @param targetFilePath Destination file path.
   * @param format Playlist format to write.
   * @param useRelativePaths If true, uses relative file paths when possible.
   * @return True if write succeeded.
   */
  static bool exportPlaylist(const StaticPlaylist &playlist,
                             const std::string &targetFilePath,
                             PlaylistFormat format,
                             bool useRelativePaths = true);

  /**
   * @brief Reorders an item within a playlist.
   * @param playlist Playlist to modify.
   * @param fromIndex Original position index.
   * @param toIndex Target position index.
   * @return True if moved successfully.
   */
  static bool moveItem(StaticPlaylist &playlist, size_t fromIndex,
                       size_t toIndex);

  /**
   * @brief Computes a relative path from a base directory to a target file.
   */
  [[nodiscard]] static std::string
  makeRelativePath(const std::string &targetFilePath,
                   const std::string &baseDir);

  /**
   * @brief Resolves a possibly relative path against a base directory.
   */
  [[nodiscard]] static std::string resolvePath(const std::string &path,
                                               const std::string &baseDir);

  // Format-specific stream parsers and writers
  static bool parseM3U(std::istream &in, const std::string &baseDir,
                       StaticPlaylist &outPlaylist);
  static bool writeM3U(std::ostream &out, const StaticPlaylist &playlist,
                       const std::string &baseDir, bool useRelative);

  static bool parsePLS(std::istream &in, const std::string &baseDir,
                       StaticPlaylist &outPlaylist);
  static bool writePLS(std::ostream &out, const StaticPlaylist &playlist,
                       const std::string &baseDir, bool useRelative);

  static bool parseXSPF(std::istream &in, const std::string &baseDir,
                        StaticPlaylist &outPlaylist);
  static bool writeXSPF(std::ostream &out, const StaticPlaylist &playlist,
                        const std::string &baseDir, bool useRelative);
};

} // namespace core
