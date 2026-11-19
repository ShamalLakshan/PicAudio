#include "channelmatrixdsp.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>
#include <cmath>

namespace PicAudioEngine {

constexpr double kInvSqrt2 = 0.7071067811865475; // -3 dB

ChannelMatrixDsp::ChannelMatrixDsp() {
  for (size_t o = 0; o < kMaxChannels; ++o) {
    for (size_t i = 0; i < kMaxChannels; ++i) {
      m_stagingMatrix[o][i] = (o == i) ? 1.0 : 0.0;
    }
  }
  for (size_t i = 0; i < kMaxChannels; ++i) {
    m_inputLevels[i].store(0.0f, std::memory_order_relaxed);
    m_outputLevels[i].store(0.0f, std::memory_order_relaxed);
  }
  publishConfigLocked();
}

QString ChannelMatrixDsp::name() const {
  return QStringLiteral("Channel Matrix");
}

QString ChannelMatrixDsp::id() const {
  return QStringLiteral("channel_matrix");
}

AudioFormat ChannelMatrixDsp::outputFormat(const AudioFormat &input) const {
  if (!input.isValid()) {
    return {};
  }

  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (m_preset == Preset::Passthrough ||
      m_bypassed.load(std::memory_order_relaxed)) {
    return input;
  }

  AudioFormat out{input};
  out.setChannelCount(m_outChannels);
  out.setChannelLayout(
      AudioFormat::defaultChannelLayoutForChannelCount(m_outChannels));
  return out;
}

void ChannelMatrixDsp::prepare(const AudioFormat &format) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  m_inputFormat = format;
  m_inChannels = std::clamp(format.isValid() ? format.channelCount() : 2, 1,
                            static_cast<int>(kMaxChannels));

  m_outputFormat = format;
  if (m_preset == Preset::Passthrough) {
    m_outChannels = m_inChannels;
  }
  m_outputFormat.setChannelCount(m_outChannels);
  m_outputFormat.setChannelLayout(
      AudioFormat::defaultChannelLayoutForChannelCount(m_outChannels));

  publishConfigLocked();
}

void ChannelMatrixDsp::reset() {
  for (size_t i = 0; i < kMaxChannels; ++i) {
    m_inputLevels[i].store(0.0f, std::memory_order_relaxed);
    m_outputLevels[i].store(0.0f, std::memory_order_relaxed);
  }
}

void ChannelMatrixDsp::setCoefficient(size_t outCh, size_t inCh, float gain) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (outCh < kMaxChannels && inCh < kMaxChannels) {
    m_stagingMatrix[outCh][inCh] =
        std::clamp(static_cast<double>(gain), 0.0, 4.0);
    m_preset = Preset::Custom;
    publishConfigLocked();
  }
}

float ChannelMatrixDsp::coefficient(size_t outCh, size_t inCh) const {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (outCh < kMaxChannels && inCh < kMaxChannels) {
    return static_cast<float>(m_stagingMatrix[outCh][inCh]);
  }
  return (outCh == inCh) ? 1.0f : 0.0f;
}

void ChannelMatrixDsp::setPreset(Preset preset) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  m_preset = preset;
  switch (preset) {
  case Preset::Passthrough:
    resetToIdentityLocked();
    break;
  case Preset::DownmixToStereo:
    applyDownmixToStereoLocked();
    break;
  case Preset::DownmixToMono:
    applyDownmixToMonoLocked();
    break;
  case Preset::UpmixTo51:
    applyUpmixTo51Locked();
    break;
  case Preset::UpmixTo71:
    applyUpmixTo71Locked();
    break;
  case Preset::Custom:
    break;
  }
  publishConfigLocked();
}

ChannelMatrixDsp::Preset ChannelMatrixDsp::activePreset() const {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  return m_preset;
}

void ChannelMatrixDsp::setOutputChannels(int channels) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  m_outChannels = std::clamp(channels, 1, static_cast<int>(kMaxChannels));
  m_outputFormat.setChannelCount(m_outChannels);
  m_outputFormat.setChannelLayout(
      AudioFormat::defaultChannelLayoutForChannelCount(m_outChannels));
  publishConfigLocked();
}

int ChannelMatrixDsp::outputChannels() const {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  return m_outChannels;
}

