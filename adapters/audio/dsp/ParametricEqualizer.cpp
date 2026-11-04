#include "ParametricEqualizer.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace adapters {

namespace {
constexpr float kPi = 3.14159265358979323846f;

inline float softLimit(float x) {
  constexpr float kThreshold = 0.95f;
  constexpr float kHeadroom = 1.0f - kThreshold;
  if (x > kThreshold) {
    return kThreshold + kHeadroom * std::tanh((x - kThreshold) / kHeadroom);
  } else if (x < -kThreshold) {
    return -kThreshold + kHeadroom * std::tanh((x + kThreshold) / kHeadroom);
  }
  return x;
}

static const std::array<float, 10> kBaseFreqs10 = {
    31.25f,  62.5f,   125.0f,  250.0f,  500.0f,
    1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f};

static const struct PresetEntry {
  const char *name;
  std::array<float, 10> gains;
} kPresets[] = {
    {"Flat", {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}},
    {"Bass Boost",
     {5.5f, 4.5f, 3.5f, 2.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}},
    {"Treble Boost",
     {0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 1.5f, 2.5f, 4.0f, 5.0f, 6.0f}},
    {"Rock", {4.5f, 3.0f, 1.5f, 0.0f, -1.0f, -0.5f, 1.5f, 3.0f, 4.0f, 4.5f}},
    {"Pop", {-1.0f, 0.5f, 2.0f, 3.0f, 3.5f, 2.5f, 1.0f, 0.0f, 1.0f, 2.0f}},
    {"Jazz", {3.0f, 2.0f, 1.0f, 1.5f, -1.0f, -1.0f, 0.0f, 1.5f, 2.5f, 3.5f}},
    {"Classical",
     {4.0f, 3.0f, 2.0f, 1.5f, -1.0f, -1.0f, 0.0f, 2.0f, 3.0f, 3.5f}},
    {"Vocal", {-2.0f, -1.0f, 0.0f, 2.5f, 4.0f, 3.5f, 2.0f, 0.5f, -1.0f, -2.0f}},
    {"Reference",
     {0.5f, 0.2f, 0.0f, -0.2f, -0.2f, 0.0f, 0.2f, 0.4f, 0.5f, 0.5f}},
};

float interpolateGainFrom10(const std::array<float, 10> &g10, float freq) {
  if (freq <= kBaseFreqs10.front())
    return g10.front();
  if (freq >= kBaseFreqs10.back())
    return g10.back();

  for (size_t i = 0; i < 9; ++i) {
    if (freq >= kBaseFreqs10[i] && freq <= kBaseFreqs10[i + 1]) {
      float logF = std::log2(freq);
      float log0 = std::log2(kBaseFreqs10[i]);
      float log1 = std::log2(kBaseFreqs10[i + 1]);
      float t = (logF - log0) / (log1 - log0);
      return g10[i] * (1.0f - t) + g10[i + 1] * t;
    }
  }
  return 0.0f;
}

} // namespace

ParametricEqualizer::ParametricEqualizer() {
  std::lock_guard<std::mutex> lock(m_mutex);
  applyBandCountLocked(10);
  recalculateCoefficientsLocked();
  publishConfigurationLocked();
}

void ParametricEqualizer::applyBandCountLocked(size_t count) {
  if (count == 31) {
    m_bandCount = 31;
    m_filterQ = 4.32f;
    m_bandFrequencies = {
        20.0f,    25.0f,    31.5f,   40.0f,   50.0f,   63.0f,   80.0f,
        100.0f,   125.0f,   160.0f,  200.0f,  250.0f,  315.0f,  400.0f,
        500.0f,   630.0f,   800.0f,  1000.0f, 1250.0f, 1600.0f, 2000.0f,
        2500.0f,  3150.0f,  4000.0f, 5000.0f, 6300.0f, 8000.0f, 10000.0f,
        12500.0f, 16000.0f, 20000.0f};
    m_bandLabels = {"20",    "25",   "31.5", "40",  "50",   "63",   "80",
                    "100",   "125",  "160",  "200", "250",  "315",  "400",
                    "500",   "630",  "800",  "1k",  "1.2k", "1.6k", "2k",
                    "2.5k",  "3.1k", "4k",   "5k",  "6.3k", "8k",   "10k",
                    "12.5k", "16k",  "20k"};
  } else if (count == 15) {
    m_bandCount = 15;
    m_filterQ = 2.14f;
    m_bandFrequencies = {25.0f,   40.0f,   63.0f,   100.0f,   160.0f,
                         250.0f,  400.0f,  630.0f,  1000.0f,  1600.0f,
                         2500.0f, 4000.0f, 6300.0f, 10000.0f, 16000.0f};
    m_bandLabels = {"25 Hz",   "40 Hz",  "63 Hz",   "100 Hz", "160 Hz",
                    "250 Hz",  "400 Hz", "630 Hz",  "1 kHz",  "1.6 kHz",
                    "2.5 kHz", "4 kHz",  "6.3 kHz", "10 kHz", "16 kHz"};
  } else {
    m_bandCount = 10;
    m_filterQ = 1.41421356f;
    m_bandFrequencies = {31.25f,  62.5f,   125.0f,  250.0f,  500.0f,
                         1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f};
    m_bandLabels = {"31 Hz", "63 Hz", "125 Hz", "250 Hz", "500 Hz",
                    "1 kHz", "2 kHz", "4 kHz",  "8 kHz",  "16 kHz"};
  }

  m_bandGainsDb.assign(m_bandCount, 0.0f);
  for (size_t i = 0; i < kMaxBands; ++i) {
    m_stagingCoeffs[i] = BiquadCoefficients{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, true};
  }
}

