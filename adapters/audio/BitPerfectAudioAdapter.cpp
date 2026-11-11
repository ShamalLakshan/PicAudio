#include "BitPerfectAudioAdapter.hpp"

#include <core/coresettings.h>
#include <core/engine/audioloader.h>
#include <core/engine/dsp/dspregistry.h>
#include <core/engine/visualisationservice.h>
#include <core/internalcoresettings.h>
#include <core/player/playercontroller.h>
#include <core/track.h>
#include <utils/settings/settingsmanager.h>

#include "enginehandler.h"
#include "input/ffmpeg/ffmpeginput.h"
#include "input/taglibparser.h"
#include "outputs/qtaudio/qtaudiooutput.h"

#if defined(PICAUDIO_ENABLE_ALSA)
#include "outputs/alsa/alsaoutput.h"
#endif

#if defined(PICAUDIO_ENABLE_PIPEWIRE)
#include "outputs/pipewire/pipewireoutput.h"
#endif

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <iostream>
#include <mutex>

namespace adapters {

struct BitPerfectAudioEngine::Impl {
  std::shared_ptr<PicAudioEngine::SettingsManager> settings;
  std::unique_ptr<PicAudioEngine::CoreSettings> coreSettings;
  std::shared_ptr<PicAudioEngine::AudioLoader> audioLoader;
  std::unique_ptr<PicAudioEngine::DspRegistry> dspRegistry;
  std::unique_ptr<PicAudioEngine::PlayerController> playerController;
  std::unique_ptr<PicAudioEngine::EngineHandler> engineHandler;
  PicAudioEngine::VisualisationSessionPtr visSession;

  std::string currentFilePath;
  PicAudioEngine::Track currentTrack;
  std::function<void()> endOfTrackCallback;
  AudioBufferCallback audioBufferCallback;

  std::string activeBackend{"Linux PipeWire Lock-Free Client"};
  std::string activeDevice{"System Default Output"};
  uint32_t bufferLatencyMs{50};
  bool bitPerfectExclusive{true};
  std::atomic<float> currentAudioLevel{0.0f};

  // Equalizer state
  bool eqEnabled{true};
  float eqPreampGain{0.0f};
  size_t eqBandCount{10};
  std::string eqPreset{"Flat"};
  std::vector<float> eqBands{10, 0.0f};
  int dspRoutingOrder{0};
  bool dspMasterEnabled{true};

  // Channel Matrix state (8x8)
  int channelPreset{0};
  uint8_t outputChannels{2};
  float channelMatrix[8][8]{};

  // ReplayGain state
  int replayGainMode{3};
  float replayGainTaggedPreamp{0.0f};
  float replayGainUntaggedPreamp{0.0f};
  bool truePeakLimiter{true};

  // Crossfade & Gapless
  bool crossfadeEnabled{false};
  float crossfadeDurationSec{2.0f};
  int crossfadeCurve{0};
  bool gaplessEnabled{true};
  int ditherMode{1};

