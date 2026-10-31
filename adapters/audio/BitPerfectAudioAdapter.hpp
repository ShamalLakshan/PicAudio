#pragma once

#include "core/ports/IAudioEnginePort.hpp"
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
 * @brief Bit-perfect audio engine adapter powered by Qt 6 Multimedia & FFmpeg
 * backend.
 */
class BitPerfectAudioAdapter : public QObject, public core::IAudioEnginePort {
  Q_OBJECT

public:
  explicit BitPerfectAudioAdapter(QObject *parent = nullptr);
  ~BitPerfectAudioAdapter() override;

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

  mutable std::mutex m_visMutex;
  std::vector<float> m_pcmRingBuffer;
  mutable std::vector<float> m_currentBands;
  mutable std::vector<float> m_currentWaveform;
  mutable float m_audioLevel{0.0f};
};

} // namespace adapters