void ParametricEqualizer::publishConfigurationLocked() {
  uint32_t activeIdx = m_activeConfigIndex.load(std::memory_order_relaxed);
  uint32_t inactiveIdx = 1 - activeIdx;

  auto &cfg = m_configs[inactiveIdx];
  cfg.coeffs = m_stagingCoeffs;
  cfg.bandCount = m_bandCount;
  cfg.channels = m_channels;
  cfg.preampLinear = m_preampLinear;
  cfg.bypassed = m_bypassed.load(std::memory_order_relaxed);

  m_activeConfigIndex.store(inactiveIdx, std::memory_order_release);
}

void ParametricEqualizer::configure(uint32_t sampleRate, uint8_t channels) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_sampleRate = (sampleRate > 0) ? sampleRate : 44100;
  m_channels =
      std::clamp<uint8_t>(channels, 1, static_cast<uint8_t>(kMaxChannels));
  recalculateCoefficientsLocked();
  publishConfigurationLocked();
  m_resetStatePending.store(true, std::memory_order_release);
}

void ParametricEqualizer::setBandCount(size_t count) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (count != 10 && count != 15 && count != 31) {
    count = 10;
  }
  if (count == m_bandCount) {
    return;
  }

  std::vector<float> oldFreqs = m_bandFrequencies;
  std::vector<float> oldGains = m_bandGainsDb;

  applyBandCountLocked(count);

  if (!oldFreqs.empty() && !oldGains.empty()) {
    for (size_t i = 0; i < m_bandCount; ++i) {
      float f = m_bandFrequencies[i];
      if (f <= oldFreqs.front()) {
        m_bandGainsDb[i] = oldGains.front();
      } else if (f >= oldFreqs.back()) {
        m_bandGainsDb[i] = oldGains.back();
      } else {
        for (size_t j = 0; j < oldFreqs.size() - 1; ++j) {
          if (f >= oldFreqs[j] && f <= oldFreqs[j + 1]) {
            float lf = std::log2(f);
            float l0 = std::log2(oldFreqs[j]);
            float l1 = std::log2(oldFreqs[j + 1]);
            float t = (lf - l0) / (l1 - l0);
            m_bandGainsDb[i] = oldGains[j] * (1.0f - t) + oldGains[j + 1] * t;
            break;
          }
        }
      }
    }
  }

  recalculateCoefficientsLocked();
  publishConfigurationLocked();
  m_resetStatePending.store(true, std::memory_order_release);
}

size_t ParametricEqualizer::getBandCount() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_bandCount;
}

std::vector<float> ParametricEqualizer::getBandFrequencies() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_bandFrequencies;
}

std::vector<std::string> ParametricEqualizer::getBandFrequencyLabels() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_bandLabels;
}

void ParametricEqualizer::setBandGain(size_t bandIndex, float gainDb) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (bandIndex >= m_bandCount) {
    return;
  }
  m_bandGainsDb[bandIndex] = std::clamp(gainDb, -12.0f, 12.0f);
  recalculateBandLocked(bandIndex);
  publishConfigurationLocked();
}

float ParametricEqualizer::getBandGain(size_t bandIndex) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (bandIndex >= m_bandCount) {
    return 0.0f;
  }
  return m_bandGainsDb[bandIndex];
}

