#include "BitPerfectAudioAdapter.hpp"
#include "audio/AudioThreadPriority.hpp"
#include "audio/alsa/AlsaAudioSink.hpp"
#include "audio/coreaudio/CoreAudioSink.hpp"
#include "audio/pipewire/PipeWireAudioSink.hpp"
#include "audio/wasapi/WasapiAudioSink.hpp"
#include <QMetaObject>
#include <QUrl>
#include <algorithm>
#include <cmath>
#include <complex>
#include <iostream>
#include <vector>

namespace adapters {

BitPerfectAudioEngine::BitPerfectAudioEngine(QObject *parent)
    : QObject(parent), m_player(std::make_unique<QMediaPlayer>()),
      m_pcmAudioDevice(std::make_unique<PcmAudioDevice>(m_outputRingBuffer)) {

#if __has_include(<QAudioBufferOutput>)
  m_bufferOutput = std::make_unique<QAudioBufferOutput>();
  m_player->setAudioBufferOutput(m_bufferOutput.get());
  connect(m_bufferOutput.get(), &QAudioBufferOutput::audioBufferReceived, this,
          &BitPerfectAudioEngine::handleAudioBuffer, Qt::DirectConnection);
#endif

  configureAudioSink(44100, 2);

  // Launch dedicated real-time audio DSP worker thread
  m_dspThreadRunning.store(true, std::memory_order_release);
  m_dspThread = std::thread(&BitPerfectAudioEngine::dspWorkerLoop, this);

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

BitPerfectAudioEngine::~BitPerfectAudioEngine() {
  m_dspThreadRunning.store(false, std::memory_order_release);
  m_dspCv.notify_all();
  if (m_dspThread.joinable()) {
    m_dspThread.join();
  }

  stop();
  if (m_audioSink) {
    m_audioSink->stop();
    m_audioSink.reset();
  }
  if (m_pcmAudioDevice) {
    m_pcmAudioDevice->close();
    m_pcmAudioDevice.reset();
  }
#if __has_include(<QAudioBufferOutput>)
  if (m_player) {
    m_player->setAudioBufferOutput(nullptr);
  }
  m_bufferOutput.reset();
#endif
  if (m_player) {
    m_player->stop();
    m_player.reset();
  }
}

void BitPerfectAudioEngine::configureAudioSink(uint32_t sampleRate,
                                               uint8_t channels) {
  QAudioFormat fmt;
  fmt.setSampleRate(sampleRate > 0 ? sampleRate : 44100);
  fmt.setChannelCount(channels > 0 ? channels : 2);
  fmt.setSampleFormat(QAudioFormat::Float);

  if (m_audioSink && m_audioSink->format() == fmt) {
    return;
  }

  if (m_audioSink) {
    m_audioSink->stop();
    m_audioSink.reset();
  }

  auto defaultDevice = QMediaDevices::defaultAudioOutput();
  if (defaultDevice.isNull()) {
    return;
  }

  m_currentSinkSampleRate.store(static_cast<uint32_t>(fmt.sampleRate()),
                                std::memory_order_release);
  m_currentSinkChannels.store(static_cast<uint8_t>(fmt.channelCount()),
                              std::memory_order_release);

  m_audioSink = std::make_unique<QAudioSink>(defaultDevice, fmt);
  m_audioSink->setVolume(m_volume);
  if (m_player && m_player->playbackState() == QMediaPlayer::PlayingState) {
    if (m_pcmAudioDevice) {
      m_audioSink->start(m_pcmAudioDevice.get());
    }
  }
}

bool BitPerfectAudioEngine::initialize(uint32_t sampleRate, uint8_t channels) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_streamInfo.sampleRate = sampleRate;
  m_streamInfo.channels = channels;
  m_streamInfo.bitDepth = 24;
  m_streamInfo.isBitPerfect = true;
  m_streamInfo.codec = "FLAC/PCM";
  const uint8_t outCh = m_outputChannels.load(std::memory_order_relaxed);
  m_matrixProcessor.configure(channels, outCh);
  m_parametricEq.configure(sampleRate, outCh);
  configureAudioSink(sampleRate, outCh);
  return true;
}

bool BitPerfectAudioEngine::load(const std::string &filePath) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_currentFilePath = filePath;
  m_decodedRingBuffer.reset();
  m_outputRingBuffer.reset();
  m_dspCv.notify_all();

