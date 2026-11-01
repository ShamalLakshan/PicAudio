#include "SpectrumVisualizerPlugin.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <numbers>

namespace picaudio::plugins {

SpectrumVisualizerPlugin::SpectrumVisualizerPlugin()
    : m_hannWindow(kFftSize, 0.0f), m_inputRingBuffer(kFftSize, 0.0f),
      m_currentBands(kDefaultBands, 0.0f),
      m_smoothedBands(kDefaultBands, 0.0f) {}

const char *SpectrumVisualizerPlugin::getId() const noexcept {
  return "com.picaudio.spectrum";
}

const char *SpectrumVisualizerPlugin::getName() const noexcept {
  return "Spectrum Visualizer";
}

const char *SpectrumVisualizerPlugin::getVersion() const noexcept {
  return "1.0.0";
}

const char *SpectrumVisualizerPlugin::getAuthor() const noexcept {
  return "PicAudio Team";
}

const char *SpectrumVisualizerPlugin::getDescription() const noexcept {
  return "Real-time FFT audio spectrum visualizer plugin with logarithmic "
         "frequency binning, temporal smoothing, and peak decay.";
}

uint32_t SpectrumVisualizerPlugin::getApiVersion() const noexcept {
  return PICAUDIO_PLUGIN_API_VERSION;
}

uint32_t SpectrumVisualizerPlugin::getCapabilities() const noexcept {
  return plugin::PICAUDIO_CAP_VISUALIZER | plugin::PICAUDIO_CAP_UI;
}

bool SpectrumVisualizerPlugin::initialize() {
  std::lock_guard<std::mutex> lock(m_mutex);

  // Precalculate Hann window coefficients: 0.5 * (1 - cos(2 * pi * n / (N -
  // 1)))
  for (size_t i = 0; i < kFftSize; ++i) {
    m_hannWindow[i] =
        0.5f * (1.0f - std::cos(2.0f * std::numbers::pi_v<float> *
                                static_cast<float>(i) /
                                static_cast<float>(kFftSize - 1)));
  }

  std::fill(m_inputRingBuffer.begin(), m_inputRingBuffer.end(), 0.0f);
  std::fill(m_currentBands.begin(), m_currentBands.end(), 0.0f);
  std::fill(m_smoothedBands.begin(), m_smoothedBands.end(), 0.0f);
  m_ringWritePos = 0;
  m_accumulatedFrames = 0;
  m_enabled = true;

  return true;
}

void SpectrumVisualizerPlugin::start() {
  // Background worker not required; processing driven by player audio callbacks
}

void SpectrumVisualizerPlugin::stop() {
  std::lock_guard<std::mutex> lock(m_mutex);
  std::fill(m_smoothedBands.begin(), m_smoothedBands.end(), 0.0f);
  std::fill(m_currentBands.begin(), m_currentBands.end(), 0.0f);
}

void SpectrumVisualizerPlugin::shutdown() { stop(); }

void *SpectrumVisualizerPlugin::getCapabilityInterface(uint32_t capability) {
  if (capability == plugin::PICAUDIO_CAP_VISUALIZER) {
    return static_cast<plugin::IVisualizerPlugin *>(this);
  }
  if (capability == plugin::PICAUDIO_CAP_UI) {
    return static_cast<plugin::IUiPlugin *>(this);
  }
  return nullptr;
}

void SpectrumVisualizerPlugin::setHostUiServices(
    const plugin::PicAudioHostUiServices *services) {
  m_hostUiServices = services;
}

size_t SpectrumVisualizerPlugin::getCustomActions(
    plugin::PicAudioCustomAction *outActions, size_t maxActions) {
  if (!outActions || maxActions < 1) {
    return 0;
  }
  outActions[0].id = "open_visualizer";
  outActions[0].label = "Open Visualizer View";
  outActions[0].tooltip =
      "Jump to full-stage audio visualizer view in Now Playing";
  outActions[0].iconName = "equalizer";
  outActions[0].isPrimary = true;
  return 1;
}

bool SpectrumVisualizerPlugin::triggerAction(const char *actionId) {
  if (!actionId) {
    return false;
  }
  if (std::strcmp(actionId, "open_visualizer") == 0) {
    if (m_hostUiServices && m_hostUiServices->navigateToView) {
      m_hostUiServices->navigateToView(m_hostUiServices->hostContext, 4);
      return true;
    }
  }
  return false;
}

bool SpectrumVisualizerPlugin::isEnabled() const noexcept { return m_enabled; }

void SpectrumVisualizerPlugin::setEnabled(bool enabled) {
  m_enabled = enabled;
  if (!enabled) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::fill(m_smoothedBands.begin(), m_smoothedBands.end(), 0.0f);
    std::fill(m_currentBands.begin(), m_currentBands.end(), 0.0f);
  }
}

void SpectrumVisualizerPlugin::processAudioBuffer(const float *buffer,
                                                  size_t frames,
                                                  uint32_t sampleRate,
                                                  uint8_t channels) {
  if (!m_enabled || !buffer || frames == 0 || channels == 0) {
    return;
  }

  std::lock_guard<std::mutex> lock(m_mutex);
  m_sampleRate = (sampleRate > 0) ? sampleRate : 48000;

  for (size_t f = 0; f < frames; ++f) {
    float monoSample = 0.0f;
    for (uint8_t c = 0; c < channels; ++c) {
      monoSample += buffer[f * channels + c];
    }
    monoSample /= static_cast<float>(channels);

    m_inputRingBuffer[m_ringWritePos] = monoSample;
    m_ringWritePos = (m_ringWritePos + 1) % kFftSize;
    ++m_accumulatedFrames;
  }

  // Run FFT calculation periodically (e.g. after accumulating at least 256 new
  // frames)
  if (m_accumulatedFrames >= 256) {
    computeFftAndBands();
    m_accumulatedFrames = 0;
  }
}

