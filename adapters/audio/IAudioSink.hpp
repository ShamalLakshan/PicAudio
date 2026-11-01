#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace adapters {

/**
 * @brief Information about a physical or logical audio output device.
 */
struct AudioDeviceInfo {
  std::string id;   ///< Unique identifier (e.g. "hw:0,0", "plughw:1,0",
                    ///< "pipewire-default")
  std::string name; ///< Human-readable label (e.g. "HDA Intel PCH (hw:0,0)")
  std::string description; ///< Device or card details
  bool isDefault{false};
};

/**
 * @brief Output stream configuration for hardware sinks.
 */
struct AudioSinkConfig {
  uint32_t sampleRate{44100};
  uint8_t channels{2};
  uint8_t bitDepth{16};
  bool isFloat{false};
  uint32_t bufferLatencyMs{50};
  std::string deviceId;
};

/**
 * @brief Hardware audio output sink interface.
 */
class IAudioSink {
public:
  virtual ~IAudioSink() = default;

  /**
   * @brief Opens the audio sink with given configuration.
   * @param config Target stream configuration and device.
   * @return True if initialized successfully.
   */
  virtual bool open(const AudioSinkConfig &config) = 0;

  /**
   * @brief Closes the audio sink and releases hardware resources.
   */
  virtual void close() = 0;

  /**
   * @brief Queries whether the sink is currently open and ready for output.
   */
  [[nodiscard]] virtual bool isOpen() const = 0;

  /**
   * @brief Writes interleaved PCM audio frames to the sink.
   * @param buffer Pointer to interleaved PCM data.
   * @param frames Number of audio frames (samples per channel) to write.
   * @return Number of frames actually written.
   */
  virtual size_t write(const void *buffer, size_t frames) = 0;

  /**
   * @brief Pauses audio output.
   */
  virtual void pause() = 0;

  /**
   * @brief Resumes audio output.
   */
  virtual void resume() = 0;

  /**
   * @brief Flushes pending hardware buffers.
   */
  virtual void flush() = 0;

  /**
   * @brief Current hardware buffer latency in milliseconds.
   */
  [[nodiscard]] virtual uint32_t getLatencyMs() const = 0;

  /**
   * @brief Number of buffer underruns / xruns recorded.
   */
  [[nodiscard]] virtual uint64_t getXrunCount() const = 0;

  /**
   * @brief True if audio is streamed bit-perfect without OS resampler
   * interference.
   */
  [[nodiscard]] virtual bool isBitPerfect() const { return true; }

  /**
   * @brief Unique display name of this audio backend driver.
   */
  [[nodiscard]] virtual std::string getName() const = 0;

  /**
   * @brief Enumerates available physical or logical audio output devices.
   */
  virtual std::vector<AudioDeviceInfo> enumerateDevices() = 0;
};

} // namespace adapters