  std::string actualPath = filePath;
  m_virtualStartOffsetMs = 0;
  m_virtualEndOffsetMs = 0;

  size_t cuePos = filePath.find("#cue:");
  if (cuePos != std::string::npos) {
    actualPath = filePath.substr(0, cuePos);
  }

  // Inspect audio file and configure in-process decoder
  m_activeDecoder = AudioDecoderFactory::createDecoder(actualPath);
  if (m_activeDecoder && m_activeDecoder->open(actualPath)) {
    auto meta = m_activeDecoder->getMetadata();
    m_activeDecoderName =
        meta.codec.empty() ? "Direct In-Process Decoder" : meta.codec;
    m_streamInfo.sampleRate = meta.sampleRate;
    m_streamInfo.channels = meta.channels;
    m_streamInfo.bitDepth = meta.bitDepth;
    m_streamInfo.codec = meta.codec;
  } else {
    m_activeDecoderName = "Qt Multimedia / FFmpeg Pipeline";
  }

  if (m_player) {
    m_player->setSource(
        QUrl::fromLocalFile(QString::fromStdString(actualPath)));
  }
  return true;
}

bool BitPerfectAudioEngine::play() {
  if (m_player) {
    if (m_audioSink) {
      if (m_audioSink->state() == QAudio::SuspendedState) {
        m_audioSink->resume();
      } else if (m_audioSink->state() == QAudio::StoppedState) {
        if (m_pcmAudioDevice) {
          m_audioSink->start(m_pcmAudioDevice.get());
        }
      }
    }
    m_player->play();
    return true;
  }
  return false;
}

bool BitPerfectAudioEngine::pause() {
  if (m_player) {
    m_player->pause();
    if (m_audioSink && m_audioSink->state() == QAudio::ActiveState) {
      m_audioSink->suspend();
    }
    return true;
  }
  return false;
}

bool BitPerfectAudioEngine::stop() {
  if (m_player) {
    m_player->stop();
    if (m_audioSink) {
      m_audioSink->stop();
    }
    if (m_pcmAudioDevice) {
      m_pcmAudioDevice->resetBuffer();
    }
    m_decodedRingBuffer.reset();
    m_outputRingBuffer.reset();
    m_dspCv.notify_all();
    return true;
  }
  return false;
}

bool BitPerfectAudioEngine::seek(uint64_t positionMs) {
  if (m_player) {
    m_decodedRingBuffer.reset();
    m_outputRingBuffer.reset();
    m_dspCv.notify_all();
    m_player->setPosition(static_cast<qint64>(positionMs));
    return true;
  }
  return false;
}

void BitPerfectAudioEngine::setVolume(float volume) {
  m_volume = std::clamp(volume, 0.0f, 1.0f);
  if (m_audioSink) {
    m_audioSink->setVolume(m_volume);
  }
}

