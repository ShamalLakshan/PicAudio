#pragma once

#include "core/entities/Track.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace adapters {

/**
 * @brief Audiophile CUE sheet parser supporting TRACK, INDEX 00/01, TITLE,
 * PERFORMER, SONGWRITER, embedded CUE sheets, and accurate frame-to-millisecond
 * conversion.
 */
class CueSheetParser {
public:
  /**
   * @brief Parses an external .cue file into discrete virtual tracks.
   * @param cueFilePath Absolute path to the .cue sheet file.
   * @param audioFileDurationMs Optional known duration of the underlying audio
   * file.
   * @return Vector of parsed virtual tracks.
   */
  static std::vector<core::Track> parseFile(const std::string &cueFilePath,
                                            uint64_t audioFileDurationMs = 0);

  /**
   * @brief Parses CUE sheet textual content (e.g. from embedded tags) into
   * virtual tracks.
   * @param cueContent Raw CUE sheet text content.
   * @param baseAudioPath Path to the companion or container audio file.
   * @param audioFileDurationMs Known duration of the companion audio file.
   * @return Vector of parsed virtual tracks.
   */
  static std::vector<core::Track> parseString(const std::string &cueContent,
                                              const std::string &baseAudioPath,
                                              uint64_t audioFileDurationMs = 0);

  /**
   * @brief Converts a CUE MM:SS:FF timestamp string into milliseconds.
   * @param timestamp String in "MM:SS:FF" format (FF = 75 frames per second).
   * @return Milliseconds timestamp.
   */
  static uint64_t parseIndexTimestampMs(const std::string &timestamp);
};

} // namespace adapters
