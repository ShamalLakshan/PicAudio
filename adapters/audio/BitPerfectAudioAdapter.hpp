#pragma once

#include "audio/dsp/DspManager.hpp"
#include "core/ports/IAudioEnginePort.hpp"

#include <memory>
#include <string>
#include <vector>

namespace PicAudioEngine {
class EngineHandler;
class PlayerController;
class SettingsManager;
class AudioLoader;
class DspRegistry;
class VisualisationSession;
class Track;
} // namespace PicAudioEngine

namespace PicAudio {
namespace Engine = PicAudioEngine;
} // namespace PicAudio

namespace adapters {

/**
 * @brief High-fidelity audio engine adapter implementing IAudioEnginePort
 * using PicAudio's multi-threaded audio pipeline, lock-free mixer, and real-time
 * visualization.
 */
class BitPerfectAudioEngine : public core::IAudioEnginePort {
public:
  BitPerfectAudioEngine();
  ~BitPerfectAudioEngine() override;

  BitPerfectAudioEngine(const BitPerfectAudioEngine &) = delete;
  BitPerfectAudioEngine &operator=(const BitPerfectAudioEngine &) = delete;

  // Lifecycle & Playback Controls
  bool initialize(uint32_t sampleRate = 44100, uint8_t channels = 2) override;
  bool load(const std::string &filePath) override;
  bool play() override;
  bool pause() override;
  bool stop() override;
  bool seek(uint64_t positionMs) override;
  void setVolume(float volume) override;
  float getVolume() const override;
  uint64_t getPositionMs() const override;
  uint64_t getDurationMs() const override;
  bool isPlaying() const override;
  core::AudioStreamInfo getStreamInfo() const override;

  // Callbacks
  void setEndOfTrackCallback(std::function<void()> callback) override;
  void setAudioBufferCallback(AudioBufferCallback callback) override;

  // Playback Rate
  void setPlaybackRate(float rate) override;
  float getPlaybackRate() const override;

  // Visualization / Spectrum
  std::vector<float> getFrequencyBands(size_t bandCount = 36) const override;
  std::vector<float> getWaveformSamples(size_t sampleCount = 64) const override;
  float getAudioLevel() const override;

  // Hardware Outputs & Devices
  void setAudioBackend(const std::string &backend) override;
  std::string getAudioBackend() const override;
  void setAudioDevice(const std::string &deviceId) override;
  std::string getAudioDevice() const override;
  std::vector<DeviceInfo>
  getAvailableDevices(const std::string &backend = "") const override;
  void setBufferLatencyMs(uint32_t ms) override;
  uint32_t getBufferLatencyMs() const override;
  void setBitPerfectExclusive(bool exclusive) override;
  bool isBitPerfectExclusive() const override;
  uint64_t getXrunCount() const override;

  // Channel Matrix & DSP
  void setChannelMatrixCoefficient(size_t outCh, size_t inCh,
                                   float gain) override;
  float getChannelMatrixCoefficient(size_t outCh, size_t inCh) const override;
  void setChannelMatrixPreset(int preset) override;
  int getChannelMatrixPreset() const override;
  void normalizeChannelMatrix() override;
  std::vector<float> getChannelLevels() const override;
  void setOutputChannelCount(uint8_t channels) override;
  uint8_t getOutputChannelCount() const override;
  std::string getSerializedChannelMatrix() const override;
  bool setSerializedChannelMatrix(const std::string &json) override;

  // Equalizer
  void setEqBandGain(size_t bandIndex, float gainDb) override;
  float getEqBandGain(size_t bandIndex) const override;
  void setEqBandGains(const std::vector<float> &gains) override;
  std::vector<float> getEqBandGains() const override;
  void setEqPreampGain(float gainDb) override;
  float getEqPreampGain() const override;
  void setEqEnabled(bool enabled) override;
  bool isEqEnabled() const override;
  void setEqPreset(const std::string &preset) override;
  std::string getEqPreset() const override;
  void resetEq() override;
  void setEqBandCount(size_t bandCount) override;
  size_t getEqBandCount() const override;
  std::vector<std::string> getEqBandFrequencyLabels() const override;

  // DSP Routing & Master
  void setDspRoutingOrder(int order) override;
  int getDspRoutingOrder() const override;
  void setDspMasterEnabled(bool enabled) override;
  bool isDspMasterEnabled() const override;

  // ReplayGain 2.0 & Loudness Normalization
  void setReplayGainMode(int mode) override;
  int getReplayGainMode() const override;
  void setReplayGainPreamp(float taggedDb, float untaggedDb) override;
  void setTruePeakLimiterEnabled(bool enabled) override;
  bool isTruePeakLimiterEnabled() const override;
  void setCurrentTrackReplayGain(bool hasGain, float trackGain, float trackPeak,
                                 float albumGain, float albumPeak,
                                 bool isAlbum) override;

  // Crossfade & Gapless
  void setCrossfadeEnabled(bool enabled) override;
  bool isCrossfadeEnabled() const override;
  void setCrossfadeDurationSec(float sec) override;
  float getCrossfadeDurationSec() const override;
  void setCrossfadeCurve(int curve) override;
  int getCrossfadeCurve() const override;
  void setGaplessEnabled(bool enabled) override;
  bool isGaplessEnabled() const override;

  // Dither Mode
  void setDitherMode(int mode) override;
  int getDitherMode() const override;

  // Diagnostics
  float getBufferFillPercentage() const override;
  std::string getActiveDecoderName() const override;

  // External CLAP DSP Plugin Host Manager
  DspManager &dspManager() { return m_dspManager; }

  // Direct reference to underlying engine controllers
  PicAudioEngine::EngineHandler *engineHandler() const;
  PicAudioEngine::PlayerController *playerController() const;

private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;
  DspManager m_dspManager;
};

using BitPerfectAudioAdapter = BitPerfectAudioEngine;
using DirectHardwareAudioEngine = BitPerfectAudioEngine;
using StudioMasterAudioEngine = BitPerfectAudioEngine;

} // namespace adapters
