#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace core {

/**
 * @brief Represents a single audio track.
 */
struct Track {
  std::string id;
  std::string filePath;
  std::string title;
  std::string artist;
  std::string album;
  std::string albumArtist;
  std::string genre;
  uint32_t year{0};
  uint32_t trackNumber{0};
  uint32_t discNumber{1};
  uint64_t durationMs{0};

  // Audio stream specs
  uint32_t sampleRate{44100}; ///< in Hz (44100, 96000, 192000...)
  uint8_t bitDepth{16};       ///< 16, 24, 32-bit
  uint8_t channels{2};        ///< 1: mono, 2: stereo, 6: 5.1
  uint32_t bitrate{0};        ///< kbps (e.g. 919)
  std::string codec{"FLAC"};  ///< FLAC, WAV, ALAC, DSD, MP3...

  // Cover art & file stats
  std::string artHash; ///< Hash used to fetch cached cover image
  uint64_t fileSizeBytes{0};
  uint64_t dateAdded{0}; ///< Unix timestamp in seconds

  // Ratings, favorites & listening statistics (Phase 4)
  uint8_t rating{0};      ///< 0 to 5 stars
  bool isFavorite{false}; ///< True if marked as favorite
  uint32_t playCount{0};  ///< Number of plays satisfying scrobble threshold
  uint32_t skipCount{0};  ///< Number of skips before 30 seconds
  uint64_t lastPlayed{0}; ///< Unix timestamp in seconds of last playback
  std::string comment;    ///< Metadata tag comment

  // ReplayGain 2.0 & EBU R128 loudness tags
  float replayGainTrackGain{0.0f}; ///< in dB (e.g. -6.5 dB)
  float replayGainTrackPeak{1.0f}; ///< Normalized linear peak (e.g. 0.98)
  float replayGainAlbumGain{0.0f}; ///< in dB
  float replayGainAlbumPeak{1.0f}; ///< Normalized linear peak
  bool hasReplayGain{false};

  // Virtual track bounds (CUE sheets & multi-track containers)
  uint64_t startTimeMs{0};    ///< Virtual track start offset in milliseconds
  uint64_t endTimeMs{0};      ///< Virtual track end boundary (0 if full file)
  bool isVirtualTrack{false}; ///< True if this track maps to a CUE sub-index

  /**
   * @brief Formats duration into MM:SS format.
   */
  [[nodiscard]] std::string durationFormatted() const {
    const uint64_t totalSeconds = durationMs / 1000;
    const uint64_t minutes = totalSeconds / 60;
    const uint64_t seconds = totalSeconds % 60;
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02u:%02u", static_cast<unsigned>(minutes),
                  static_cast<unsigned>(seconds));
    return std::string(buf);
  }

  /**
   * @brief Returns a friendly quality badge string, like "FLAC • 24-bit/96kHz".
   */
  [[nodiscard]] std::string audiophileBadge() const {
    std::string badge = codec;
    if (sampleRate > 0) {
      badge += " • " + std::to_string(bitDepth) + "-bit/" +
               std::to_string(sampleRate / 1000) + "kHz";
    }
    return badge;
  }
};

} // namespace core