void ChannelMatrixDsp::normalizeLocked() {
  for (size_t o = 0; o < static_cast<size_t>(m_outChannels); ++o) {
    double rowSum = 0.0;
    for (size_t i = 0; i < static_cast<size_t>(m_inChannels); ++i) {
      rowSum += std::abs(m_stagingMatrix[o][i]);
    }
    if (rowSum > 1.0) {
      double scale = 1.0 / rowSum;
      for (size_t i = 0; i < static_cast<size_t>(m_inChannels); ++i) {
        m_stagingMatrix[o][i] *= scale;
      }
    }
  }
}

void ChannelMatrixDsp::normalize() {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  normalizeLocked();
  publishConfigLocked();
}

void ChannelMatrixDsp::resetToIdentityLocked() {
  for (size_t o = 0; o < kMaxChannels; ++o) {
    for (size_t i = 0; i < kMaxChannels; ++i) {
      m_stagingMatrix[o][i] = (o == i) ? 1.0 : 0.0;
    }
  }
  m_preset = Preset::Passthrough;
  m_outChannels = m_inChannels;
}

void ChannelMatrixDsp::resetToIdentity() {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  resetToIdentityLocked();
  publishConfigLocked();
}

void ChannelMatrixDsp::setBypassed(bool bypass) {
  m_bypassed.store(bypass, std::memory_order_release);
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  publishConfigLocked();
}

bool ChannelMatrixDsp::isBypassed() const {
  return m_bypassed.load(std::memory_order_acquire);
}

std::vector<float> ChannelMatrixDsp::inputLevels() const {
  std::vector<float> levels(kMaxChannels, 0.0f);
  for (size_t i = 0; i < kMaxChannels; ++i) {
    levels[i] = m_inputLevels[i].load(std::memory_order_relaxed);
  }
  return levels;
}

std::vector<float> ChannelMatrixDsp::outputLevels() const {
  std::vector<float> levels(kMaxChannels, 0.0f);
  for (size_t i = 0; i < kMaxChannels; ++i) {
    levels[i] = m_outputLevels[i].load(std::memory_order_relaxed);
  }
  return levels;
}

void ChannelMatrixDsp::applyDownmixToStereoLocked() {
  m_outChannels = 2;
  for (size_t o = 0; o < kMaxChannels; ++o) {
    for (size_t i = 0; i < kMaxChannels; ++i) {
      m_stagingMatrix[o][i] = 0.0;
    }
  }

  if (m_inChannels == 1) {
    m_stagingMatrix[0][0] = 1.0;
    m_stagingMatrix[1][0] = 1.0;
  } else if (m_inChannels == 2) {
    m_stagingMatrix[0][0] = 1.0;
    m_stagingMatrix[1][1] = 1.0;
  } else if (m_inChannels == 4) {
    m_stagingMatrix[0][0] = 1.0;
    m_stagingMatrix[0][2] = kInvSqrt2;
    m_stagingMatrix[1][1] = 1.0;
    m_stagingMatrix[1][3] = kInvSqrt2;
  } else if (m_inChannels == 6) {
    // 5.1 (FL, FR, FC, LFE, RL, RR) to Stereo
    m_stagingMatrix[0][0] = 1.0;
    m_stagingMatrix[0][2] = kInvSqrt2;
    m_stagingMatrix[0][4] = kInvSqrt2;

    m_stagingMatrix[1][1] = 1.0;
    m_stagingMatrix[1][2] = kInvSqrt2;
    m_stagingMatrix[1][5] = kInvSqrt2;
  } else if (m_inChannels >= 8) {
    // 7.1 to Stereo
    m_stagingMatrix[0][0] = 1.0;
    m_stagingMatrix[0][2] = kInvSqrt2;
    m_stagingMatrix[0][4] = kInvSqrt2;
    m_stagingMatrix[0][6] = kInvSqrt2;

    m_stagingMatrix[1][1] = 1.0;
    m_stagingMatrix[1][2] = kInvSqrt2;
    m_stagingMatrix[1][5] = kInvSqrt2;
    m_stagingMatrix[1][7] = kInvSqrt2;
  } else {
    m_stagingMatrix[0][0] = 1.0;
    m_stagingMatrix[1][1] = 1.0;
  }

  normalizeLocked();
}