float BitPerfectAudioEngine::getVolume() const { return m_volume; }

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
  const int totalSamples = buffer.sampleCount() > 0 ? buffer.sampleCount()
                                                    : (frameCount * inChannels);

  uint32_t sampleRate = static_cast<uint32_t>(format.sampleRate());
  if (sampleRate == 0) {
    sampleRate = 44100;
  }

  m_currentInputChannels.store(static_cast<uint8_t>(inChannels),
                               std::memory_order_release);

  const uint8_t targetOutCh = m_outputChannels.load(std::memory_order_relaxed);
  if (sampleRate != m_currentSinkSampleRate.load(std::memory_order_relaxed)) {
    m_parametricEq.configure(sampleRate, targetOutCh);
    QMetaObject::invokeMethod(
        this,
        [this, sampleRate, targetOutCh]() {
          configureAudioSink(sampleRate, targetOutCh);
        },
        Qt::QueuedConnection);
  }

  std::vector<float> inInterleaved(totalSamples, 0.0f);

  if (format.sampleFormat() == QAudioFormat::Float) {
    const float *data = buffer.constData<float>();
    if (data) {
      size_t copyBytes = std::min(static_cast<size_t>(buffer.byteCount()),
                                  inInterleaved.size() * sizeof(float));
      std::memcpy(inInterleaved.data(), data, copyBytes);
    }
  } else if (format.sampleFormat() == QAudioFormat::Int16) {
    const int16_t *data = buffer.constData<int16_t>();
    if (data) {
      const int count = std::min(
          totalSamples, static_cast<int>(buffer.byteCount() / sizeof(int16_t)));
      for (int i = 0; i < count; ++i) {
        inInterleaved[i] = static_cast<float>(data[i]) / 32768.0f;
      }
    }
  } else if (format.sampleFormat() == QAudioFormat::Int32) {
    const int32_t *data = buffer.constData<int32_t>();
    if (data) {
      const int count = std::min(
          totalSamples, static_cast<int>(buffer.byteCount() / sizeof(int32_t)));
      for (int i = 0; i < count; ++i) {
        inInterleaved[i] = static_cast<float>(data[i]) / 2147483648.0f;
      }
    }
  } else if (format.sampleFormat() == QAudioFormat::UInt8) {
    const uint8_t *data = buffer.constData<uint8_t>();
    if (data) {
      const int count = std::min(
          totalSamples, static_cast<int>(buffer.byteCount() / sizeof(uint8_t)));
      for (int i = 0; i < count; ++i) {
        inInterleaved[i] = (static_cast<float>(data[i]) - 128.0f) / 128.0f;
      }
    }
  }

  // Enqueue decoded frames into ring buffer for the dedicated DSP worker thread
  m_decodedRingBuffer.write(inInterleaved.data(), inInterleaved.size());
  m_dspCv.notify_one();
}
#endif

