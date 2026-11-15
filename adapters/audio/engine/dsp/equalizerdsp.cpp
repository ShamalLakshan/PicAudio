#include "equalizerdsp.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace PicAudioEngine {

namespace {

static constexpr std::array<float, 10> kBaseFreqs10 = {
    31.25f,  62.5f,   125.0f,  250.0f,  500.0f,
    1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f};

struct PresetEntry {
  const char *name;
  std::array<float, 10> gains;
};

static constexpr PresetEntry kPresetEntries[] = {
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

static float interpolateGainFrom10(const std::array<float, 10> &g10,
                                   float freq) {
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

EqualizerDsp::EqualizerDsp() {
  m_bandFrequencies = bandFrequenciesForCount(10);
  m_bandGainsDb.assign(10, 0.0f);
  recalculateCoefficientsLocked();
}

QString EqualizerDsp::name() const {
  return QStringLiteral("Parametric Equalizer");
}

QString EqualizerDsp::id() const { return QStringLiteral("equalizer"); }

AudioFormat EqualizerDsp::outputFormat(const AudioFormat &input) const {
  return input;
}

void EqualizerDsp::prepare(const AudioFormat &format) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  m_sampleRate = format.isValid() ? format.sampleRate() : 44100;
  m_channels = std::clamp(format.isValid() ? format.channelCount() : 2, 1,
                          static_cast<int>(kMaxChannels));
  recalculateCoefficientsLocked();
  reset();
}

void EqualizerDsp::reset() {
  m_resetPending.store(true, std::memory_order_release);
}

void EqualizerDsp::setBandCount(size_t count) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (count != 10 && count != 15 && count != 31) {
    count = 10;
  }
  if (m_bandCount == count) {
    return;
  }
  m_bandCount = count;
  m_bandFrequencies = bandFrequenciesForCount(count);
  m_bandGainsDb.assign(count, 0.0f);
  m_preset = QStringLiteral("Custom");
  recalculateCoefficientsLocked();
}

size_t EqualizerDsp::bandCount() const {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  return m_bandCount;
}

void EqualizerDsp::setBandGain(size_t bandIndex, float gainDb) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (bandIndex >= m_bandGainsDb.size()) {
    return;
  }
  m_bandGainsDb[bandIndex] = std::clamp(gainDb, -15.0f, 15.0f);
  m_preset = QStringLiteral("Custom");
  recalculateCoefficientsLocked();
}

float EqualizerDsp::bandGain(size_t bandIndex) const {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (bandIndex < m_bandGainsDb.size()) {
    return m_bandGainsDb[bandIndex];
  }
  return 0.0f;
}

void EqualizerDsp::setBandGains(const std::vector<float> &gains) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  size_t count = std::min(gains.size(), m_bandGainsDb.size());
  for (size_t i = 0; i < count; ++i) {
    m_bandGainsDb[i] = std::clamp(gains[i], -15.0f, 15.0f);
  }
  m_preset = QStringLiteral("Custom");
  recalculateCoefficientsLocked();
}

std::vector<float> EqualizerDsp::bandGains() const {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  return m_bandGainsDb;
}

void EqualizerDsp::setPreampGain(float gainDb) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  m_preampDb = std::clamp(gainDb, -15.0f, 15.0f);
  m_preampLinear = std::pow(10.0, static_cast<double>(m_preampDb) / 20.0);
  publishConfigLocked();
}

float EqualizerDsp::preampGain() const {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  return m_preampDb;
}

void EqualizerDsp::setBypassed(bool bypass) {
  m_bypassed.store(bypass, std::memory_order_release);
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  publishConfigLocked();
}

bool EqualizerDsp::isBypassed() const {
  return m_bypassed.load(std::memory_order_acquire);
}

void EqualizerDsp::setPreset(const QString &presetName) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  m_preset = presetName;
  m_bandGainsDb = presetGains(presetName, m_bandCount);
  if (presetName == QStringLiteral("Flat")) {
    m_preampDb = 0.0f;
    m_preampLinear = 1.0;
  }
  recalculateCoefficientsLocked();
}

QString EqualizerDsp::preset() const {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  return m_preset;
}

std::vector<QString> EqualizerDsp::availablePresets() {
  std::vector<QString> list;
  list.reserve(std::size(kPresetEntries));
  for (const auto &p : kPresetEntries) {
    list.push_back(QString::fromUtf8(p.name));
  }
  return list;
}

