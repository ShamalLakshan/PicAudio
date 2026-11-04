#pragma once

#include "audio/LockFreeRingBuffer.hpp"
#include "audio/WaveformExtractor.hpp"
#include "audio/decoders/AudioDecoderFactory.hpp"
#include "audio/decoders/IAudioDecoder.hpp"
#include "audio/dsp/ChannelMatrixProcessor.hpp"
#include "audio/dsp/CrossfadeEngine.hpp"
#include "audio/dsp/DspManager.hpp"
#include "audio/dsp/LoudnessAndDitherProcessor.hpp"
#include "audio/dsp/ParametricEqualizer.hpp"
#include "core/ports/IAudioEnginePort.hpp"
#include <QAudioFormat>
#include <QAudioOutput>
#include <QAudioSink>
#include <QIODevice>
#include <QMediaDevices>
#include <QMediaPlayer>
#include <QObject>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#if __has_include(<QAudioBufferOutput>)
#include <QAudioBuffer>
#include <QAudioBufferOutput>
#endif

namespace adapters {

/**
 * @brief Lock-free pulling device feeding processed float PCM samples to
 * QAudioSink.
 */
class PcmAudioDevice : public QIODevice {
  Q_OBJECT
public:
  explicit PcmAudioDevice(LockFreeRingBuffer<float> &ringBuffer,
                          QObject *parent = nullptr)
      : QIODevice(parent), m_ringBuffer(ringBuffer) {
    open(QIODevice::ReadOnly | QIODevice::Unbuffered);
  }

  qint64 readData(char *data, qint64 maxlen) override {
    size_t samplesNeeded = maxlen / sizeof(float);
    float *dst = reinterpret_cast<float *>(data);
    size_t samplesRead = m_ringBuffer.read(dst, samplesNeeded);
    if (samplesRead < samplesNeeded) {
      std::memset(dst + samplesRead, 0,
                  (samplesNeeded - samplesRead) * sizeof(float));
    }
    return static_cast<qint64>(samplesNeeded * sizeof(float));
  }

  qint64 writeData(const char *, qint64) override { return 0; }

  qint64 bytesAvailable() const override {
    return static_cast<qint64>(m_ringBuffer.availableRead() * sizeof(float) +
                               QIODevice::bytesAvailable());
  }

  bool isSequential() const override { return true; }

  void resetBuffer() { m_ringBuffer.reset(); }

private:
  LockFreeRingBuffer<float> &m_ringBuffer;
};

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

  void setDspRoutingOrder(int order) override;
  int getDspRoutingOrder() const override;
  void setDspMasterEnabled(bool enabled) override;
  bool isDspMasterEnabled() const override;

  void setReplayGainMode(int mode) override;
  int getReplayGainMode() const override;
  void setReplayGainPreamp(float taggedDb, float untaggedDb) override;
  void setTruePeakLimiterEnabled(bool enabled) override;
  bool isTruePeakLimiterEnabled() const override;
  void setCurrentTrackReplayGain(bool hasGain, float trackGain, float trackPeak,
                                 float albumGain, float albumPeak,
                                 bool isAlbum) override;

  void setCrossfadeEnabled(bool enabled) override;
  bool isCrossfadeEnabled() const override;
  void setCrossfadeDurationSec(float sec) override;
  float getCrossfadeDurationSec() const override;
  void setCrossfadeCurve(int curve) override;
  int getCrossfadeCurve() const override;
  void setGaplessEnabled(bool enabled) override;
  bool isGaplessEnabled() const override;

  void setDitherMode(int mode) override;
  int getDitherMode() const override;

  float getBufferFillPercentage() const override;
  std::string getActiveDecoderName() const override;

  ParametricEqualizer &parametricEqualizer() { return m_parametricEq; }
  const ParametricEqualizer &parametricEqualizer() const {
    return m_parametricEq;
  }
  DspManager &dspManager() { return m_dspManager; }
  const DspManager &dspManager() const { return m_dspManager; }
  LoudnessAndDitherProcessor &loudnessProcessor() {
    return m_loudnessProcessor;
  }
  const LoudnessAndDitherProcessor &loudnessProcessor() const {
    return m_loudnessProcessor;
  }
  CrossfadeEngine &crossfadeEngine() { return m_crossfadeEngine; }
  const CrossfadeEngine &crossfadeEngine() const { return m_crossfadeEngine; }
  WaveformExtractor &waveformExtractor() { return m_waveformExtractor; }

signals:
  void playbackFinished();

private:
  std::unique_ptr<QMediaPlayer> m_player;
  std::unique_ptr<QAudioOutput> m_audioOutput;
#if __has_include(<QAudioBufferOutput>)
  void handleAudioBuffer(const QAudioBuffer &buffer);
  std::unique_ptr<QAudioBufferOutput> m_bufferOutput;
#endif
  void updateFftBands(const std::vector<float> &samples, uint32_t sampleRate);
  void configureAudioSink(uint32_t sampleRate, uint8_t channels);
  void dspWorkerLoop();
  void updateVisualizerTap(const float *samples, size_t frames,
                           uint8_t channels, uint32_t sampleRate);

  core::AudioStreamInfo m_streamInfo;
  mutable std::mutex m_mutex;
  std::function<void()> m_endOfTrackCallback;
  core::IAudioEnginePort::AudioBufferCallback m_audioBufferCallback;
  std::string m_currentFilePath;

  std::string m_audioBackend{"Linux PipeWire Lock-Free Client"};
  std::string m_audioDevice{"default"};
  uint32_t m_bufferLatencyMs{50};
  bool m_bitPerfectExclusive{true};
  float m_volume{0.8f};

  ChannelMatrixProcessor m_matrixProcessor;
  std::atomic<uint8_t> m_outputChannels{2};

  ParametricEqualizer m_parametricEq;
  DspManager m_dspManager;
  LoudnessAndDitherProcessor m_loudnessProcessor;
  CrossfadeEngine m_crossfadeEngine;
  WaveformExtractor m_waveformExtractor;

  std::unique_ptr<IAudioDecoder> m_activeDecoder;
  std::string m_activeDecoderName{"Direct In-Process / FLAC"};
  uint64_t m_virtualStartOffsetMs{0};
  uint64_t m_virtualEndOffsetMs{0};

  LockFreeRingBuffer<float> m_decodedRingBuffer{262144};
  LockFreeRingBuffer<float> m_outputRingBuffer{131072};
  std::thread m_dspThread;
  std::atomic<bool> m_dspThreadRunning{false};
  std::condition_variable m_dspCv;
  std::mutex m_dspCvMutex;
  std::atomic<uint8_t> m_currentInputChannels{2};

  std::unique_ptr<PcmAudioDevice> m_pcmAudioDevice;
  std::unique_ptr<QAudioSink> m_audioSink;
  std::atomic<uint32_t> m_currentSinkSampleRate{44100};
  std::atomic<uint8_t> m_currentSinkChannels{2};

  mutable std::mutex m_visMutex;
  std::vector<float> m_pcmVisBuffer{4096, 0.0f};
  size_t m_pcmVisWriteHead{0};
  mutable std::vector<float> m_currentBands;
  mutable std::vector<float> m_currentWaveform;
  mutable float m_audioLevel{0.0f};
};

using BitPerfectAudioAdapter = BitPerfectAudioEngine;
using DirectHardwareAudioEngine = BitPerfectAudioEngine;
using StudioMasterAudioEngine = BitPerfectAudioEngine;

} // namespace adapters
