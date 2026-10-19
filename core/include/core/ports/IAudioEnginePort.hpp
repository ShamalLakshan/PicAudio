#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace core {

/**
 * @brief Current playback stream parameters.
 */
struct AudioStreamInfo {
  uint32_t sampleRate{44100};
  uint8_t bitDepth{16};
  uint8_t channels{2};
  uint32_t bitrate{0};
  std::string codec{"PCM"};
  bool isBitPerfect{true};
};

/**
 * @brief Audio engine port for low-latency, bit-perfect audio playback.
 */
class IAudioEnginePort {
public:
  virtual ~IAudioEnginePort() = default;

  virtual bool initialize(uint32_t sampleRate = 44100,
                          uint8_t channels = 2) = 0;
  virtual bool load(const std::string &filePath) = 0;
  virtual bool play() = 0;
  virtual bool pause() = 0;
  virtual bool stop() = 0;
  virtual bool seek(uint64_t positionMs) = 0;
  virtual void setVolume(float volume) = 0; ///< 0.0f (mute) to 1.0f (max)
  virtual float getVolume() const = 0;
  virtual uint64_t getPositionMs() const = 0;
  virtual uint64_t getDurationMs() const = 0;
  virtual bool isPlaying() const = 0;
  virtual AudioStreamInfo getStreamInfo() const = 0;
  virtual void setEndOfTrackCallback(std::function<void()> callback) {
    (void)callback;
  }

  /**
   * @brief Sets the audio playback rate multiplier.
   * @param rate Speed multiplier (e.g. 1.0f for normal speed).
   */
  virtual void setPlaybackRate(float rate) { (void)rate; }

  /**
   * @brief Gets the active audio playback rate multiplier.
   * @return Current speed multiplier.
   */
  virtual float getPlaybackRate() const { return 1.0f; }

  /**
   * @brief Gets frequency spectrum bands normalized to [0.0f, 1.0f].
   * @param bandCount Number of frequency bands requested.
   */
  virtual std::vector<float> getFrequencyBands(size_t bandCount = 36) const {
    (void)bandCount;
    return {};
  }

  /**
   * @brief Gets waveform sample points normalized to [-1.0f, 1.0f].
   * @param sampleCount Number of waveform sample points requested.
   */
  virtual std::vector<float> getWaveformSamples(size_t sampleCount = 64) const {
    (void)sampleCount;
    return {};
  }

  /**
   * @brief Gets current RMS audio level / amplitude in [0.0f, 1.0f].
   */
  virtual float getAudioLevel() const { return 0.0f; }
};

} // namespace core