std::vector<float> EqualizerDsp::presetGains(const QString &presetName,
                                             size_t count) {
  const PresetEntry *entry = nullptr;
  for (const auto &p : kPresetEntries) {
    if (presetName.compare(QString::fromUtf8(p.name), Qt::CaseInsensitive) ==
        0) {
      entry = &p;
      break;
    }
  }
  if (!entry) {
    return std::vector<float>(count, 0.0f);
  }
  if (count == 10) {
    return std::vector<float>(entry->gains.begin(), entry->gains.end());
  }
  auto freqs = bandFrequenciesForCount(count);
  std::vector<float> res(count, 0.0f);
  for (size_t i = 0; i < count; ++i) {
    res[i] = interpolateGainFrom10(entry->gains, freqs[i]);
  }
  return res;
}

std::vector<float> EqualizerDsp::bandFrequenciesForCount(size_t count) {
  if (count == 15) {
    return {25.0f,   40.0f,   63.0f,   100.0f,   160.0f,
            250.0f,  400.0f,  630.0f,  1000.0f,  1600.0f,
            2500.0f, 4000.0f, 6300.0f, 10000.0f, 16000.0f};
  }
  if (count == 31) {
    return {20.0f,    25.0f,    31.5f,   40.0f,   50.0f,   63.0f,   80.0f,
            100.0f,   125.0f,   160.0f,  200.0f,  250.0f,  315.0f,  400.0f,
            500.0f,   630.0f,   800.0f,  1000.0f, 1250.0f, 1600.0f, 2000.0f,
            2500.0f,  3150.0f,  4000.0f, 5000.0f, 6300.0f, 8000.0f, 10000.0f,
            12500.0f, 16000.0f, 20000.0f};
  }
  return {31.0f,   63.0f,   125.0f,  250.0f,  500.0f,
          1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f};
}

std::vector<QString> EqualizerDsp::bandFrequencyLabelsForCount(size_t count) {
  auto freqs = bandFrequenciesForCount(count);
  std::vector<QString> labels;
  labels.reserve(freqs.size());
  for (float f : freqs) {
    if (f >= 1000.0f) {
      float k = f / 1000.0f;
      if (std::abs(k - std::round(k)) < 0.05f) {
        labels.push_back(
            QString::asprintf("%d kHz", static_cast<int>(std::round(k))));
      } else {
        labels.push_back(QString::asprintf("%.1f kHz", k));
      }
    } else {
      if (std::abs(f - std::round(f)) < 0.05f) {
        labels.push_back(
            QString::asprintf("%d Hz", static_cast<int>(std::round(f))));
      } else {
        labels.push_back(QString::asprintf("%.1f Hz", f));
      }
    }
  }
  return labels;
}

void EqualizerDsp::recalculateCoefficientsLocked() {
  if (m_sampleRate <= 0) {
    m_sampleRate = 44100;
  }
  double nyquist = static_cast<double>(m_sampleRate) * 0.5;
  double Q = (m_bandCount == 31) ? 4.318 : (m_bandCount == 15 ? 2.145 : 1.414);

  for (size_t i = 0; i < m_bandCount && i < kMaxBands; ++i) {
    double f0 = static_cast<double>(m_bandFrequencies[i]);
    double gainDb = (i < m_bandGainsDb.size())
                        ? static_cast<double>(m_bandGainsDb[i])
                        : 0.0;

    if (f0 >= nyquist * 0.99 || std::abs(gainDb) < 0.05) {
      m_stagingCoeffs[i] = BiquadCoeffs{1.0, 0.0, 0.0, 0.0, 0.0, true};
      continue;
    }

    double A = std::pow(10.0, gainDb / 40.0);
    double w0 =
        2.0 * std::numbers::pi * (f0 / static_cast<double>(m_sampleRate));
    double alpha = std::sin(w0) / (2.0 * Q);
    double cosw0 = std::cos(w0);

    double b0 = 1.0 + alpha * A;
    double b1 = -2.0 * cosw0;
    double b2 = 1.0 - alpha * A;
    double a0 = 1.0 + alpha / A;
    double a1 = -2.0 * cosw0;
    double a2 = 1.0 - alpha / A;

    double invA0 = 1.0 / a0;
    m_stagingCoeffs[i] = BiquadCoeffs{b0 * invA0, b1 * invA0, b2 * invA0,
                                      a1 * invA0, a2 * invA0, false};
  }

  publishConfigLocked();
}

void EqualizerDsp::publishConfigLocked() {
  uint32_t currentIdx = m_activeConfigIdx.load(std::memory_order_relaxed);
  uint32_t nextIdx = 1 - currentIdx;

  auto &cfg = m_configs[nextIdx];
  cfg.bandCount = m_bandCount;
  cfg.channels = m_channels;
  cfg.preampLinear = m_preampLinear;
  cfg.bypassed = m_bypassed.load(std::memory_order_relaxed);
  cfg.coeffs = m_stagingCoeffs;

  m_activeConfigIdx.store(nextIdx, std::memory_order_release);
}