void ChannelMatrixDsp::applyDownmixToMonoLocked() {
  m_outChannels = 1;
  for (size_t o = 0; o < kMaxChannels; ++o) {
    for (size_t i = 0; i < kMaxChannels; ++i) {
      m_stagingMatrix[o][i] = 0.0;
    }
  }

  const double weight = 1.0 / static_cast<double>(std::max(1, m_inChannels));
  for (size_t i = 0; i < static_cast<size_t>(m_inChannels); ++i) {
    m_stagingMatrix[0][i] = weight;
  }
}

void ChannelMatrixDsp::applyUpmixTo51Locked() {
  m_outChannels = 6;
  for (size_t o = 0; o < kMaxChannels; ++o) {
    for (size_t i = 0; i < kMaxChannels; ++i) {
      m_stagingMatrix[o][i] = 0.0;
    }
  }

  if (m_inChannels == 1) {
    m_stagingMatrix[0][0] = 0.707;
    m_stagingMatrix[1][0] = 0.707;
    m_stagingMatrix[2][0] = 1.000;
    m_stagingMatrix[3][0] = 0.500;
    m_stagingMatrix[4][0] = 0.500;
    m_stagingMatrix[5][0] = 0.500;
  } else {
    // Stereo to 5.1
    m_stagingMatrix[0][0] = 1.0;
    m_stagingMatrix[1][1] = 1.0;
    m_stagingMatrix[2][0] = kInvSqrt2 * 0.7;
    m_stagingMatrix[2][1] = kInvSqrt2 * 0.7;
    m_stagingMatrix[3][0] = 0.5;
    m_stagingMatrix[3][1] = 0.5;
    m_stagingMatrix[4][0] = kInvSqrt2 * 0.7;
    m_stagingMatrix[4][1] = -kInvSqrt2 * 0.4;
    m_stagingMatrix[5][0] = -kInvSqrt2 * 0.4;
    m_stagingMatrix[5][1] = kInvSqrt2 * 0.7;
  }
}

void ChannelMatrixDsp::applyUpmixTo71Locked() {
  m_outChannels = 8;
  for (size_t o = 0; o < kMaxChannels; ++o) {
    for (size_t i = 0; i < kMaxChannels; ++i) {
      m_stagingMatrix[o][i] = 0.0;
    }
  }

  if (m_inChannels == 1) {
    m_stagingMatrix[0][0] = 0.707;
    m_stagingMatrix[1][0] = 0.707;
    m_stagingMatrix[2][0] = 1.000;
    m_stagingMatrix[3][0] = 0.500;
    m_stagingMatrix[4][0] = 0.500;
    m_stagingMatrix[5][0] = 0.500;
    m_stagingMatrix[6][0] = 0.600;
    m_stagingMatrix[7][0] = 0.600;
  } else {
    // Stereo to 7.1
    m_stagingMatrix[0][0] = 1.0;
    m_stagingMatrix[1][1] = 1.0;
    m_stagingMatrix[2][0] = kInvSqrt2 * 0.7;
    m_stagingMatrix[2][1] = kInvSqrt2 * 0.7;
    m_stagingMatrix[3][0] = 0.5;
    m_stagingMatrix[3][1] = 0.5;
    m_stagingMatrix[4][0] = 0.5;
    m_stagingMatrix[4][1] = -0.3;
    m_stagingMatrix[5][0] = -0.3;
    m_stagingMatrix[5][1] = 0.5;
    m_stagingMatrix[6][0] = 0.65;
    m_stagingMatrix[6][1] = -0.2;
    m_stagingMatrix[7][0] = -0.2;
    m_stagingMatrix[7][1] = 0.65;
  }
}