size_t SpectrumVisualizerPlugin::getFrequencyBands(float *outBands,
                                                   size_t maxBands) const {
  if (!outBands || maxBands == 0) {
    return 0;
  }

  std::lock_guard<std::mutex> lock(m_mutex);
  const size_t copyCount = std::min(maxBands, m_smoothedBands.size());
  for (size_t i = 0; i < copyCount; ++i) {
    outBands[i] = m_smoothedBands[i];
  }
  return copyCount;
}

void SpectrumVisualizerPlugin::radix2Fft(
    std::vector<std::complex<float>> &data) {
  const size_t n = data.size();
  if (n <= 1) {
    return;
  }

  // Bit-reversal permutation
  size_t j = 0;
  for (size_t i = 0; i < n; ++i) {
    if (i < j) {
      std::swap(data[i], data[j]);
    }
    size_t bit = n >> 1;
    while (bit >= 1 && (j & bit)) {
      j ^= bit;
      bit >>= 1;
    }
    j ^= bit;
  }

  // Cooley-Tukey Radix-2 butterflies
  for (size_t len = 2; len <= n; len <<= 1) {
    const float angle =
        -2.0f * std::numbers::pi_v<float> / static_cast<float>(len);
    const std::complex<float> wlen(std::cos(angle), std::sin(angle));
    for (size_t i = 0; i < n; i += len) {
      std::complex<float> w(1.0f, 0.0f);
      for (size_t k = 0; k < len / 2; ++k) {
        const std::complex<float> u = data[i + k];
        const std::complex<float> v = data[i + k + len / 2] * w;
        data[i + k] = u + v;
        data[i + k + len / 2] = u - v;
        w *= wlen;
      }
    }
  }
}

void SpectrumVisualizerPlugin::computeFftAndBands() {
  std::vector<std::complex<float>> fftBuffer(kFftSize);

  // Unroll circular buffer and apply window function
  for (size_t i = 0; i < kFftSize; ++i) {
    const size_t ringIdx = (m_ringWritePos + i) % kFftSize;
    fftBuffer[i] =
        std::complex<float>(m_inputRingBuffer[ringIdx] * m_hannWindow[i], 0.0f);
  }

  radix2Fft(fftBuffer);

  // Magnitude spectrum computation
  const size_t halfSize = kFftSize / 2;
  std::vector<float> magnitudes(halfSize);
  for (size_t i = 0; i < halfSize; ++i) {
    const float mag = std::abs(fftBuffer[i]) / static_cast<float>(halfSize);
    magnitudes[i] = mag;
  }

  // Logarithmic frequency binning across audible range (20 Hz - 20,000 Hz)
  const float minFreq = 20.0f;
  const float nyquist = static_cast<float>(m_sampleRate) * 0.5f;
  const float maxFreq = std::min(20000.0f, nyquist);
  const float hzPerBin =
      static_cast<float>(m_sampleRate) / static_cast<float>(kFftSize);

  const size_t numBands = m_currentBands.size();
  for (size_t b = 0; b < numBands; ++b) {
    const float fracStart =
        static_cast<float>(b) / static_cast<float>(numBands);
    const float fracEnd =
        static_cast<float>(b + 1) / static_cast<float>(numBands);

    const float fLow = minFreq * std::pow(maxFreq / minFreq, fracStart);
    const float fHigh = minFreq * std::pow(maxFreq / minFreq, fracEnd);

    size_t binStart = static_cast<size_t>(std::floor(fLow / hzPerBin));
    size_t binEnd = static_cast<size_t>(std::ceil(fHigh / hzPerBin));

    binStart = std::clamp(binStart, size_t(0), halfSize - 1);
    binEnd = std::clamp(binEnd, binStart + 1, halfSize);

    float peakVal = 0.0f;
    for (size_t k = binStart; k < binEnd; ++k) {
      peakVal = std::max(peakVal, magnitudes[k]);
    }

    // dB scaling and normalization to [0.0f, 1.0f]
    // Reference dynamic range: -60 dB to 0 dB
    float db = 20.0f * std::log10(std::max(peakVal, 1e-4f));
    float normalized = std::clamp((db + 60.0f) / 60.0f, 0.0f, 1.0f);

    // Slight low-frequency boost and high-frequency compensation
    const float eqCurve =
        1.0f + 0.3f * std::sin(fracStart * std::numbers::pi_v<float>);
    normalized = std::clamp(normalized * eqCurve, 0.0f, 1.0f);

    m_currentBands[b] = normalized;

    // Attack & decay smoothing
    // Fast attack (0.7) when rising, gentle decay (0.2) when falling
    if (normalized >= m_smoothedBands[b]) {
      m_smoothedBands[b] = m_smoothedBands[b] * 0.3f + normalized * 0.7f;
    } else {
      m_smoothedBands[b] = m_smoothedBands[b] * 0.85f + normalized * 0.15f;
    }
  }
}

} // namespace picaudio::plugins

extern "C" {

PICAUDIO_PLUGIN_EXPORT picaudio::plugin::IPicAudioPlugin *create_plugin() {
  return new (std::nothrow) picaudio::plugins::SpectrumVisualizerPlugin();
}

PICAUDIO_PLUGIN_EXPORT void
destroy_plugin(picaudio::plugin::IPicAudioPlugin *plugin) {
  delete plugin;
}
}
