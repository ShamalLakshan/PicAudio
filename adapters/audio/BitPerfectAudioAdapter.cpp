#include "BitPerfectAudioAdapter.hpp"
#include "audio/AudioThreadPriority.hpp"
#include "audio/alsa/AlsaAudioSink.hpp"
#include "audio/pipewire/PipeWireAudioSink.hpp"
#include <QUrl>
#include <algorithm>
#include <cmath>
#include <complex>
#include <iostream>
#include <vector>

namespace adapters {

BitPerfectAudioEngine::BitPerfectAudioEngine(QObject *parent)
    : QObject(parent), m_player(std::make_unique<QMediaPlayer>()),
      m_audioOutput(std::make_unique<QAudioOutput>()) {

  m_player->setAudioOutput(m_audioOutput.get());
  m_audioOutput->setVolume(0.8f);

#if __has_include(<QAudioBufferOutput>)
  m_bufferOutput = std::make_unique<QAudioBufferOutput>(this);
  m_player->setAudioBufferOutput(m_bufferOutput.get());
  connect(m_bufferOutput.get(), &QAudioBufferOutput::audioBufferReceived, this,
          &BitPerfectAudioEngine::handleAudioBuffer);
#endif

  connect(m_player.get(), &QMediaPlayer::mediaStatusChanged, this,
          [this](QMediaPlayer::MediaStatus status) {
            if (status == QMediaPlayer::EndOfMedia) {
              emit playbackFinished();
              if (m_endOfTrackCallback) {
                m_endOfTrackCallback();
              }
            }
          });

  connect(m_player.get(), &QMediaPlayer::errorOccurred, this,
          [this](QMediaPlayer::Error error, const QString &errorString) {
            std::cerr << "BitPerfectAudioEngine Error (" << error
                      << "): " << errorString.toStdString() << std::endl;
          });
}

BitPerfectAudioEngine::~BitPerfectAudioEngine() { stop(); }

bool BitPerfectAudioEngine::initialize(uint32_t sampleRate, uint8_t channels) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_streamInfo.sampleRate = sampleRate;
  m_streamInfo.channels = channels;
  m_streamInfo.bitDepth = 24;
  m_streamInfo.isBitPerfect = true;
  m_streamInfo.codec = "FLAC/PCM";
  m_matrixProcessor.configure(channels, m_outputChannels);
  return true;
}

bool BitPerfectAudioEngine::load(const std::string &filePath) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_currentFilePath = filePath;
  if (m_player) {
    m_player->setSource(QUrl::fromLocalFile(QString::fromStdString(filePath)));
  }
  return true;
}

bool BitPerfectAudioEngine::play() {
  if (m_player) {
    m_player->play();
    return true;
  }
  return false;
}

bool BitPerfectAudioEngine::pause() {
  if (m_player) {
    m_player->pause();
    return true;
  }
  return false;
}

bool BitPerfectAudioEngine::stop() {
  if (m_player) {
    m_player->stop();
    return true;
  }
  return false;
}

bool BitPerfectAudioEngine::seek(uint64_t positionMs) {
  if (m_player) {
    m_player->setPosition(static_cast<qint64>(positionMs));
    return true;
  }
  return false;
}

void BitPerfectAudioEngine::setVolume(float volume) {
  if (m_audioOutput) {
    m_audioOutput->setVolume(std::clamp(volume, 0.0f, 1.0f));
  }
}

float BitPerfectAudioEngine::getVolume() const {
  if (m_audioOutput) {
    return m_audioOutput->volume();
  }
  return 0.8f;
}

uint64_t BitPerfectAudioEngine::getPositionMs() const {
  if (m_player) {
    return static_cast<uint64_t>(std::max<qint64>(0, m_player->position()));
  }
  return 0;
}

uint64_t BitPerfectAudioEngine::getDurationMs() const {
  if (m_player) {
    return static_cast<uint64_t>(std::max<qint64>(0, m_player->duration()));
  }
  return 0;
}

bool BitPerfectAudioEngine::isPlaying() const {
  return m_player && (m_player->playbackState() == QMediaPlayer::PlayingState);
}

core::AudioStreamInfo BitPerfectAudioEngine::getStreamInfo() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_streamInfo;
}

void BitPerfectAudioEngine::setEndOfTrackCallback(
    std::function<void()> callback) {
  m_endOfTrackCallback = std::move(callback);
}

void BitPerfectAudioEngine::setAudioBufferCallback(
    core::IAudioEnginePort::AudioBufferCallback callback) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_audioBufferCallback = std::move(callback);
}