void BitPerfectAudioEngine::dspWorkerLoop() {
  configureAudioThreadPriority(AudioPriorityMode::RealtimeRoundRobin, 70);

  std::vector<float> inChunk;
  std::vector<float> matrixOut;

  while (m_dspThreadRunning.load(std::memory_order_relaxed)) {
    const uint8_t inChannels =
        m_currentInputChannels.load(std::memory_order_relaxed);
    const uint8_t outChannels =
        m_outputChannels.load(std::memory_order_relaxed);
    const uint32_t sampleRate =
        m_currentSinkSampleRate.load(std::memory_order_relaxed);

    if (inChannels == 0 || outChannels == 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
      continue;
    }

    size_t availRead = m_decodedRingBuffer.availableRead();
    size_t availWrite = m_outputRingBuffer.availableWrite();

    size_t framesToRead = availRead / inChannels;
    size_t framesCanWrite = availWrite / outChannels;
    size_t frames = std::min({framesToRead, framesCanWrite, size_t(1024)});

    if (frames == 0) {
      std::unique_lock<std::mutex> lock(m_dspCvMutex);
      m_dspCv.wait_for(lock, std::chrono::milliseconds(5), [this]() {
        return !m_dspThreadRunning.load(std::memory_order_relaxed) ||
               (m_decodedRingBuffer.availableRead() >= 256 &&
                m_outputRingBuffer.availableWrite() >= 256);
      });
      continue;
    }

    inChunk.resize(frames * inChannels);
    size_t readSamples =
        m_decodedRingBuffer.read(inChunk.data(), frames * inChannels);
    frames = readSamples / inChannels;
    if (frames == 0)
      continue;

    const bool eqActive = !m_parametricEq.isBypassed();
    const bool dspActive = m_dspManager.isDspMasterEnabled();
    const bool loudnessActive =
        (m_loudnessProcessor.getReplayGainMode() != ReplayGainMode::Disabled ||
         m_loudnessProcessor.isTruePeakLimiterEnabled() ||
         m_loudnessProcessor.getDitherMode() != DitherMode::None);
    const bool crossfadeActive = m_crossfadeEngine.isCrossfadeActive();
    const bool needsMatrix =
        (inChannels != outChannels) ||
        (m_matrixProcessor.getActivePreset() != MatrixPreset::Passthrough);

    if (!needsMatrix && !eqActive && !dspActive && !loudnessActive &&
        !crossfadeActive) {
      // Fast path: bit-perfect passthrough when DSP and processing are disabled
      m_outputRingBuffer.write(inChunk.data(), frames * inChannels);
      updateVisualizerTap(inChunk.data(), frames, inChannels, sampleRate);
      continue;
    }

    // 1. Multi-channel matrix processing (Mono to 7.1)
    matrixOut.resize(frames * outChannels);
    if (needsMatrix) {
      m_matrixProcessor.process(inChunk.data(), matrixOut.data(), frames);
    } else {
      std::memcpy(matrixOut.data(), inChunk.data(),
                  frames * outChannels * sizeof(float));
    }

    // 2. Equalizer & External DSP according to user routing sequence
    if (m_dspManager.getRoutingOrder() == DspManager::RoutingOrder::EqThenDsp) {
      if (eqActive) {
        m_parametricEq.process(matrixOut.data(), frames);
      }
      if (dspActive) {
        m_dspManager.processDspChain(matrixOut.data(), frames, sampleRate,
                                     outChannels);
      }
    } else {
      if (dspActive) {
        m_dspManager.processDspChain(matrixOut.data(), frames, sampleRate,
                                     outChannels);
      }
      if (eqActive) {
        m_parametricEq.process(matrixOut.data(), frames);
      }
    }

    // 3. Loudness Normalization (ReplayGain / R128), Limiter, and 64-bit TPDF
    // Dithering
    m_loudnessProcessor.process(matrixOut.data(), frames, outChannels);

    // 4. Smooth Crossfading / Buffer Blending
    m_crossfadeEngine.process(matrixOut.data(), frames, outChannels);

    // 5. Write into output ring buffer for the sound card / sink
    m_outputRingBuffer.write(matrixOut.data(), matrixOut.size());

    // 6. Update audio level, waveform, and FFT visualizer
    updateVisualizerTap(matrixOut.data(), frames, outChannels, sampleRate);
  }
}