void ChannelMatrixDsp::publishConfigLocked() {
  uint32_t currentIdx = m_activeConfigIdx.load(std::memory_order_relaxed);
  uint32_t nextIdx = 1 - currentIdx;

  auto &cfg = m_configs[nextIdx];
  cfg.matrix = m_stagingMatrix;
  cfg.inChannels = m_inChannels;
  cfg.outChannels = m_outChannels;
  cfg.preset = m_preset;
  cfg.bypassed = m_bypassed.load(std::memory_order_relaxed);

  bool isIdentity = (m_inChannels == m_outChannels);
  if (isIdentity) {
    for (size_t o = 0; o < static_cast<size_t>(m_outChannels); ++o) {
      for (size_t i = 0; i < static_cast<size_t>(m_inChannels); ++i) {
        double expected = (o == i) ? 1.0 : 0.0;
        if (std::abs(m_stagingMatrix[o][i] - expected) > 0.0001) {
          isIdentity = false;
          break;
        }
      }
      if (!isIdentity)
        break;
    }
  }
  cfg.isPassthrough = (m_preset == Preset::Passthrough) || isIdentity;

  m_activeConfigIdx.store(nextIdx, std::memory_order_release);
}

void ChannelMatrixDsp::process(ProcessingBufferList &chunks) {
  if (!isEnabled()) {
    return;
  }

  uint32_t activeIdx = m_activeConfigIdx.load(std::memory_order_acquire);
  const auto &cfg = m_configs[activeIdx];

  if (cfg.bypassed) {
    return;
  }

  const size_t chunkCount = chunks.count();
  if (chunkCount == 0) {
    return;
  }

  const int inChannels = cfg.inChannels;
  const int outChannels = cfg.outChannels;

  std::array<double, kMaxChannels> inRmsSumSq{};
  std::array<double, kMaxChannels> outRmsSumSq{};
  size_t totalFrames = 0;

  if (inChannels == outChannels) {
    // In-place processing path
    for (size_t c = 0; c < chunkCount; ++c) {
      auto *buffer = chunks.item(c);
      if (!buffer || !buffer->isValid()) {
        continue;
      }

      auto span = buffer->data();
      if (span.empty()) {
        continue;
      }

      const int frames = buffer->frameCount();
      totalFrames += static_cast<size_t>(frames);

      std::array<double, kMaxChannels> inTemp{};

      for (int f = 0; f < frames; ++f) {
        const size_t baseIdx =
            static_cast<size_t>(f) * static_cast<size_t>(inChannels);

        for (int i = 0; i < inChannels; ++i) {
          double s = span[baseIdx + static_cast<size_t>(i)];
          inTemp[i] = s;
          inRmsSumSq[i] += s * s;
        }

        for (int o = 0; o < outChannels; ++o) {
          double sum = 0.0;
          for (int i = 0; i < inChannels; ++i) {
            sum += cfg.matrix[o][i] * inTemp[i];
          }

          // Anti-clipping soft knee
          if (sum > 1.25)
            sum = 1.25;
          else if (sum < -1.25)
            sum = -1.25;

          span[baseIdx + static_cast<size_t>(o)] = sum;
          outRmsSumSq[o] += sum * sum;
        }
      }
    }
  } else {
    // Channel-count adapting path
    ProcessingBufferList output;

    for (size_t c = 0; c < chunkCount; ++c) {
      const auto *buffer = chunks.item(c);
      if (!buffer || !buffer->isValid()) {
        continue;
      }

      const int frames = buffer->frameCount();
      if (frames <= 0) {
        output.addChunk(*buffer);
        continue;
      }

      const auto inSamples = buffer->constData();
      if (inSamples.empty()) {
        output.addChunk(*buffer);
        continue;
      }

      totalFrames += static_cast<size_t>(frames);

      const auto outSamples =
          static_cast<size_t>(frames) * static_cast<size_t>(outChannels);
      auto *outBuffer =
          output.addItem(m_outputFormat, buffer->startTimeNs(), outSamples);
      if (!outBuffer) {
        output.addChunk(*buffer);
        continue;
      }

      outBuffer->setSourceFrameDurationNs(buffer->sourceFrameDurationNs());
      auto outSpan = outBuffer->data();
      if (outSpan.size() < outSamples) {
        output.addChunk(*buffer);
        continue;
      }

      for (int f = 0; f < frames; ++f) {
        const size_t inBase =
            static_cast<size_t>(f) * static_cast<size_t>(inChannels);
        const size_t outBase =
            static_cast<size_t>(f) * static_cast<size_t>(outChannels);

        for (int i = 0; i < inChannels; ++i) {
          double s = inSamples[inBase + static_cast<size_t>(i)];
          inRmsSumSq[i] += s * s;
        }

        for (int o = 0; o < outChannels; ++o) {
          double sum = 0.0;
          for (int i = 0; i < inChannels; ++i) {
            sum +=
                cfg.matrix[o][i] * inSamples[inBase + static_cast<size_t>(i)];
          }

          if (sum > 1.25)
            sum = 1.25;
          else if (sum < -1.25)
            sum = -1.25;

          outSpan[outBase + static_cast<size_t>(o)] = sum;
          outRmsSumSq[o] += sum * sum;
        }
      }
    }

    chunks.clear();
    const size_t outCount = output.count();
    for (size_t i = 0; i < outCount; ++i) {
      if (const auto *b = output.item(i); b && b->isValid()) {
        chunks.addChunk(*b);
      }
    }
  }

  // Update lock-free atomic RMS levels with ballistics
  if (totalFrames > 0) {
    const double invFrames = 1.0 / static_cast<double>(totalFrames);
    for (size_t i = 0; i < kMaxChannels; ++i) {
      if (i < static_cast<size_t>(inChannels)) {
        float rms = static_cast<float>(std::sqrt(inRmsSumSq[i] * invFrames));
        float prev = m_inputLevels[i].load(std::memory_order_relaxed);
        m_inputLevels[i].store(std::max(rms, prev * 0.85f),
                               std::memory_order_relaxed);
      } else {
        float prev = m_inputLevels[i].load(std::memory_order_relaxed);
        m_inputLevels[i].store(prev * 0.85f, std::memory_order_relaxed);
      }

      if (i < static_cast<size_t>(outChannels)) {
        float rms = static_cast<float>(std::sqrt(outRmsSumSq[i] * invFrames));
        float prev = m_outputLevels[i].load(std::memory_order_relaxed);
        m_outputLevels[i].store(std::max(rms, prev * 0.85f),
                                std::memory_order_relaxed);
      } else {
        float prev = m_outputLevels[i].load(std::memory_order_relaxed);
        m_outputLevels[i].store(prev * 0.85f, std::memory_order_relaxed);
      }
    }
  }
}