void BitPerfectAudioEngine::setPlaybackRate(float rate) {
  if (m_player) {
    m_player->setPlaybackRate(static_cast<qreal>(rate));
  }
}

float BitPerfectAudioEngine::getPlaybackRate() const {
  if (m_player) {
    return static_cast<float>(m_player->playbackRate());
  }
  return 1.0f;
}

#if __has_include(<QAudioBufferOutput>)
void BitPerfectAudioEngine::handleAudioBuffer(const QAudioBuffer &buffer) {
  if (!buffer.isValid() || buffer.frameCount() <= 0) {
    return;
  }

  const int frameCount = static_cast<int>(buffer.frameCount());
  const auto format = buffer.format();
  const int inChannels = std::clamp(format.channelCount(), 1, 8);
  uint32_t sampleRate = static_cast<uint32_t>(format.sampleRate());
  if (sampleRate == 0) {
    sampleRate = 44100;
  }

  std::vector<float> inInterleaved(frameCount * inChannels, 0.0f);

  if (format.sampleFormat() == QAudioFormat::Float) {
    const float *data = buffer.constData<float>();
    if (data) {
      for (int i = 0; i < frameCount * inChannels; ++i) {
        inInterleaved[i] = data[i];
      }
    }
  } else if (format.sampleFormat() == QAudioFormat::Int16) {
    const int16_t *data = buffer.constData<int16_t>();
    if (data) {
      for (int i = 0; i < frameCount * inChannels; ++i) {
        inInterleaved[i] = static_cast<float>(data[i]) / 32768.0f;
      }
    }
  } else if (format.sampleFormat() == QAudioFormat::Int32) {
    const int32_t *data = buffer.constData<int32_t>();
    if (data) {
      for (int i = 0; i < frameCount * inChannels; ++i) {
        inInterleaved[i] = static_cast<float>(data[i]) / 2147483648.0f;
      }
    }
  } else if (format.sampleFormat() == QAudioFormat::UInt8) {
    const uint8_t *data = buffer.constData<uint8_t>();
    if (data) {
      for (int i = 0; i < frameCount * inChannels; ++i) {
        inInterleaved[i] = (static_cast<float>(data[i]) - 128.0f) / 128.0f;
      }
    }
  }

  // Multi-channel matrix processing (Mono to 7.1)
  const uint8_t outChannels = m_outputChannels;
  std::vector<float> outInterleaved(frameCount * outChannels, 0.0f);
  m_matrixProcessor.process(inInterleaved.data(), outInterleaved.data(),
                            frameCount);

  // Compute composite downmix for FFT bands and waveform visualizer
  std::vector<float> monoSamples(frameCount, 0.0f);
  for (int i = 0; i < frameCount; ++i) {
    float sum = 0.0f;
    for (int c = 0; c < outChannels; ++c) {
      sum += outInterleaved[i * outChannels + c];
    }
    monoSamples[i] = sum / static_cast<float>(outChannels);
  }

  // Calculate audio level (RMS and peak)
  float sumSq = 0.0f;
  float peak = 0.0f;
  for (float s : monoSamples) {
    float absS = std::abs(s);
    if (absS > peak)
      peak = absS;
    sumSq += s * s;
  }
  float rms = std::sqrt(sumSq / static_cast<float>(frameCount));
  float level = std::clamp(rms * 2.2f + peak * 0.3f, 0.0f, 1.0f);

  {
    std::lock_guard<std::mutex> lock(m_visMutex);
    m_audioLevel = std::max(level, m_audioLevel * 0.82f);

    m_pcmRingBuffer.insert(m_pcmRingBuffer.end(), monoSamples.begin(),
                           monoSamples.end());
    if (m_pcmRingBuffer.size() > 4096) {
      m_pcmRingBuffer.erase(m_pcmRingBuffer.begin(),
                            m_pcmRingBuffer.begin() +
                                (m_pcmRingBuffer.size() - 2048));
    }
  }

  updateFftBands(monoSamples, sampleRate);

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_audioBufferCallback) {
      m_audioBufferCallback(outInterleaved.data(), frameCount, sampleRate,
                            outChannels);
    }
  }
}
#endif