  Impl() {
    for (int r = 0; r < 8; ++r) {
      for (int c = 0; c < 8; ++c) {
        channelMatrix[r][c] = (r == c) ? 1.0f : 0.0f;
      }
    }
  }
};

BitPerfectAudioEngine::BitPerfectAudioEngine()
    : m_impl(std::make_unique<Impl>()) {}

BitPerfectAudioEngine::~BitPerfectAudioEngine() {
  if (m_impl && m_impl->playerController) {
    m_impl->playerController->stop();
  }
}

bool BitPerfectAudioEngine::initialize(uint32_t sampleRate, uint8_t channels) {
  (void)sampleRate;
  (void)channels;

  QString configDir =
      QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
  QDir{}.mkpath(configDir);
  QString settingsFile = configDir + QStringLiteral("/engine_settings.ini");

  m_impl->settings = std::make_shared<PicAudioEngine::SettingsManager>(settingsFile);
  m_impl->coreSettings =
      std::make_unique<PicAudioEngine::CoreSettings>(m_impl->settings.get());

  m_impl->audioLoader = std::make_shared<PicAudioEngine::AudioLoader>();
  m_impl->dspRegistry = std::make_unique<PicAudioEngine::DspRegistry>();

  // Register Native In-Process FFmpeg Decoder supporting FLAC, WAV, MP3, AAC,
  // OGG, Opus, ALAC, DSD
  m_impl->audioLoader->addDecoder(
      QStringLiteral("FFmpeg"),
      []() { return std::make_unique<PicAudioEngine::FFmpegDecoder>(); }, 1000);

  // Register Readers for metadata and audio probing
  m_impl->audioLoader->addReader(
      QStringLiteral("TagLib"),
      []() { return std::make_unique<PicAudioEngine::TagLibReader>(); }, 100);
  m_impl->audioLoader->addReader(
      QStringLiteral("FFmpeg"),
      []() { return std::make_unique<PicAudioEngine::FFmpegReader>(); }, 1000);

  // Initialize PlayerController and EngineHandler
  m_impl->playerController = std::make_unique<PicAudioEngine::PlayerController>(
      m_impl->settings.get(), nullptr);
  m_impl->engineHandler = std::make_unique<PicAudioEngine::EngineHandler>(
      m_impl->audioLoader, m_impl->playerController.get(),
      m_impl->settings.get(), m_impl->dspRegistry.get());

  // Register Hardware Outputs
#if defined(PICAUDIO_ENABLE_PIPEWIRE)
  m_impl->engineHandler->addOutput(
      QStringLiteral("Linux PipeWire Lock-Free Client"),
      []() { return std::make_unique<PicAudioEngine::Pipewire::PipeWireOutput>(); });
  m_impl->engineHandler->addOutput(QStringLiteral("PipeWire"), []() {
    return std::make_unique<PicAudioEngine::Pipewire::PipeWireOutput>();
  });
#endif

#if defined(PICAUDIO_ENABLE_ALSA)
  m_impl->engineHandler->addOutput(
      QStringLiteral("ALSA Direct Hardware (hw:)"),
      []() { return std::make_unique<PicAudioEngine::Alsa::AlsaOutput>(); });
  m_impl->engineHandler->addOutput(QStringLiteral("ALSA"), []() {
    return std::make_unique<PicAudioEngine::Alsa::AlsaOutput>();
  });
#endif

  m_impl->engineHandler->addOutput(
      QStringLiteral("Qt Multimedia Audio Sink"),
      []() { return std::make_unique<PicAudioEngine::QtAudioOutput>(); });
  m_impl->engineHandler->addOutput(
      QStringLiteral("System Default Output"),
      []() { return std::make_unique<PicAudioEngine::QtAudioOutput>(); });

  // Default audio output backend selection
#if defined(PICAUDIO_ENABLE_PIPEWIRE)
  const QString defaultBackend =
      QStringLiteral("Linux PipeWire Lock-Free Client");
#elif defined(PICAUDIO_ENABLE_ALSA)
  const QString defaultBackend = QStringLiteral("ALSA Direct Hardware (hw:)");
#else
  const QString defaultBackend = QStringLiteral("Qt Multimedia Audio Sink");
#endif
  m_impl->activeBackend = defaultBackend.toStdString();
  m_impl->activeDevice = "default";
  m_impl->settings->set<PicAudioEngine::Settings::Core::AudioOutput>(
      defaultBackend + QStringLiteral("|default"));

  // Initialise output wiring & signal relays
  m_impl->engineHandler->setup();

  // Create pull-based visualisation session
  if (auto *vs = m_impl->engineHandler->visualisationService()) {
    m_impl->visSession = vs->createSession();
  }

  // Connect EngineHandler level & PCM hooks
  QObject::connect(
      m_impl->engineHandler.get(), &PicAudioEngine::EngineHandler::levelReady,
      [this](const PicAudioEngine::LevelFrame &frame) {
        float lvl = (frame.channelCount > 0) ? frame.rms[0] : 0.0f;
        m_impl->currentAudioLevel.store(lvl, std::memory_order_relaxed);
      });

  QObject::connect(
      m_impl->engineHandler.get(), &PicAudioEngine::EngineHandler::pcmReady,
      [this](const PicAudioEngine::PcmFrame &frame) {
        if (m_impl->audioBufferCallback && frame.frameCount > 0) {
          m_impl->audioBufferCallback(
              frame.samples.data(), static_cast<size_t>(frame.frameCount),
              static_cast<uint32_t>(frame.format.sampleRate()),
              static_cast<uint8_t>(frame.format.channelCount()));
        }
      });

  QObject::connect(m_impl->engineHandler.get(),
                   &PicAudioEngine::EngineHandler::trackBoundaryReached,
                   [this](const PicAudioEngine::Engine::AboutToFinishContext &) {
                     if (m_impl->endOfTrackCallback) {
                       m_impl->endOfTrackCallback();
                     }
                   });

  return true;
}

bool BitPerfectAudioEngine::load(const std::string &filePath) {
  if (!m_impl || !m_impl->playerController) {
    return false;
  }

  m_impl->currentFilePath = filePath;
  PicAudioEngine::Track track{QString::fromStdString(filePath)};

  // Read metadata & audio specs via AudioLoader reader
  if (auto reader = m_impl->audioLoader->loadReaderForTrack(track);
      reader.reader) {
    reader.reader->readTrack(reader.input.source, track);
  }

  m_impl->currentTrack = track;
  m_impl->playerController->changeCurrentTrack(track);
  return true;
}

bool BitPerfectAudioEngine::play() {
  if (!m_impl || !m_impl->playerController) {
    return false;
  }
  m_impl->playerController->play();
  return true;
}

bool BitPerfectAudioEngine::pause() {
  if (!m_impl || !m_impl->playerController) {
    return false;
  }
  m_impl->playerController->pause();
  return true;
}

bool BitPerfectAudioEngine::stop() {
  if (!m_impl || !m_impl->playerController) {
    return false;
  }
  m_impl->playerController->stop();
  return true;
}

bool BitPerfectAudioEngine::seek(uint64_t positionMs) {
  if (!m_impl || !m_impl->playerController) {
    return false;
  }
  m_impl->playerController->seek(positionMs);
  return true;
}

void BitPerfectAudioEngine::setVolume(float volume) {
  if (!m_impl || !m_impl->settings) {
    return;
  }
  double vol = static_cast<double>(std::clamp(volume, 0.0f, 1.0f));
  m_impl->settings->set<PicAudioEngine::Settings::Core::OutputVolume>(vol);
}

float BitPerfectAudioEngine::getVolume() const {
  if (!m_impl || !m_impl->settings) {
    return 1.0f;
  }
  double vol = m_impl->settings->value<PicAudioEngine::Settings::Core::OutputVolume>();
  return static_cast<float>(std::clamp(vol, 0.0, 1.0));
}

uint64_t BitPerfectAudioEngine::getPositionMs() const {
  if (!m_impl || !m_impl->playerController) {
    return 0;
  }
  return m_impl->playerController->currentPosition();
}

uint64_t BitPerfectAudioEngine::getDurationMs() const {
  if (!m_impl) {
    return 0;
  }
  return static_cast<uint64_t>(m_impl->currentTrack.duration());
}

bool BitPerfectAudioEngine::isPlaying() const {
  if (!m_impl || !m_impl->playerController) {
    return false;
  }
  return m_impl->playerController->playState() ==
         PicAudioEngine::Player::PlayState::Playing;
}

core::AudioStreamInfo BitPerfectAudioEngine::getStreamInfo() const {
  core::AudioStreamInfo info;
  if (!m_impl || !m_impl->playerController) {
    return info;
  }
  const auto outInfo = m_impl->playerController->playbackOutputInfo();
  info.sampleRate =
      outInfo.format.sampleRate() > 0 ? outInfo.format.sampleRate() : 44100;
  info.channels =
      outInfo.format.channelCount() > 0 ? outInfo.format.channelCount() : 2;
  info.bitDepth =
      outInfo.format.bitdepth() > 0 ? outInfo.format.bitdepth() : 16;
  info.bitrate = m_impl->playerController->bitrate();
  QString dec = m_impl->playerController->decoder();
  info.codec = dec.isEmpty() ? "FLAC / PCM" : dec.toStdString();
  info.isBitPerfect = true;
  return info;
}

void BitPerfectAudioEngine::setEndOfTrackCallback(
    std::function<void()> callback) {
  if (m_impl) {
    m_impl->endOfTrackCallback = std::move(callback);
  }
}

void BitPerfectAudioEngine::setAudioBufferCallback(
    AudioBufferCallback callback) {
  if (m_impl) {
    m_impl->audioBufferCallback = std::move(callback);
  }
}

void BitPerfectAudioEngine::setPlaybackRate(float rate) { (void)rate; }

float BitPerfectAudioEngine::getPlaybackRate() const { return 1.0f; }

std::vector<float>
BitPerfectAudioEngine::getFrequencyBands(size_t bandCount) const {
  if (!m_impl || !m_impl->visSession || bandCount == 0) {
    return std::vector<float>(bandCount, 0.0f);
  }

  PicAudioEngine::VisualisationSession::SpectrumWindow spec;
  uint64_t nowMs = m_impl->visSession->currentTimeMs();
  constexpr int fftSize = 1024;

  if (m_impl->visSession->getSpectrumWindowEndingAt(spec, nowMs, fftSize)) {
    if (!spec.magnitudes.empty()) {
      std::vector<float> bands(bandCount, 0.0f);
      size_t totalBins = static_cast<size_t>(spec.binCount());
      for (size_t i = 0; i < bandCount; ++i) {
        size_t startBin = i * totalBins / bandCount;
        size_t endBin = std::max(startBin + 1, (i + 1) * totalBins / bandCount);
        float maxVal = 0.0f;
        for (size_t b = startBin; b < endBin && b < totalBins; ++b) {
          maxVal = std::max(maxVal, spec.magnitudes[b]);
        }
        bands[i] = std::clamp(maxVal, 0.0f, 1.0f);
      }
      return bands;
    }
  }

  return std::vector<float>(bandCount, 0.0f);
}

std::vector<float>
BitPerfectAudioEngine::getWaveformSamples(size_t sampleCount) const {
  if (!m_impl || !m_impl->visSession || sampleCount == 0) {
    return std::vector<float>(sampleCount, 0.0f);
  }

  PicAudioEngine::VisualisationSession::PcmWindow pcm;
  uint64_t nowMs = m_impl->visSession->currentTimeMs();

  if (m_impl->visSession->getPcmWindowEndingAt(pcm, nowMs, 50)) {
    if (!pcm.samples.empty()) {
      std::vector<float> samples(sampleCount, 0.0f);
      size_t step = std::max<size_t>(1, pcm.samples.size() / sampleCount);
      for (size_t i = 0; i < sampleCount && (i * step) < pcm.samples.size();
           ++i) {
        samples[i] = std::clamp(pcm.samples[i * step], -1.0f, 1.0f);
      }
      return samples;
    }
  }

  return std::vector<float>(sampleCount, 0.0f);
}

float BitPerfectAudioEngine::getAudioLevel() const {
  if (!m_impl) {
    return 0.0f;
  }
  return m_impl->currentAudioLevel.load(std::memory_order_relaxed);
}

void BitPerfectAudioEngine::setAudioBackend(const std::string &backend) {
  if (!m_impl) {
    return;
  }
  m_impl->activeBackend = backend;
  if (m_impl->engineHandler) {
    QString be = backend.empty()
                     ? QStringLiteral("Linux PipeWire Lock-Free Client")
                     : QString::fromStdString(backend);
    QString dev = m_impl->activeDevice.empty()
                      ? QStringLiteral("default")
                      : QString::fromStdString(m_impl->activeDevice);
    m_impl->engineHandler->applyOutputProfile(
        PicAudioEngine::Engine::OutputProfileRequest{be, dev});
    if (m_impl->settings) {
      using namespace Qt::StringLiterals;
      m_impl->settings->set<PicAudioEngine::Settings::Core::AudioOutput>(be + u"|"_s +
                                                                 dev);
    }
  }
}

std::string BitPerfectAudioEngine::getAudioBackend() const {
  return m_impl ? m_impl->activeBackend : "";
}

void BitPerfectAudioEngine::setAudioDevice(const std::string &deviceId) {
  if (!m_impl) {
    return;
  }
  m_impl->activeDevice = deviceId;
  if (m_impl->engineHandler) {
    QString be = m_impl->activeBackend.empty()
                     ? QStringLiteral("Linux PipeWire Lock-Free Client")
                     : QString::fromStdString(m_impl->activeBackend);
    QString dev = deviceId.empty() ? QStringLiteral("default")
                                   : QString::fromStdString(deviceId);
    m_impl->engineHandler->applyOutputProfile(
        PicAudioEngine::Engine::OutputProfileRequest{be, dev});
    if (m_impl->settings) {
      using namespace Qt::StringLiterals;
      m_impl->settings->set<PicAudioEngine::Settings::Core::AudioOutput>(be + u"|"_s +
                                                                 dev);
    }
  }
}

std::string BitPerfectAudioEngine::getAudioDevice() const {
  return m_impl ? m_impl->activeDevice : "";
}

std::vector<core::IAudioEnginePort::DeviceInfo>
BitPerfectAudioEngine::getAvailableDevices(const std::string &backend) const {
  std::vector<DeviceInfo> result;
  if (!m_impl || !m_impl->engineHandler) {
    return result;
  }

  QString be = backend.empty() ? QString::fromStdString(m_impl->activeBackend)
                               : QString::fromStdString(backend);
  auto devs = m_impl->engineHandler->getOutputDevices(be);
  for (const auto &d : devs) {
    result.push_back(DeviceInfo{d.name.toStdString(), d.name.toStdString(),
                                d.desc.toStdString(), d.name == "default"});
  }

  if (result.empty()) {
    result.push_back(
        DeviceInfo{"default", "Default Device", "System Default Output", true});
  }
  return result;
}

void BitPerfectAudioEngine::setBufferLatencyMs(uint32_t ms) {
  if (m_impl) {
    m_impl->bufferLatencyMs = ms;
    if (m_impl->settings) {
      m_impl->settings->fileSet(QStringLiteral("ALSA/BufferLength"),
                                static_cast<int>(ms));
    }
  }
}

uint32_t BitPerfectAudioEngine::getBufferLatencyMs() const {
  return m_impl ? m_impl->bufferLatencyMs : 50;
}

void BitPerfectAudioEngine::setBitPerfectExclusive(bool exclusive) {
  if (m_impl) {
    m_impl->bitPerfectExclusive = exclusive;
  }
}

bool BitPerfectAudioEngine::isBitPerfectExclusive() const {
  return m_impl ? m_impl->bitPerfectExclusive : true;
}

uint64_t BitPerfectAudioEngine::getXrunCount() const { return 0; }

void BitPerfectAudioEngine::setChannelMatrixCoefficient(size_t outCh,
                                                        size_t inCh,
                                                        float gain) {
  if (m_impl && outCh < 8 && inCh < 8) {
    m_impl->channelMatrix[outCh][inCh] = gain;
    m_impl->channelPreset = 5; // Custom
  }
}

float BitPerfectAudioEngine::getChannelMatrixCoefficient(size_t outCh,
                                                         size_t inCh) const {
  if (m_impl && outCh < 8 && inCh < 8) {
    return m_impl->channelMatrix[outCh][inCh];
  }
  return (outCh == inCh) ? 1.0f : 0.0f;
}

void BitPerfectAudioEngine::setChannelMatrixPreset(int preset) {
  if (!m_impl) {
    return;
  }
  m_impl->channelPreset = preset;
  for (int r = 0; r < 8; ++r) {
    for (int c = 0; c < 8; ++c) {
      m_impl->channelMatrix[r][c] = 0.0f;
    }
  }

  switch (preset) {
  case 0: // Passthrough
    for (int i = 0; i < 8; ++i) {
      m_impl->channelMatrix[i][i] = 1.0f;
    }
    m_impl->outputChannels = 2;
    break;
  case 1: // DownmixToStereo (ITU-R BS.775)
    m_impl->outputChannels = 2;
    m_impl->channelMatrix[0][0] = 0.4142f; // FL
    m_impl->channelMatrix[1][1] = 0.4142f; // FR
    m_impl->channelMatrix[0][2] = 0.2929f; // FC -> L
    m_impl->channelMatrix[1][2] = 0.2929f; // FC -> R
    m_impl->channelMatrix[0][4] = 0.2929f; // RL -> L
    m_impl->channelMatrix[1][5] = 0.2929f; // RR -> R
    break;
  case 2: // DownmixToMono
    m_impl->outputChannels = 1;
    m_impl->channelMatrix[0][0] = 0.5f;
    m_impl->channelMatrix[0][1] = 0.5f;
    break;
  case 3: // UpmixTo51
    m_impl->outputChannels = 6;
    m_impl->channelMatrix[0][0] = 1.0f;
    m_impl->channelMatrix[1][1] = 1.0f;
    m_impl->channelMatrix[2][0] = 0.5f;
    m_impl->channelMatrix[2][1] = 0.5f;
    m_impl->channelMatrix[3][0] = 0.3f;
    m_impl->channelMatrix[3][1] = 0.3f;
    m_impl->channelMatrix[4][0] = 0.7f;
    m_impl->channelMatrix[5][1] = 0.7f;
    break;
  case 4: // UpmixTo71
    m_impl->outputChannels = 8;
    for (int i = 0; i < 8; ++i) {
      m_impl->channelMatrix[i][i % 2] = 0.7f;
    }
    break;
  default:
    break;
  }
}

int BitPerfectAudioEngine::getChannelMatrixPreset() const {
  return m_impl ? m_impl->channelPreset : 0;
}

void BitPerfectAudioEngine::normalizeChannelMatrix() {
  if (!m_impl) {
    return;
  }
  for (int r = 0; r < 8; ++r) {
    float sum = 0.0f;
    for (int c = 0; c < 8; ++c) {
      sum += std::abs(m_impl->channelMatrix[r][c]);
    }
    if (sum > 1.0f) {
      for (int c = 0; c < 8; ++c) {
        m_impl->channelMatrix[r][c] /= sum;
      }
    }
  }
}

std::vector<float> BitPerfectAudioEngine::getChannelLevels() const {
  std::vector<float> levels(8, 0.0f);
  if (m_impl) {
    float cur = m_impl->currentAudioLevel.load(std::memory_order_relaxed);
    for (size_t i = 0; i < m_impl->outputChannels && i < 8; ++i) {
      levels[i] = cur;
    }
  }
  return levels;
}

void BitPerfectAudioEngine::setOutputChannelCount(uint8_t channels) {
  if (m_impl) {
    m_impl->outputChannels = std::clamp(channels, uint8_t{1}, uint8_t{8});
  }
}

uint8_t BitPerfectAudioEngine::getOutputChannelCount() const {
  return m_impl ? m_impl->outputChannels : 2;
}

std::string BitPerfectAudioEngine::getSerializedChannelMatrix() const {
  if (!m_impl) {
    return "{}";
  }
  QJsonObject root;
  root["preset"] = m_impl->channelPreset;
  root["outputChannels"] = m_impl->outputChannels;
  QJsonArray rows;
  for (int r = 0; r < 8; ++r) {
    QJsonArray row;
    for (int c = 0; c < 8; ++c) {
      row.append(static_cast<double>(m_impl->channelMatrix[r][c]));
    }
    rows.append(row);
  }
  root["matrix"] = rows;
  return QJsonDocument(root).toJson(QJsonDocument::Compact).toStdString();
}

bool BitPerfectAudioEngine::setSerializedChannelMatrix(
    const std::string &json) {
  if (!m_impl) {
    return false;
  }
  QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(json));
  if (!doc.isObject()) {
    return false;
  }
  QJsonObject root = doc.object();
  m_impl->channelPreset = root["preset"].toInt();
  m_impl->outputChannels = static_cast<uint8_t>(root["outputChannels"].toInt());
  QJsonArray rows = root["matrix"].toArray();
  for (int r = 0; r < 8 && r < rows.size(); ++r) {
    QJsonArray row = rows[r].toArray();
    for (int c = 0; c < 8 && c < row.size(); ++c) {
      m_impl->channelMatrix[r][c] = static_cast<float>(row[c].toDouble());
    }
  }
  return true;
}