void ParametricEqualizer::setBandGains(const std::vector<float> &gains) {
  std::lock_guard<std::mutex> lock(m_mutex);
  const size_t count = std::min(gains.size(), m_bandCount);
  for (size_t i = 0; i < count; ++i) {
    m_bandGainsDb[i] = std::clamp(gains[i], -12.0f, 12.0f);
    recalculateBandLocked(i);
  }
  publishConfigurationLocked();
}

std::vector<float> ParametricEqualizer::getBandGains() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_bandGainsDb;
}

void ParametricEqualizer::setPreampGain(float gainDb) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_preampDb = std::clamp(gainDb, -12.0f, 12.0f);
  m_preampLinear = std::pow(10.0f, m_preampDb / 20.0f);
  publishConfigurationLocked();
}

float ParametricEqualizer::getPreampGain() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_preampDb;
}

void ParametricEqualizer::setBypassed(bool bypass) {
  m_bypassed.store(bypass, std::memory_order_release);
  std::lock_guard<std::mutex> lock(m_mutex);
  publishConfigurationLocked();
}

bool ParametricEqualizer::isBypassed() const {
  return m_bypassed.load(std::memory_order_acquire);
}

void ParametricEqualizer::setPreset(const std::string &presetName) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_preset = presetName;

  for (const auto &entry : kPresets) {
    if (presetName == entry.name) {
      if (m_bandCount == 10) {
        for (size_t i = 0; i < 10; ++i) {
          m_bandGainsDb[i] = entry.gains[i];
        }
      } else {
        for (size_t i = 0; i < m_bandCount; ++i) {
          m_bandGainsDb[i] =
              interpolateGainFrom10(entry.gains, m_bandFrequencies[i]);
        }
      }
      recalculateCoefficientsLocked();
      publishConfigurationLocked();
      return;
    }
  }
}

std::string ParametricEqualizer::getPreset() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_preset;
}

void ParametricEqualizer::reset() {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_preampDb = 0.0f;
  m_preampLinear = 1.0f;
  m_preset = "Flat";
  std::fill(m_bandGainsDb.begin(), m_bandGainsDb.end(), 0.0f);
  recalculateCoefficientsLocked();
  publishConfigurationLocked();
  m_resetStatePending.store(true, std::memory_order_release);
}

void ParametricEqualizer::recalculateBandLocked(size_t bandIndex) {
  if (bandIndex >= m_bandCount) {
    return;
  }

  const float gainDb = m_bandGainsDb[bandIndex];
  if (std::abs(gainDb) < 0.05f) {
    m_stagingCoeffs[bandIndex] =
        BiquadCoefficients{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, true};
    return;
  }

  const float f0 = std::min(m_bandFrequencies[bandIndex],
                            static_cast<float>(m_sampleRate) * 0.45f);
  const float omega = 2.0f * kPi * f0 / static_cast<float>(m_sampleRate);
  const float cosOmega = std::cos(omega);
  const float sinOmega = std::sin(omega);
  const float alpha = sinOmega / (2.0f * m_filterQ);
  const float A = std::pow(10.0f, gainDb / 40.0f);

  const float b0 = 1.0f + alpha * A;
  const float b1 = -2.0f * cosOmega;
  const float b2 = 1.0f - alpha * A;
  const float a0 = 1.0f + alpha / A;
  const float a1 = -2.0f * cosOmega;
  const float a2 = 1.0f - alpha / A;

  const float invA0 = 1.0f / a0;
  m_stagingCoeffs[bandIndex].b0 = b0 * invA0;
  m_stagingCoeffs[bandIndex].b1 = b1 * invA0;
  m_stagingCoeffs[bandIndex].b2 = b2 * invA0;
  m_stagingCoeffs[bandIndex].a1 = a1 * invA0;
  m_stagingCoeffs[bandIndex].a2 = a2 * invA0;
  m_stagingCoeffs[bandIndex].isPassthrough = false;
}

void ParametricEqualizer::recalculateCoefficientsLocked() {
  for (size_t i = 0; i < m_bandCount; ++i) {
    recalculateBandLocked(i);
  }
}