void EqualizerDsp::process(ProcessingBufferList &chunks) {
  if (!isEnabled()) {
    return;
  }

  uint32_t activeIdx = m_activeConfigIdx.load(std::memory_order_acquire);
  const auto &cfg = m_configs[activeIdx];

  if (cfg.bypassed) {
    return;
  }

  if (m_resetPending.exchange(false, std::memory_order_acq_rel)) {
    for (auto &bandState : m_states) {
      for (auto &chState : bandState) {
        chState.s1 = 0.0;
        chState.s2 = 0.0;
      }
    }
  }

  const size_t bandCount = cfg.bandCount;
  const double preamp = cfg.preampLinear;
  const size_t chunkCount = chunks.count();

  for (size_t c = 0; c < chunkCount; ++c) {
    auto *buffer = chunks.item(c);
    if (!buffer || !buffer->isValid()) {
      continue;
    }

    auto span = buffer->data();
    if (span.empty()) {
      continue;
    }

    const int channels = std::clamp(buffer->format().channelCount(), 1,
                                    static_cast<int>(kMaxChannels));
    const int frames = buffer->frameCount();
    const bool applyPreamp = (std::abs(preamp - 1.0) > 0.001);

    for (int frame = 0; frame < frames; ++frame) {
      const size_t baseIdx =
          static_cast<size_t>(frame) * static_cast<size_t>(channels);

      for (int ch = 0; ch < channels; ++ch) {
        double sample = span[baseIdx + static_cast<size_t>(ch)];

        if (applyPreamp) {
          sample *= preamp;
        }

        // Direct Form II Transposed cascaded biquad filters
        for (size_t b = 0; b < bandCount; ++b) {
          const auto &cf = cfg.coeffs[b];
          if (cf.isPassthrough) {
            continue;
          }

          auto &st = m_states[b][static_cast<size_t>(ch)];

          // Anti-denormal guard
          sample += 1e-20;
          sample -= 1e-20;

          double y = cf.b0 * sample + st.s1;
          st.s1 = cf.b1 * sample - cf.a1 * y + st.s2;
          st.s2 = cf.b2 * sample - cf.a2 * y;
          sample = y;
        }

        // Soft-knee limiting for safety
        if (sample > 1.25)
          sample = 1.25;
        else if (sample < -1.25)
          sample = -1.25;

        span[baseIdx + static_cast<size_t>(ch)] = sample;
      }
    }
  }
}

QByteArray EqualizerDsp::saveSettings() const {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  QJsonObject root;
  root[QStringLiteral("bypassed")] = m_bypassed.load(std::memory_order_relaxed);
  root[QStringLiteral("preampDb")] = static_cast<double>(m_preampDb);
  root[QStringLiteral("bandCount")] = static_cast<int>(m_bandCount);
  root[QStringLiteral("preset")] = m_preset;

  QJsonArray bands;
  for (float g : m_bandGainsDb) {
    bands.append(static_cast<double>(g));
  }
  root[QStringLiteral("bands")] = bands;

  return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

bool EqualizerDsp::loadSettings(const QByteArray &preset) {
  QJsonParseError err{};
  QJsonDocument doc = QJsonDocument::fromJson(preset, &err);
  if (err.error != QJsonParseError::NoError || !doc.isObject()) {
    return false;
  }

  QJsonObject root = doc.object();
  std::lock_guard<std::recursive_mutex> lock(m_mutex);

  if (root.contains(QStringLiteral("bypassed"))) {
    m_bypassed.store(root[QStringLiteral("bypassed")].toBool(),
                     std::memory_order_release);
  }
  if (root.contains(QStringLiteral("preampDb"))) {
    m_preampDb =
        static_cast<float>(root[QStringLiteral("preampDb")].toDouble());
    m_preampLinear = std::pow(10.0, static_cast<double>(m_preampDb) / 20.0);
  }
  if (root.contains(QStringLiteral("bandCount"))) {
    size_t count =
        static_cast<size_t>(root[QStringLiteral("bandCount")].toInt());
    if (count == 10 || count == 15 || count == 31) {
      if (m_bandCount != count) {
        m_bandCount = count;
        m_bandFrequencies = bandFrequenciesForCount(count);
        m_bandGainsDb.assign(count, 0.0f);
      }
    }
  }
  if (root.contains(QStringLiteral("preset"))) {
    m_preset = root[QStringLiteral("preset")].toString();
  }
  if (root.contains(QStringLiteral("bands"))) {
    QJsonArray bands = root[QStringLiteral("bands")].toArray();
    size_t count =
        std::min(static_cast<size_t>(bands.size()), m_bandGainsDb.size());
    for (size_t i = 0; i < count; ++i) {
      m_bandGainsDb[i] =
          static_cast<float>(bands[static_cast<int>(i)].toDouble());
    }
  }

  recalculateCoefficientsLocked();
  return true;
}

} // namespace PicAudioEngine