void BitPerfectAudioEngine::setEqBandGain(size_t bandIndex, float gainDb) {
  if (m_impl && bandIndex < m_impl->eqBands.size()) {
    m_impl->eqBands[bandIndex] = gainDb;
    m_impl->eqPreset = "Custom";
  }
}

float BitPerfectAudioEngine::getEqBandGain(size_t bandIndex) const {
  if (m_impl && bandIndex < m_impl->eqBands.size()) {
    return m_impl->eqBands[bandIndex];
  }
  return 0.0f;
}

void BitPerfectAudioEngine::setEqBandGains(const std::vector<float> &gains) {
  if (m_impl) {
    m_impl->eqBands = gains;
    m_impl->eqPreset = "Custom";
  }
}

std::vector<float> BitPerfectAudioEngine::getEqBandGains() const {
  return m_impl ? m_impl->eqBands : std::vector<float>{};
}

void BitPerfectAudioEngine::setEqPreampGain(float gainDb) {
  if (m_impl) {
    m_impl->eqPreampGain = gainDb;
  }
}

float BitPerfectAudioEngine::getEqPreampGain() const {
  return m_impl ? m_impl->eqPreampGain : 0.0f;
}

void BitPerfectAudioEngine::setEqEnabled(bool enabled) {
  if (m_impl) {
    m_impl->eqEnabled = enabled;
  }
}