static void fftRadix2(std::vector<std::complex<float>> &a) {
  const size_t n = a.size();
  for (size_t i = 1, j = 0; i < n; ++i) {
    size_t bit = n >> 1;
    for (; j & bit; bit >>= 1) {
      j ^= bit;
    }
    j ^= bit;
    if (i < j) {
      std::swap(a[i], a[j]);
    }
  }
  constexpr float kPi = 3.14159265358979323846f;
  for (size_t len = 2; len <= n; len <<= 1) {
    float ang = -2.0f * kPi / static_cast<float>(len);
    std::complex<float> wlen(std::cos(ang), std::sin(ang));
    for (size_t i = 0; i < n; i += len) {
      std::complex<float> w(1.0f, 0.0f);
      for (size_t j = 0; j < len / 2; ++j) {
        std::complex<float> u = a[i + j];
        std::complex<float> v = a[i + j + len / 2] * w;
        a[i + j] = u + v;
        a[i + j + len / 2] = u - v;
        w *= wlen;
      }
    }
  }
}

void BitPerfectAudioEngine::updateFftBands(const std::vector<float> &samples,
                                           uint32_t sampleRate) {
  (void)samples;
  constexpr size_t N = 512;
  std::vector<std::complex<float>> fftInput(N, {0.0f, 0.0f});

  {
    std::lock_guard<std::mutex> lock(m_visMutex);
    if (m_pcmRingBuffer.size() < N) {
      return;
    }
    size_t start = m_pcmRingBuffer.size() - N;
    constexpr float kTwoPi = 6.28318530717958647692f;
    for (size_t i = 0; i < N; ++i) {
      float w = 0.5f * (1.0f - std::cos(kTwoPi * static_cast<float>(i) /
                                        static_cast<float>(N - 1)));
      fftInput[i] = std::complex<float>(m_pcmRingBuffer[start + i] * w, 0.0f);
    }

    if (m_currentWaveform.size() != 64) {
      m_currentWaveform.resize(64, 0.0f);
    }
    size_t step = N / 64;
    for (size_t i = 0; i < 64; ++i) {
      float target = std::clamp(m_pcmRingBuffer[start + i * step], -1.0f, 1.0f);
      m_currentWaveform[i] = m_currentWaveform[i] * 0.3f + target * 0.7f;
    }
  }

  fftRadix2(fftInput);

  constexpr size_t numBands = 36;
  std::vector<float> newBands(numBands, 0.0f);

  float nyquist = static_cast<float>(sampleRate) * 0.5f;
  float minFreq = 25.0f;
  float maxFreq = std::min(20000.0f, nyquist);
  float logRatio = std::log(maxFreq / minFreq);

  for (size_t b = 0; b < numBands; ++b) {
    float f0 = minFreq * std::exp(logRatio * static_cast<float>(b) /
                                  static_cast<float>(numBands));
    float f1 = minFreq * std::exp(logRatio * static_cast<float>(b + 1) /
                                  static_cast<float>(numBands));

    size_t k0 = std::clamp<size_t>(
        static_cast<size_t>((f0 / nyquist) * (N / 2)), 0, N / 2 - 1);
    size_t k1 = std::clamp<size_t>(
        static_cast<size_t>((f1 / nyquist) * (N / 2)), k0 + 1, N / 2);

    float peakMag = 0.0f;
    for (size_t k = k0; k < k1; ++k) {
      float mag = std::abs(fftInput[k]) / static_cast<float>(N / 2);
      if (mag > peakMag)
        peakMag = mag;
    }

    float trebleBoost =
        1.0f + 1.8f * (static_cast<float>(b) / static_cast<float>(numBands));
    float val = peakMag * trebleBoost * 4.0f;

    newBands[b] = std::clamp(val, 0.0f, 1.0f);
  }

  {
    std::lock_guard<std::mutex> lock(m_visMutex);
    if (m_currentBands.size() != numBands) {
      m_currentBands = newBands;
    } else {
      for (size_t b = 0; b < numBands; ++b) {
        if (newBands[b] >= m_currentBands[b]) {
          m_currentBands[b] = m_currentBands[b] * 0.2f + newBands[b] * 0.8f;
        } else {
          m_currentBands[b] = m_currentBands[b] * 0.78f;
        }
      }
    }
  }
}

std::vector<float>
BitPerfectAudioEngine::getFrequencyBands(size_t bandCount) const {
  std::lock_guard<std::mutex> lock(m_visMutex);
  if (!isPlaying()) {
    for (auto &b : m_currentBands) {
      b *= 0.7f;
    }
    m_audioLevel *= 0.7f;
  }
  if (m_currentBands.empty()) {
    return std::vector<float>(bandCount, 0.0f);
  }
  if (m_currentBands.size() == bandCount) {
    return m_currentBands;
  }
  std::vector<float> result(bandCount, 0.0f);
  for (size_t i = 0; i < bandCount; ++i) {
    size_t srcIdx = (i * m_currentBands.size()) / bandCount;
    result[i] = m_currentBands[srcIdx];
  }
  return result;
}

