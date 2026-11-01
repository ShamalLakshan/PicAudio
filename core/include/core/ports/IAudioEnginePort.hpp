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

  using AudioBufferCallback =
      std::function<void(const float *buffer, size_t frames,
                         uint32_t sampleRate, uint8_t channels)>;

  /**
   * @brief Registers an audio buffer callback hook for real-time visualizers &
   * DSP.
   */
  virtual void setAudioBufferCallback(AudioBufferCallback callback) {
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
   * @brief Hardware audio device information descriptor.
   */
  struct DeviceInfo {
    std::string id;
    std::string name;
    std::string description;
    bool isDefault{false};
  };

  /**
   * @brief Sets the active hardware audio backend.
   */
  virtual void setAudioBackend(const std::string &backend) { (void)backend; }
  virtual std::string getAudioBackend() const { return ""; }

  /**
   * @brief Sets target hardware audio output device.
   */
  virtual void setAudioDevice(const std::string &deviceId) { (void)deviceId; }
  virtual std::string getAudioDevice() const { return ""; }

  /**
   * @brief Sets target hardware buffer latency in milliseconds.
   */
  virtual void setBufferLatencyMs(uint32_t ms) { (void)ms; }
  virtual uint32_t getBufferLatencyMs() const { return 50; }

  /**
   * @brief Enables or disables bit-perfect exclusive hardware access.
   */
  virtual void setBitPerfectExclusive(bool exclusive) { (void)exclusive; }
  virtual bool isBitPerfectExclusive() const { return true; }

  /**
   * @brief Number of buffer underruns / xruns recorded during playback.
   */
  virtual uint64_t getXrunCount() const { return 0; }

  /**
   * @brief Queries available devices for a given backend.
   */
  virtual std::vector<DeviceInfo>
  getAvailableDevices(const std::string &backend = "") const {
    (void)backend;
    return {};
  }

  /**
   * @brief Gets current RMS audio level / amplitude in [0.0f, 1.0f].
   */
  virtual float getAudioLevel() const { return 0.0f; }

  /**
   * @brief Sets matrix gain coefficient from inCh to outCh.
   */
  virtual void setChannelMatrixCoefficient(size_t outCh, size_t inCh,
                                           float gain) {
    (void)outCh;
    (void)inCh;
    (void)gain;
  }

  virtual float getChannelMatrixCoefficient(size_t outCh, size_t inCh) const {
    (void)outCh;
    (void)inCh;
    return (outCh == inCh) ? 1.0f : 0.0f;
  }

  /**
   * @brief Sets channel matrix preset (0: Passthrough, 1: DownmixToStereo, 2:
   * DownmixToMono, 3: UpmixTo51, 4: UpmixTo71, 5: Custom).
   */
  virtual void setChannelMatrixPreset(int preset) { (void)preset; }
  virtual int getChannelMatrixPreset() const { return 0; }

  /**
   * @brief Normalizes matrix coefficients to prevent clipping.
   */
  virtual void normalizeChannelMatrix() {}

  /**
   * @brief Gets real-time RMS levels for all 8 surround channels [FL, FR, FC,
   * LFE, RL, RR, SL, SR].
   */
  virtual std::vector<float> getChannelLevels() const { return {}; }

  /**
   * @brief Sets target hardware / output channel count (1 to 8).
   */
  virtual void setOutputChannelCount(uint8_t channels) { (void)channels; }
  virtual uint8_t getOutputChannelCount() const { return 2; }

  /**
   * @brief Serializes matrix configuration to JSON.
   */
  virtual std::string getSerializedChannelMatrix() const { return ""; }
  virtual bool setSerializedChannelMatrix(const std::string &json) {
    (void)json;
    return false;
  }
};

} // namespace core
