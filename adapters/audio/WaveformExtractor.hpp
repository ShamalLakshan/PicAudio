#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <future>
#include <mutex>
#include <string>
#include <vector>

namespace adapters {

/**
 * @brief Background peak extractor generating min/max RMS waveform summaries
 * with binary disk caching in AppData for real-time seekbar visualization.
 */
class WaveformExtractor {
public:
  explicit WaveformExtractor(const std::string &cacheDirectory = "");
  ~WaveformExtractor() = default;

  void setCacheDirectory(const std::string &dir);
  [[nodiscard]] std::string getCacheDirectory() const;

  /**
   * @brief Extracts normalized amplitude peak buckets [0.0f, 1.0f] for an audio
   * file. Loads from disk cache if present; otherwise calculates in background.
   */
  std::vector<float> extractPeaks(const std::string &audioFilePath,
                                  size_t bucketCount = 128);

  /**
   * @brief Asynchronously extracts waveform peaks and invokes callback when
   * ready.
   */
  void extractPeaksAsync(const std::string &audioFilePath, size_t bucketCount,
                         std::function<void(std::vector<float>)> callback);

private:
  std::string getCacheFilePath(const std::string &audioFilePath,
                               size_t bucketCount) const;
  bool loadFromCache(const std::string &cachePath, size_t bucketCount,
                     std::vector<float> &outPeaks) const;
  bool saveToCache(const std::string &cachePath,
                   const std::vector<float> &peaks) const;

  mutable std::mutex m_mutex;
  std::string m_cacheDir;
};

} // namespace adapters