bool BitPerfectAudioEngine::isEqEnabled() const {
  return m_impl ? m_impl->eqEnabled : false;
}

void BitPerfectAudioEngine::setEqPreset(const std::string &preset) {
  if (!m_impl) {
    return;
  }
  m_impl->eqPreset = preset;
  if (preset == "Flat") {
    std::fill(m_impl->eqBands.begin(), m_impl->eqBands.end(), 0.0f);
    m_impl->eqPreampGain = 0.0f;
  } else if (preset == "Bass Boost") {
    std::fill(m_impl->eqBands.begin(), m_impl->eqBands.end(), 0.0f);
    if (m_impl->eqBands.size() >= 3) {
      m_impl->eqBands[0] = 5.0f;
      m_impl->eqBands[1] = 3.5f;
      m_impl->eqBands[2] = 1.5f;
    }
  } else if (preset == "Treble Boost") {
    std::fill(m_impl->eqBands.begin(), m_impl->eqBands.end(), 0.0f);
    if (m_impl->eqBands.size() >= 10) {
      m_impl->eqBands[7] = 2.0f;
      m_impl->eqBands[8] = 4.0f;
      m_impl->eqBands[9] = 5.0f;
    }
  }
}

std::string BitPerfectAudioEngine::getEqPreset() const {
  return m_impl ? m_impl->eqPreset : "Flat";
}