void BitPerfectAudioEngine::updateVisualizerTap(const float *samples,
                                                size_t frames, uint8_t channels,
                                                uint32_t sampleRate) {
  if (!samples || frames == 0 || channels == 0)
    return;

  std::vector<float> monoSamples(frames, 0.0f);
  for (size_t i = 0; i < frames; ++i) {
    float sum = 0.0f;
    for (uint8_t c = 0; c < channels; ++c) {
      sum += samples[i * channels + c];
    }
    monoSamples[i] = sum / static_cast<float>(channels);
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
  float rms = std::sqrt(sumSq / static_cast<float>(frames));
  float level = std::clamp(rms * 2.2f + peak * 0.3f, 0.0f, 1.0f);

  {
    std::lock_guard<std::mutex> lock(m_visMutex);
    m_audioLevel = std::max(level, m_audioLevel * 0.82f);

    if (m_pcmVisBuffer.size() != 4096) {
      m_pcmVisBuffer.assign(4096, 0.0f);
      m_pcmVisWriteHead = 0;
    }
    for (float s : monoSamples) {
      m_pcmVisBuffer[m_pcmVisWriteHead] = s;
      m_pcmVisWriteHead = (m_pcmVisWriteHead + 1) % 4096;
    }
  }

  core::IAudioEnginePort::AudioBufferCallback cb;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    cb = m_audioBufferCallback;
  }
  if (cb) {
    cb(samples, frames, sampleRate, channels);
  }
}

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
    if (m_pcmVisBuffer.size() < N) {
      return;
    }
    const size_t total = m_pcmVisBuffer.size();
    size_t start = (m_pcmVisWriteHead + total - N) % total;
    constexpr float kTwoPi = 6.28318530717958647692f;
    for (size_t i = 0; i < N; ++i) {
      float w = 0.5f * (1.0f - std::cos(kTwoPi * static_cast<float>(i) /
                                        static_cast<float>(N - 1)));
      float sample = m_pcmVisBuffer[(start + i) % total];
      fftInput[i] = std::complex<float>(sample * w, 0.0f);
    }

    if (m_currentWaveform.size() != 64) {
      m_currentWaveform.resize(64, 0.0f);
    }
    size_t step = N / 64;
    for (size_t i = 0; i < 64; ++i) {
      float sample = m_pcmVisBuffer[(start + i * step) % total];
      float target = std::clamp(sample, -1.0f, 1.0f);
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
  if (!isPlaying()) {
    std::lock_guard<std::mutex> lock(m_visMutex);
    for (auto &b : m_currentBands) {
      b *= 0.7f;
    }
    m_audioLevel *= 0.7f;
    if (m_currentBands.empty()) {
      return std::vector<float>(bandCount, 0.0f);
    }
  } else {
    const_cast<BitPerfectAudioEngine *>(this)->updateFftBands(
        {}, m_currentSinkSampleRate);
  }

  std::lock_guard<std::mutex> lock(m_visMutex);
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
  } else if (activeBackend.find("WASAPI") != std::string::npos) {
    WasapiAudioSink wasapi;
    auto wDevs = wasapi.enumerateDevices();
    for (const auto &d : wDevs) {
      result.push_back({d.id, d.name, d.description, d.isDefault});
    }
  } else if (activeBackend.find("CoreAudio") != std::string::npos) {
    CoreAudioSink ca;
    auto caDevs = ca.enumerateDevices();
    for (const auto &d : caDevs) {
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
  uint8_t ch = std::clamp<uint8_t>(channels, 1, 8);
  m_outputChannels.store(ch, std::memory_order_release);
  m_matrixProcessor.setOutputChannels(ch);
}

uint8_t BitPerfectAudioEngine::getOutputChannelCount() const {
  return m_outputChannels.load(std::memory_order_acquire);
}

std::string BitPerfectAudioEngine::getSerializedChannelMatrix() const {
  return m_matrixProcessor.serialize();
}

bool BitPerfectAudioEngine::setSerializedChannelMatrix(
    const std::string &json) {
  return m_matrixProcessor.deserialize(json);
}

void BitPerfectAudioEngine::setEqBandGain(size_t bandIndex, float gainDb) {
  m_parametricEq.setBandGain(bandIndex, gainDb);
}

float BitPerfectAudioEngine::getEqBandGain(size_t bandIndex) const {
  return m_parametricEq.getBandGain(bandIndex);
}

void BitPerfectAudioEngine::setEqBandGains(const std::vector<float> &gains) {
  m_parametricEq.setBandGains(gains);
}

std::vector<float> BitPerfectAudioEngine::getEqBandGains() const {
  return m_parametricEq.getBandGains();
}

void BitPerfectAudioEngine::setEqPreampGain(float gainDb) {
  m_parametricEq.setPreampGain(gainDb);
}

float BitPerfectAudioEngine::getEqPreampGain() const {
  return m_parametricEq.getPreampGain();
}

void BitPerfectAudioEngine::setEqEnabled(bool enabled) {
  m_parametricEq.setBypassed(!enabled);
}

bool BitPerfectAudioEngine::isEqEnabled() const {
  return !m_parametricEq.isBypassed();
}

void BitPerfectAudioEngine::setEqPreset(const std::string &preset) {
  m_parametricEq.setPreset(preset);
}

std::string BitPerfectAudioEngine::getEqPreset() const {
  return m_parametricEq.getPreset();
}

void BitPerfectAudioEngine::resetEq() { m_parametricEq.reset(); }

void BitPerfectAudioEngine::setEqBandCount(size_t bandCount) {
  m_parametricEq.setBandCount(bandCount);
}

size_t BitPerfectAudioEngine::getEqBandCount() const {
  return m_parametricEq.getBandCount();
}

std::vector<std::string>
BitPerfectAudioEngine::getEqBandFrequencyLabels() const {
  return m_parametricEq.getBandFrequencyLabels();
}

void BitPerfectAudioEngine::setDspRoutingOrder(int order) {
  m_dspManager.setRoutingOrder(static_cast<DspManager::RoutingOrder>(order));
}

int BitPerfectAudioEngine::getDspRoutingOrder() const {
  return static_cast<int>(m_dspManager.getRoutingOrder());
}

void BitPerfectAudioEngine::setDspMasterEnabled(bool enabled) {
  m_dspManager.setDspMasterEnabled(enabled);
}

bool BitPerfectAudioEngine::isDspMasterEnabled() const {
  return m_dspManager.isDspMasterEnabled();
}

void BitPerfectAudioEngine::setReplayGainMode(int mode) {
  m_loudnessProcessor.setReplayGainMode(static_cast<ReplayGainMode>(mode));
}

int BitPerfectAudioEngine::getReplayGainMode() const {
  return static_cast<int>(m_loudnessProcessor.getReplayGainMode());
}

void BitPerfectAudioEngine::setReplayGainPreamp(float taggedDb,
                                                float untaggedDb) {
  m_loudnessProcessor.setPreampTaggedDb(taggedDb);
  m_loudnessProcessor.setPreampUntaggedDb(untaggedDb);
}

void BitPerfectAudioEngine::setTruePeakLimiterEnabled(bool enabled) {
  m_loudnessProcessor.setTruePeakLimiterEnabled(enabled);
}

bool BitPerfectAudioEngine::isTruePeakLimiterEnabled() const {
  return m_loudnessProcessor.isTruePeakLimiterEnabled();
}

void BitPerfectAudioEngine::setCurrentTrackReplayGain(
    bool hasGain, float trackGain, float trackPeak, float albumGain,
    float albumPeak, bool isAlbum) {
  m_loudnessProcessor.setCurrentTrackLoudness(hasGain, trackGain, trackPeak,
                                              albumGain, albumPeak, isAlbum);
}

void BitPerfectAudioEngine::setCrossfadeEnabled(bool enabled) {
  m_crossfadeEngine.setEnabled(enabled);
}

bool BitPerfectAudioEngine::isCrossfadeEnabled() const {
  return m_crossfadeEngine.isEnabled();
}

void BitPerfectAudioEngine::setCrossfadeDurationSec(float sec) {
  m_crossfadeEngine.setDurationSec(sec);
}

float BitPerfectAudioEngine::getCrossfadeDurationSec() const {
  return m_crossfadeEngine.getDurationSec();
}

void BitPerfectAudioEngine::setCrossfadeCurve(int curve) {
  m_crossfadeEngine.setCurve(static_cast<CrossfadeCurve>(curve));
}

int BitPerfectAudioEngine::getCrossfadeCurve() const {
  return static_cast<int>(m_crossfadeEngine.getCurve());
}

void BitPerfectAudioEngine::setGaplessEnabled(bool enabled) {
  m_crossfadeEngine.setGaplessEnabled(enabled);
}

bool BitPerfectAudioEngine::isGaplessEnabled() const {
  return m_crossfadeEngine.isGaplessEnabled();
}

void BitPerfectAudioEngine::setDitherMode(int mode) {
  m_loudnessProcessor.setDitherMode(static_cast<DitherMode>(mode));
}

int BitPerfectAudioEngine::getDitherMode() const {
  return static_cast<int>(m_loudnessProcessor.getDitherMode());
}

float BitPerfectAudioEngine::getBufferFillPercentage() const {
  return m_outputRingBuffer.fillPercentage();
}

std::string BitPerfectAudioEngine::getActiveDecoderName() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_activeDecoderName;
}

} // namespace adapters