std::vector<float>
BitPerfectAudioEngine::getWaveformSamples(size_t sampleCount) const {
  std::lock_guard<std::mutex> lock(m_visMutex);
  if (!isPlaying()) {
    for (auto &w : m_currentWaveform) {
      w *= 0.7f;
    }
  }
  if (m_currentWaveform.empty()) {
    return std::vector<float>(sampleCount, 0.0f);
  }
  if (m_currentWaveform.size() == sampleCount) {
    return m_currentWaveform;
  }
  std::vector<float> result(sampleCount, 0.0f);
  for (size_t i = 0; i < sampleCount; ++i) {
    size_t srcIdx = (i * m_currentWaveform.size()) / sampleCount;
    result[i] = m_currentWaveform[srcIdx];
  }
  return result;
}

float BitPerfectAudioEngine::getAudioLevel() const {
  std::lock_guard<std::mutex> lock(m_visMutex);
  if (!isPlaying()) {
    m_audioLevel *= 0.7f;
  }
  return m_audioLevel;
}

void BitPerfectAudioEngine::setAudioBackend(const std::string &backend) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_audioBackend = backend;
}

std::string BitPerfectAudioEngine::getAudioBackend() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_audioBackend;
}

void BitPerfectAudioEngine::setAudioDevice(const std::string &deviceId) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_audioDevice = deviceId;
}

std::string BitPerfectAudioEngine::getAudioDevice() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_audioDevice;
}

void BitPerfectAudioEngine::setBufferLatencyMs(uint32_t ms) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_bufferLatencyMs = std::clamp(ms, 10u, 500u);
}

uint32_t BitPerfectAudioEngine::getBufferLatencyMs() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_bufferLatencyMs;
}

void BitPerfectAudioEngine::setBitPerfectExclusive(bool exclusive) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_bitPerfectExclusive = exclusive;
}

bool BitPerfectAudioEngine::isBitPerfectExclusive() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_bitPerfectExclusive;
}

uint64_t BitPerfectAudioEngine::getXrunCount() const { return 0; }

std::vector<core::IAudioEnginePort::DeviceInfo>
BitPerfectAudioEngine::getAvailableDevices(const std::string &backend) const {
  std::string activeBackend = backend.empty() ? m_audioBackend : backend;
  std::vector<core::IAudioEnginePort::DeviceInfo> result;

  if (activeBackend.find("ALSA") != std::string::npos) {
    AlsaAudioSink alsa;
    auto alsaDevs = alsa.enumerateDevices();
    for (const auto &d : alsaDevs) {
      result.push_back({d.id, d.name, d.description, d.isDefault});
    }
  } else if (activeBackend.find("PipeWire") != std::string::npos) {
    PipeWireAudioSink pw;
    auto pwDevs = pw.enumerateDevices();
    for (const auto &d : pwDevs) {
      result.push_back({d.id, d.name, d.description, d.isDefault});
    }
  }

  return result;
}

void BitPerfectAudioEngine::setChannelMatrixCoefficient(size_t outCh,
                                                        size_t inCh,
                                                        float gain) {
  m_matrixProcessor.setCoefficient(outCh, inCh, gain);
}

float BitPerfectAudioEngine::getChannelMatrixCoefficient(size_t outCh,
                                                         size_t inCh) const {
  return m_matrixProcessor.getCoefficient(outCh, inCh);
}

void BitPerfectAudioEngine::setChannelMatrixPreset(int preset) {
  m_matrixProcessor.loadPreset(static_cast<MatrixPreset>(preset));
}

int BitPerfectAudioEngine::getChannelMatrixPreset() const {
  return static_cast<int>(m_matrixProcessor.getActivePreset());
}

void BitPerfectAudioEngine::normalizeChannelMatrix() {
  m_matrixProcessor.normalize();
}

std::vector<float> BitPerfectAudioEngine::getChannelLevels() const {
  return m_matrixProcessor.getOutputLevels();
}

void BitPerfectAudioEngine::setOutputChannelCount(uint8_t channels) {
  m_outputChannels = std::clamp<uint8_t>(channels, 1, 8);
  m_matrixProcessor.setOutputChannels(m_outputChannels);
}

uint8_t BitPerfectAudioEngine::getOutputChannelCount() const {
  return m_outputChannels;
}

std::string BitPerfectAudioEngine::getSerializedChannelMatrix() const {
  return m_matrixProcessor.serialize();
}

bool BitPerfectAudioEngine::setSerializedChannelMatrix(
    const std::string &json) {
  return m_matrixProcessor.deserialize(json);
}

} // namespace adapters