void BitPerfectAudioEngine::resetEq() {
  if (m_impl) {
    m_impl->eqPreampGain = 0.0f;
    std::fill(m_impl->eqBands.begin(), m_impl->eqBands.end(), 0.0f);
  }
  setEqPreset("Flat");
}

void BitPerfectAudioEngine::setEqBandCount(size_t bandCount) {
  if (m_impl) {
    m_impl->eqBandCount = bandCount;
    m_impl->eqBands.resize(bandCount, 0.0f);
  }
}

size_t BitPerfectAudioEngine::getEqBandCount() const {
  return m_impl ? m_impl->eqBandCount : 10;
}

std::vector<std::string>
BitPerfectAudioEngine::getEqBandFrequencyLabels() const {
  if (!m_impl || m_impl->eqBandCount == 10) {
    return {"31 Hz", "63 Hz", "125 Hz", "250 Hz", "500 Hz",
            "1 kHz", "2 kHz", "4 kHz",  "8 kHz",  "16 kHz"};
  }
  std::vector<std::string> labels;
  for (size_t i = 0; i < m_impl->eqBandCount; ++i) {
    labels.push_back("Band " + std::to_string(i + 1));
  }
  return labels;
}

void BitPerfectAudioEngine::setDspRoutingOrder(int order) {
  if (m_impl) {
    m_impl->dspRoutingOrder = order;
  }
}

