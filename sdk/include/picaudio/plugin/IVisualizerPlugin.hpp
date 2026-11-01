#pragma once

#include "Defines.hpp"

namespace picaudio::plugin {

/**
 * @brief Extension interface for real-time visualizers (spectrum analyzers,
 * meters).
 */
class IVisualizerPlugin {
public:
  virtual ~IVisualizerPlugin() = default;

  /**
   * @brief Process a PCM audio buffer from the player output pipeline.
   * @param buffer Interleaved floating point audio samples.
   * @param frames Number of audio frames in buffer.
   * @param sampleRate Sampling rate in Hz (e.g. 44100, 48000, 96000).
   * @param channels Channel count (e.g. 2 for stereo).
   */
  virtual void processAudioBuffer(const float *buffer, size_t frames,
                                  uint32_t sampleRate, uint8_t channels) = 0;

  /**
   * @brief Retrieve normalized frequency bands [0.0f, 1.0f].
   * @param outBands Pre-allocated caller buffer to write band values into.
   * @param maxBands Capacity of the outBands buffer.
   * @return Number of bands actually populated.
   */
  virtual size_t getFrequencyBands(float *outBands, size_t maxBands) const = 0;

  /**
   * @brief Query whether visualizer computation is active.
   */
  [[nodiscard]] virtual bool isEnabled() const noexcept = 0;

  /**
   * @brief Enable or disable visualizer computation.
   */
  virtual void setEnabled(bool enabled) = 0;
};

} // namespace picaudio::plugin
