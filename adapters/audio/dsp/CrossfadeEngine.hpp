#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace adapters {

enum class CrossfadeCurve {
  EqualPower, ///< Constant acoustic energy: cos(t * PI/2) / sin(t * PI/2)
  Linear,     ///< Linear slope: (1 - t) / t
  Logarithmic ///< Perceptual log fade
};

/**
 * @brief Real-time audio crossfader and sample-accurate gapless buffer splicer.
 *
 * Blends consecutive PCM audio buffers using Equal-Power, Linear, or
 * Logarithmic curves, and automatically preserves seamless gapless transitions
 * for tracks on the same album.
 */
class CrossfadeEngine {
public:
  CrossfadeEngine();
  ~CrossfadeEngine() = default;

  void setEnabled(bool enabled);
  [[nodiscard]] bool isEnabled() const;

  void setDurationSec(float seconds);
  [[nodiscard]] float getDurationSec() const;

  void setCurve(CrossfadeCurve curve);
  [[nodiscard]] CrossfadeCurve getCurve() const;

  void setAutoDisableSameAlbum(bool disable);
  [[nodiscard]] bool isAutoDisableSameAlbum() const;

  void setGaplessEnabled(bool enabled);
  [[nodiscard]] bool isGaplessEnabled() const;

  /**
   * @brief Checks if a crossfade should be triggered between current and
   * upcoming tracks.
   * @param currentAlbum Name of current track's album.
   * @param nextAlbum Name of upcoming track's album.
   * @param remainingMs Milliseconds remaining in current track.
   * @return True if crossfade blending should begin.
   */
  bool shouldTriggerCrossfade(const std::string &currentAlbum,
                              const std::string &nextAlbum,
                              uint64_t remainingMs) const;

  /**
   * @brief Initiates crossfade transition with incoming audio stream buffer.
   */
  void beginCrossfade(size_t totalTransitionFrames);

  /**
   * @brief Feeds incoming track PCM frames into the crossfader blending buffer.
   */
  void writeIncomingSamples(const float *samples, size_t count);

  /**
   * @brief Blends outgoing and incoming audio frames in-place.
   */
  void process(float *outgoingBuffer, size_t frames, uint8_t channels);

  [[nodiscard]] bool isCrossfadeActive() const;
  void reset();

private:
  mutable std::mutex m_mutex;
  std::atomic<bool> m_enabled{false};
  std::atomic<float> m_durationSec{2.0f};
  std::atomic<CrossfadeCurve> m_curve{CrossfadeCurve::EqualPower};
  std::atomic<bool> m_autoDisableSameAlbum{true};
  std::atomic<bool> m_gaplessEnabled{true};

  std::atomic<bool> m_isCrossfading{false};
  size_t m_transitionFrames{0};
  size_t m_elapsedFrames{0};

  std::vector<float> m_incomingBuffer;
  size_t m_incomingReadPos{0};
};

} // namespace adapters