int BitPerfectAudioEngine::getDspRoutingOrder() const {
  return m_impl ? m_impl->dspRoutingOrder : 0;
}

void BitPerfectAudioEngine::setDspMasterEnabled(bool enabled) {
  if (m_impl) {
    m_impl->dspMasterEnabled = enabled;
  }
}

bool BitPerfectAudioEngine::isDspMasterEnabled() const {
  return m_impl ? m_impl->dspMasterEnabled : true;
}

void BitPerfectAudioEngine::setReplayGainMode(int mode) {
  if (m_impl) {
    m_impl->replayGainMode = mode;
    if (m_impl->settings) {
      m_impl->settings->set<PicAudioEngine::Settings::Core::RGMode>(mode);
    }
  }
}

int BitPerfectAudioEngine::getReplayGainMode() const {
  return m_impl ? m_impl->replayGainMode : 3;
}

void BitPerfectAudioEngine::setReplayGainPreamp(float taggedDb,
                                                float untaggedDb) {
  if (m_impl) {
    m_impl->replayGainTaggedPreamp = taggedDb;
    m_impl->replayGainUntaggedPreamp = untaggedDb;
    if (m_impl->settings) {
      m_impl->settings->set<PicAudioEngine::Settings::Core::RGPreAmp>(taggedDb);
      m_impl->settings->set<PicAudioEngine::Settings::Core::NonRGPreAmp>(untaggedDb);
    }
  }
}

