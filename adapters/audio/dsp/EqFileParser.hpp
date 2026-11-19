#pragma once

#include "core/ports/IAudioEnginePort.hpp"
#include <string>
#include <vector>

namespace adapters {

/**
 * @brief Result of parsing an equalizer configuration file or string.
 */
struct ParsedEqData {
  bool success{false};
  std::string errorMessage;
  int mode{1}; // 0: Graphic, 1: Parametric
  float preampDb{0.0f};
  std::vector<core::PeqFilterBand> peqBands;
  std::vector<float> graphicGains;
  size_t graphicBandCount{10};
};

/**
 * @brief Robust parser and exporter for Equalizer APO / Peace GUI parametric
 * configurations, and Squiglink / Wavelet GraphicEQ configurations.
 */
class EqFileParser {
public:
  /**
   * @brief Parses an EQ configuration from text content.
   * Auto-detects Equalizer APO / Peace format or GraphicEQ format.
   * @param content Raw text content.
   * @return ParsedEqData containing extracted bands and preamp.
   */
  static ParsedEqData parse(const std::string &content);

  /**
   * @brief Reads and parses an EQ configuration from a file path.
   * @param filePath Filesystem path to the .txt or .peace file.
   * @return ParsedEqData containing extracted bands and preamp.
   */
  static ParsedEqData parseFile(const std::string &filePath);

  /**
   * @brief Exports parametric equalizer bands into standard Equalizer APO
   * / Peace GUI configuration format text.
   * @param bands List of parametric filter bands.
   * @param preampDb Master preamp gain in dB.
   * @param title Optional title or device descriptor comment.
   * @return Formatted configuration text.
   */
  static std::string
  exportEqualizerApo(const std::vector<core::PeqFilterBand> &bands,
                     float preampDb, const std::string &title = "");

  /**
   * @brief Interpolates a series of (frequency, gain) curve points onto
   * standard ISO 1/1, 2/3, or 1/3 octave graphic equalizer center frequencies.
   * @param curvePoints Pairs of (freqHz, gainDb).
   * @param targetCount Band count (10, 15, or 31).
   * @return Gains in dB for each standard band frequency.
   */
  static std::vector<float>
  interpolateGraphicEq(const std::vector<std::pair<float, float>> &curvePoints,
                       size_t targetCount);
};

} // namespace adapters
