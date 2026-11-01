#pragma once

#include "core/ports/IAudioEnginePort.hpp"
#include "dsp/ChannelMatrixProcessor.hpp"
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QObject>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#if __has_include(<QAudioBufferOutput>)
#include <QAudioBuffer>
#include <QAudioBufferOutput>
#endif

namespace adapters {

/**
 * @brief Bit-perfect audio engine powered by low-latency hardware sinks,
 * multi-channel matrix processor, and Qt Multimedia backend.
 */
class BitPerfectAudioEngine : public QObject, public core::IAudioEnginePort {
  Q_OBJECT

public:
  explicit BitPerfectAudioEngine(QObject *parent = nullptr);
  ~BitPerfectAudioEngine() override;

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

  void setEndOfTrackCallback(std::function<void()> callback) override;
  void setAudioBufferCallback(
      core::IAudioEnginePort::AudioBufferCallback callback) override;
  void setPlaybackRate(float rate) override;
  float getPlaybackRate() const override;

  std::vector<float> getFrequencyBands(size_t bandCount = 36) const override;
  std::vector<float> getWaveformSamples(size_t sampleCount = 64) const override;
  float getAudioLevel() const override;

  void setAudioBackend(const std::string &backend) override;
  std::string getAudioBackend() const override;
  void setAudioDevice(const std::string &deviceId) override;
  std::string getAudioDevice() const override;
  void setBufferLatencyMs(uint32_t ms) override;
  uint32_t getBufferLatencyMs() const override;
  void setBitPerfectExclusive(bool exclusive) override;
  bool isBitPerfectExclusive() const override;
  uint64_t getXrunCount() const override;
  std::vector<core::IAudioEnginePort::DeviceInfo>
  getAvailableDevices(const std::string &backend = "") const override;

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

signals:
  void playbackFinished();

private:
#if __has_include(<QAudioBufferOutput>)
  void handleAudioBuffer(const QAudioBuffer &buffer);
  std::unique_ptr<QAudioBufferOutput> m_bufferOutput;
#endif
  void updateFftBands(const std::vector<float> &samples, uint32_t sampleRate);

  std::unique_ptr<QMediaPlayer> m_player;
  std::unique_ptr<QAudioOutput> m_audioOutput;
  core::AudioStreamInfo m_streamInfo;
  mutable std::mutex m_mutex;
  std::function<void()> m_endOfTrackCallback;
  core::IAudioEnginePort::AudioBufferCallback m_audioBufferCallback;
  std::string m_currentFilePath;

  std::string m_audioBackend{"Linux PipeWire Lock-Free Client"};
  std::string m_audioDevice{"default"};
  uint32_t m_bufferLatencyMs{50};
  bool m_bitPerfectExclusive{true};

  ChannelMatrixProcessor m_matrixProcessor;
  uint8_t m_outputChannels{2};

  mutable std::mutex m_visMutex;
  std::vector<float> m_pcmRingBuffer;
  mutable std::vector<float> m_currentBands;
  mutable std::vector<float> m_currentWaveform;
  mutable float m_audioLevel{0.0f};
};

using BitPerfectAudioAdapter = BitPerfectAudioEngine;

} // namespace adapters