void BitPerfectAudioEngine::setTruePeakLimiterEnabled(bool enabled) {
  if (m_impl) {
    m_impl->truePeakLimiter = enabled;
  }
}

bool BitPerfectAudioEngine::isTruePeakLimiterEnabled() const {
  return m_impl ? m_impl->truePeakLimiter : true;
}

void BitPerfectAudioEngine::setCurrentTrackReplayGain(
    bool hasGain, float trackGain, float trackPeak, float albumGain,
    float albumPeak, bool isAlbum) {
  (void)hasGain;
  (void)trackGain;
  (void)trackPeak;
  (void)albumGain;
  (void)albumPeak;
  (void)isAlbum;
}

void BitPerfectAudioEngine::setCrossfadeEnabled(bool enabled) {
  if (m_impl) {
    m_impl->crossfadeEnabled = enabled;
  }
}

bool BitPerfectAudioEngine::isCrossfadeEnabled() const {
  return m_impl ? m_impl->crossfadeEnabled : false;
}

void BitPerfectAudioEngine::setCrossfadeDurationSec(float sec) {
  if (m_impl) {
    m_impl->crossfadeDurationSec = sec;
  }
}

float BitPerfectAudioEngine::getCrossfadeDurationSec() const {
  return m_impl ? m_impl->crossfadeDurationSec : 2.0f;
}