QByteArray ChannelMatrixDsp::saveSettings() const {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  QJsonObject root;
  root[QStringLiteral("bypassed")] = m_bypassed.load(std::memory_order_relaxed);
  root[QStringLiteral("preset")] = static_cast<int>(m_preset);
  root[QStringLiteral("inChannels")] = m_inChannels;
  root[QStringLiteral("outChannels")] = m_outChannels;

  QJsonArray rows;
  for (size_t o = 0; o < kMaxChannels; ++o) {
    QJsonArray row;
    for (size_t i = 0; i < kMaxChannels; ++i) {
      row.append(m_stagingMatrix[o][i]);
    }
    rows.append(row);
  }
  root[QStringLiteral("matrix")] = rows;

  return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

bool ChannelMatrixDsp::loadSettings(const QByteArray &preset) {
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
  if (root.contains(QStringLiteral("preset"))) {
    m_preset = static_cast<Preset>(root[QStringLiteral("preset")].toInt());
  }
  if (root.contains(QStringLiteral("inChannels"))) {
    m_inChannels = std::clamp(root[QStringLiteral("inChannels")].toInt(), 1,
                              static_cast<int>(kMaxChannels));
  }
  if (root.contains(QStringLiteral("outChannels"))) {
    m_outChannels = std::clamp(root[QStringLiteral("outChannels")].toInt(), 1,
                               static_cast<int>(kMaxChannels));
  }
  if (root.contains(QStringLiteral("matrix"))) {
    QJsonArray rows = root[QStringLiteral("matrix")].toArray();
    for (size_t o = 0; o < kMaxChannels && o < static_cast<size_t>(rows.size());
         ++o) {
      QJsonArray row = rows[static_cast<int>(o)].toArray();
      for (size_t i = 0;
           i < kMaxChannels && i < static_cast<size_t>(row.size()); ++i) {
        m_stagingMatrix[o][i] = row[static_cast<int>(i)].toDouble();
      }
    }
  }

  publishConfigLocked();
  return true;
}

} // namespace PicAudioEngine