void ParametricEqualizer::process(float *buffer, size_t frames) {
  if (!buffer || frames == 0) {
    return;
  }

  if (m_bypassed.load(std::memory_order_relaxed)) {
    return;
  }

  const uint32_t activeIdx =
      m_activeConfigIndex.load(std::memory_order_acquire);
  const auto &config = m_configs[activeIdx];
  if (config.bypassed) {
    return;
  }

  if (m_resetStatePending.exchange(false, std::memory_order_acq_rel)) {
    for (auto &b : m_states) {
      for (auto &c : b) {
        c = BiquadState{0.0f, 0.0f};
      }
    }
  }

  const uint8_t channels =
      std::min(config.channels, static_cast<uint8_t>(kMaxChannels));
  const float preamp = config.preampLinear;
  const size_t bandCount = std::min(config.bandCount, kMaxBands);

  for (size_t f = 0; f < frames; ++f) {
    for (size_t c = 0; c < channels; ++c) {
      float sample = buffer[f * channels + c] * preamp;

      for (size_t b = 0; b < bandCount; ++b) {
        const auto &coeffs = config.coeffs[b];
        if (coeffs.isPassthrough) {
          continue;
        }

        auto &state = m_states[b][c];
        const float out = coeffs.b0 * sample + state.s1;
        state.s1 = coeffs.b1 * sample - coeffs.a1 * out + state.s2;
        state.s2 = coeffs.b2 * sample - coeffs.a2 * out;

        // Anti-denormal flush
        if (std::abs(state.s1) < 1.0e-15f)
          state.s1 = 0.0f;
        if (std::abs(state.s2) < 1.0e-15f)
          state.s2 = 0.0f;

        sample = out;
      }

      // Soft limiting to prevent harsh digital clipping
      buffer[f * channels + c] = softLimit(sample);
    }
  }
}

std::string ParametricEqualizer::serialize() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  std::ostringstream ss;
  ss << "{"
     << "\"bandCount\":" << m_bandCount << ","
     << "\"bypassed\":"
     << (m_bypassed.load(std::memory_order_relaxed) ? "true" : "false") << ","
     << "\"preampDb\":" << std::fixed << std::setprecision(2) << m_preampDb
     << ","
     << "\"preset\":\"" << m_preset << "\","
     << "\"gains\":[";
  for (size_t i = 0; i < m_bandCount; ++i) {
    ss << std::fixed << std::setprecision(2) << m_bandGainsDb[i];
    if (i + 1 < m_bandCount)
      ss << ",";
  }
  ss << "]}";
  return ss.str();
}

bool ParametricEqualizer::deserialize(const std::string &json) {
  if (json.empty() || json.front() != '{' || json.back() != '}') {
    return false;
  }
  try {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto findKey = [&json](const std::string &key) -> size_t {
      return json.find("\"" + key + "\"");
    };

    size_t bcPos = findKey("bandCount");
    if (bcPos != std::string::npos) {
      size_t col = json.find(':', bcPos);
      if (col != std::string::npos) {
        int bc = std::stoi(json.substr(col + 1));
        applyBandCountLocked(static_cast<size_t>(bc));
      }
    }

    size_t bypPos = findKey("bypassed");
    if (bypPos != std::string::npos) {
      size_t col = json.find(':', bypPos);
      if (col != std::string::npos) {
        std::string val = json.substr(col + 1, 5);
        bool byp = (val.find("true") != std::string::npos);
        m_bypassed.store(byp, std::memory_order_release);
      }
    }

    size_t prePos = findKey("preampDb");
    if (prePos != std::string::npos) {
      size_t col = json.find(':', prePos);
      if (col != std::string::npos) {
        m_preampDb = std::stof(json.substr(col + 1));
        m_preampLinear = std::pow(10.0f, m_preampDb / 20.0f);
      }
    }

    size_t prsPos = findKey("preset");
    if (prsPos != std::string::npos) {
      size_t q1 = json.find('"', prsPos + 8);
      if (q1 != std::string::npos) {
        size_t q2 = json.find('"', q1 + 1);
        if (q2 != std::string::npos) {
          m_preset = json.substr(q1 + 1, q2 - q1 - 1);
        }
      }
    }

    size_t gainsPos = findKey("gains");
    if (gainsPos != std::string::npos) {
      size_t bOpen = json.find('[', gainsPos);
      size_t bClose = json.find(']', bOpen);
      if (bOpen != std::string::npos && bClose != std::string::npos) {
        std::string listStr = json.substr(bOpen + 1, bClose - bOpen - 1);
        std::istringstream iss(listStr);
        std::string token;
        size_t idx = 0;
        while (std::getline(iss, token, ',') && idx < m_bandCount) {
          m_bandGainsDb[idx] = std::stof(token);
          recalculateBandLocked(idx);
          idx++;
        }
      }
    }

    publishConfigurationLocked();
    m_resetStatePending.store(true, std::memory_order_release);
    return true;
  } catch (...) {
    return false;
  }
}

} // namespace adapters