void BitPerfectAudioEngine::setCrossfadeCurve(int curve) {
  if (m_impl) {
    m_impl->crossfadeCurve = curve;
  }
}

int BitPerfectAudioEngine::getCrossfadeCurve() const {
  return m_impl ? m_impl->crossfadeCurve : 0;
}

void BitPerfectAudioEngine::setGaplessEnabled(bool enabled) {
  if (m_impl) {
    m_impl->gaplessEnabled = enabled;
    if (m_impl->settings) {
      m_impl->settings->set<PicAudioEngine::Settings::Core::GaplessPlayback>(enabled);
    }
  }
}

bool BitPerfectAudioEngine::isGaplessEnabled() const {
  return m_impl ? m_impl->gaplessEnabled : true;
}

void BitPerfectAudioEngine::setDitherMode(int mode) {
  if (m_impl) {
    m_impl->ditherMode = mode;
    if (m_impl->settings) {
      m_impl->settings->set<PicAudioEngine::Settings::Core::OutputDither>(mode != 0);
    }
  }
}

int BitPerfectAudioEngine::getDitherMode() const {
  return m_impl ? m_impl->ditherMode : 1;
}

float BitPerfectAudioEngine::getBufferFillPercentage() const {
  return isPlaying() ? 100.0f : 0.0f;
}

std::string BitPerfectAudioEngine::getActiveDecoderName() const {
  return "Direct In-Process / FFmpeg Native";
}

PicAudioEngine::EngineHandler *BitPerfectAudioEngine::engineHandler() const {
  return m_impl ? m_impl->engineHandler.get() : nullptr;
}

PicAudioEngine::PlayerController *BitPerfectAudioEngine::playerController() const {
  return m_impl ? m_impl->playerController.get() : nullptr;
}

} // namespace adapters
