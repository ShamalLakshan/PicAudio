#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <random>
#include <string>
#include <vector>

namespace adapters {

enum class ReplayGainMode { Disabled, TrackGain, AlbumGain, SmartGain };

enum class DitherMode { None, FlatTpdf, HighPassTpdf };

/**
 * @brief Audiophile loudness normalization, true-peak anti-clipping limiter,
 * and 64-bit Triangular Probability Density Function (TPDF) dithering
 * processor.
 */
class LoudnessAndDitherProcessor {
public:
  LoudnessAndDitherProcessor();
  ~LoudnessAndDitherProcessor() = default;

  void setReplayGainMode(ReplayGainMode mode);
  [[nodiscard]] ReplayGainMode getReplayGainMode() const;

  void setPreampTaggedDb(float db);
  [[nodiscard]] float getPreampTaggedDb() const;

  void setPreampUntaggedDb(float db);
  [[nodiscard]] float getPreampUntaggedDb() const;

  void setTruePeakLimiterEnabled(bool enabled);
  [[nodiscard]] bool isTruePeakLimiterEnabled() const;

  void setCurrentTrackLoudness(bool hasReplayGain, float trackGainDb,
                               float trackPeak, float albumGainDb,
                               float albumPeak, bool isAlbumPlayback);

  void setDitherMode(DitherMode mode);
  [[nodiscard]] DitherMode getDitherMode() const;

  void setTargetBitDepth(uint8_t bitDepth);
  [[nodiscard]] uint8_t getTargetBitDepth() const;

  /**
   * @brief Processes interleaved 32-bit float PCM audio samples in-place.
   * Lock-free and real-time safe for the audio thread.
   */
  void process(float *buffer, size_t frames, uint8_t channels);

  [[nodiscard]] float getCurrentAppliedGainDb() const;
  [[nodiscard]] float getCurrentAppliedGainLinear() const;

private:
  void recalculateGainLocked();

  mutable std::mutex m_mutex;
  ReplayGainMode m_mode{ReplayGainMode::SmartGain};
  float m_preampTaggedDb{0.0f};
  float m_preampUntaggedDb{-6.0f};
  std::atomic<bool> m_limiterEnabled{true};
  std::atomic<DitherMode> m_ditherMode{DitherMode::FlatTpdf};
  std::atomic<uint8_t> m_targetBitDepth{16};

  bool m_hasReplayGain{false};
  float m_trackGainDb{0.0f};
  float m_trackPeak{1.0f};
  float m_albumGainDb{0.0f};
  float m_albumPeak{1.0f};
  bool m_isAlbumPlayback{true};

  std::atomic<float> m_targetLinearGain{1.0f};
  std::atomic<float> m_currentLinearGain{1.0f};
  std::atomic<float> m_appliedGainDb{0.0f};

  // State accessed exclusively on audio rendering thread
  std::mt19937_64 m_prng{1337};
  std::uniform_real_distribution<double> m_uniformDist{-0.5, 0.5};
  double m_lastDitherNoise{0.0};
};

} // namespace adapters
