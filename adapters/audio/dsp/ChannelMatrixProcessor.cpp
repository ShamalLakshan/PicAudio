#include "ChannelMatrixProcessor.hpp"
#include <iomanip>
#include <sstream>

namespace adapters {

ChannelMatrixProcessor::ChannelMatrixProcessor() { resetToIdentity(); }

void ChannelMatrixProcessor::configure(uint8_t inChannels,
                                       uint8_t outChannels) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_inChannels = std::clamp<uint8_t>(inChannels, 1, kMaxChannels);
  m_outChannels = std::clamp<uint8_t>(outChannels, 1, kMaxChannels);

  if (m_inChannels == m_outChannels) {
    resetToIdentity();
  } else if (m_outChannels == 2) {
    applyDownmixToStereo();
  } else if (m_outChannels == 1) {
    applyDownmixToMono();
  } else if (m_outChannels == 6) {
    applyUpmixTo51();
  } else if (m_outChannels == 8) {
    applyUpmixTo71();
  } else {
    resetToIdentity();
  }
}

void ChannelMatrixProcessor::setInputChannels(uint8_t channels) {
  configure(channels, m_outChannels);
}

uint8_t ChannelMatrixProcessor::getInputChannels() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_inChannels;
}

void ChannelMatrixProcessor::setOutputChannels(uint8_t channels) {
  configure(m_inChannels, channels);
}

uint8_t ChannelMatrixProcessor::getOutputChannels() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_outChannels;
}

void ChannelMatrixProcessor::setCoefficient(size_t outChannel, size_t inChannel,
                                            float gain) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (outChannel < kMaxChannels && inChannel < kMaxChannels) {
    m_matrix[outChannel][inChannel] = std::clamp(gain, 0.0f, 4.0f);
    m_preset = MatrixPreset::Custom;
  }
}

float ChannelMatrixProcessor::getCoefficient(size_t outChannel,
                                             size_t inChannel) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (outChannel < kMaxChannels && inChannel < kMaxChannels) {
    return m_matrix[outChannel][inChannel];
  }
  return 0.0f;
}

void ChannelMatrixProcessor::loadPreset(MatrixPreset preset) {
  std::lock_guard<std::mutex> lock(m_mutex);
  switch (preset) {
  case MatrixPreset::Passthrough:
    resetToIdentity();
    break;
  case MatrixPreset::DownmixToStereo:
    applyDownmixToStereo();
    break;
  case MatrixPreset::DownmixToMono:
    applyDownmixToMono();
    break;
  case MatrixPreset::UpmixTo51:
    applyUpmixTo51();
    break;
  case MatrixPreset::UpmixTo71:
    applyUpmixTo71();
    break;
  case MatrixPreset::Custom:
    m_preset = MatrixPreset::Custom;
    break;
  }
}

MatrixPreset ChannelMatrixProcessor::getActivePreset() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_preset;
}

void ChannelMatrixProcessor::resetToIdentity() {
  for (size_t o = 0; o < kMaxChannels; ++o) {
    for (size_t i = 0; i < kMaxChannels; ++i) {
      m_matrix[o][i] = (o == i) ? 1.0f : 0.0f;
    }
  }
  m_preset = MatrixPreset::Passthrough;
}

void ChannelMatrixProcessor::applyDownmixToStereo() {
  m_outChannels = 2;
  for (size_t o = 0; o < kMaxChannels; ++o) {
    for (size_t i = 0; i < kMaxChannels; ++i) {
      m_matrix[o][i] = 0.0f;
    }
  }

  constexpr float kInvSqrt2 = 0.70710678f; // -3 dB

  if (m_inChannels == 1) {
    // Mono to Stereo
    m_matrix[0][0] = 1.0f;
    m_matrix[1][0] = 1.0f;
  } else if (m_inChannels == 2) {
    m_matrix[0][0] = 1.0f;
    m_matrix[1][1] = 1.0f;
  } else if (m_inChannels == 4) {
    // Quad (FL, FR, RL, RR) to Stereo
    m_matrix[0][0] = 1.0f;
    m_matrix[0][2] = kInvSqrt2; // RL
    m_matrix[1][1] = 1.0f;
    m_matrix[1][3] = kInvSqrt2; // RR
  } else if (m_inChannels == 6) {
    // 5.1 (FL, FR, FC, LFE, RL, RR) to Stereo (ITU-R BS.775)
    m_matrix[0][0] = 1.0f;      // FL -> L
    m_matrix[0][2] = kInvSqrt2; // FC -> L
    m_matrix[0][4] = kInvSqrt2; // RL -> L

    m_matrix[1][1] = 1.0f;      // FR -> R
    m_matrix[1][2] = kInvSqrt2; // FC -> R
    m_matrix[1][5] = kInvSqrt2; // RR -> R
  } else if (m_inChannels >= 8) {
    // 7.1 (FL, FR, FC, LFE, RL, RR, SL, SR) to Stereo
    m_matrix[0][0] = 1.0f;      // FL -> L
    m_matrix[0][2] = kInvSqrt2; // FC -> L
    m_matrix[0][4] = kInvSqrt2; // RL -> L
    m_matrix[0][6] = kInvSqrt2; // SL -> L

    m_matrix[1][1] = 1.0f;      // FR -> R
    m_matrix[1][2] = kInvSqrt2; // FC -> R
    m_matrix[1][5] = kInvSqrt2; // RR -> R
    m_matrix[1][7] = kInvSqrt2; // SR -> R
  } else {
    m_matrix[0][0] = 1.0f;
    m_matrix[1][1] = 1.0f;
  }

  normalize();
  m_preset = MatrixPreset::DownmixToStereo;
}

void ChannelMatrixProcessor::applyDownmixToMono() {
  m_outChannels = 1;
  for (size_t o = 0; o < kMaxChannels; ++o) {
    for (size_t i = 0; i < kMaxChannels; ++i) {
      m_matrix[o][i] = 0.0f;
    }
  }

  const float weight =
      1.0f / static_cast<float>(std::max<uint8_t>(1, m_inChannels));
  for (size_t i = 0; i < m_inChannels; ++i) {
    m_matrix[0][i] = weight;
  }
  m_preset = MatrixPreset::DownmixToMono;
}

void ChannelMatrixProcessor::applyUpmixTo51() {
  m_outChannels = 6;
  for (size_t o = 0; o < kMaxChannels; ++o) {
    for (size_t i = 0; i < kMaxChannels; ++i) {
      m_matrix[o][i] = 0.0f;
    }
  }

  constexpr float kInvSqrt2 = 0.70710678f;

  if (m_inChannels == 1) {
    // Mono to 5.1
    m_matrix[0][0] = 0.707f; // FL
    m_matrix[1][0] = 0.707f; // FR
    m_matrix[2][0] = 1.000f; // FC
    m_matrix[3][0] = 0.500f; // LFE
    m_matrix[4][0] = 0.500f; // RL
    m_matrix[5][0] = 0.500f; // RR
  } else {
    // Stereo (FL=0, FR=1) to 5.1 (FL=0, FR=1, FC=2, LFE=3, RL=4, RR=5)
    m_matrix[0][0] = 1.0f; // FL
    m_matrix[1][1] = 1.0f; // FR

    // Center = (FL + FR) / sqrt(2)
    m_matrix[2][0] = kInvSqrt2 * 0.7f;
    m_matrix[2][1] = kInvSqrt2 * 0.7f;

    // Subwoofer LFE mono sum
    m_matrix[3][0] = 0.5f;
    m_matrix[3][1] = 0.5f;

    // Surrounds: out-of-phase ambient extraction
    m_matrix[4][0] = kInvSqrt2 * 0.7f;
    m_matrix[4][1] = -kInvSqrt2 * 0.4f;

    m_matrix[5][0] = -kInvSqrt2 * 0.4f;
    m_matrix[5][1] = kInvSqrt2 * 0.7f;
  }

  m_preset = MatrixPreset::UpmixTo51;
}

void ChannelMatrixProcessor::applyUpmixTo71() {
  m_outChannels = 8;
  for (size_t o = 0; o < kMaxChannels; ++o) {
    for (size_t i = 0; i < kMaxChannels; ++i) {
      m_matrix[o][i] = 0.0f;
    }
  }

  constexpr float kInvSqrt2 = 0.70710678f;

  if (m_inChannels == 1) {
    // Mono to 7.1
    m_matrix[0][0] = 0.707f; // FL
    m_matrix[1][0] = 0.707f; // FR
    m_matrix[2][0] = 1.000f; // FC
    m_matrix[3][0] = 0.500f; // LFE
    m_matrix[4][0] = 0.500f; // RL
    m_matrix[5][0] = 0.500f; // RR
    m_matrix[6][0] = 0.600f; // SL
    m_matrix[7][0] = 0.600f; // SR
  } else {
    // Stereo to 7.1: FL=0, FR=1, FC=2, LFE=3, RL=4, RR=5, SL=6, SR=7
    m_matrix[0][0] = 1.0f; // FL
    m_matrix[1][1] = 1.0f; // FR

    // Center
    m_matrix[2][0] = kInvSqrt2 * 0.7f;
    m_matrix[2][1] = kInvSqrt2 * 0.7f;

    // LFE
    m_matrix[3][0] = 0.5f;
    m_matrix[3][1] = 0.5f;

    // Rear Surrounds
    m_matrix[4][0] = 0.5f;
    m_matrix[4][1] = -0.3f;

    m_matrix[5][0] = -0.3f;
    m_matrix[5][1] = 0.5f;

    // Side Surrounds
    m_matrix[6][0] = 0.65f;
    m_matrix[6][1] = -0.2f;

    m_matrix[7][0] = -0.2f;
    m_matrix[7][1] = 0.65f;
  }

  m_preset = MatrixPreset::UpmixTo71;
}

void ChannelMatrixProcessor::normalize() {
  for (size_t o = 0; o < m_outChannels; ++o) {
    float rowSum = 0.0f;
    for (size_t i = 0; i < m_inChannels; ++i) {
      rowSum += std::abs(m_matrix[o][i]);
    }
    if (rowSum > 1.0f) {
      float scale = 1.0f / rowSum;
      for (size_t i = 0; i < m_inChannels; ++i) {
        m_matrix[o][i] *= scale;
      }
    }
  }
}

void ChannelMatrixProcessor::process(const float *input, float *output,
                                     size_t frames) {
  if (!input || !output || frames == 0) {
    return;
  }

  std::lock_guard<std::mutex> lock(m_mutex);
  const size_t inCh = m_inChannels;
  const size_t outCh = m_outChannels;

  std::array<float, kMaxChannels> inSumSq{};
  std::array<float, kMaxChannels> outSumSq{};

  for (size_t f = 0; f < frames; ++f) {
    const float *inFrame = input + f * inCh;
    float *outFrame = output + f * outCh;

    // Input metering
    for (size_t i = 0; i < inCh; ++i) {
      inSumSq[i] += inFrame[i] * inFrame[i];
    }

    // Matrix multiply
    for (size_t o = 0; o < outCh; ++o) {
      float sum = 0.0f;
      for (size_t i = 0; i < inCh; ++i) {
        sum += m_matrix[o][i] * inFrame[i];
      }

      // Soft-knee limiting for protection against extreme gain spikes
      if (sum > 1.0f) {
        sum = 1.0f - std::exp(-sum);
      } else if (sum < -1.0f) {
        sum = -1.0f + std::exp(sum);
      }

      outFrame[o] = sum;
      outSumSq[o] += sum * sum;
    }
  }

  // Update live meter RMS values with ballistics smoothing
  {
    std::lock_guard<std::mutex> mLock(m_meterMutex);
    const float invFrames = 1.0f / static_cast<float>(frames);
    for (size_t i = 0; i < kMaxChannels; ++i) {
      if (i < inCh) {
        float rms = std::sqrt(inSumSq[i] * invFrames);
        m_inputLevels[i] = std::max(rms, m_inputLevels[i] * 0.85f);
      } else {
        m_inputLevels[i] *= 0.85f;
      }

      if (i < outCh) {
        float rms = std::sqrt(outSumSq[i] * invFrames);
        m_outputLevels[i] = std::max(rms, m_outputLevels[i] * 0.85f);
      } else {
        m_outputLevels[i] *= 0.85f;
      }
    }
  }
}

std::vector<float> ChannelMatrixProcessor::getInputLevels() const {
  std::lock_guard<std::mutex> lock(m_meterMutex);
  return std::vector<float>(m_inputLevels.begin(), m_inputLevels.end());
}

std::vector<float> ChannelMatrixProcessor::getOutputLevels() const {
  std::lock_guard<std::mutex> lock(m_meterMutex);
  return std::vector<float>(m_outputLevels.begin(), m_outputLevels.end());
}

std::string ChannelMatrixProcessor::serialize() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  std::ostringstream ss;
  ss << "{\"inChannels\":" << static_cast<int>(m_inChannels)
     << ",\"outChannels\":" << static_cast<int>(m_outChannels)
     << ",\"preset\":" << static_cast<int>(m_preset) << ",\"matrix\":[";

  for (size_t o = 0; o < kMaxChannels; ++o) {
    ss << "[";
    for (size_t i = 0; i < kMaxChannels; ++i) {
      ss << std::fixed << std::setprecision(4) << m_matrix[o][i];
      if (i + 1 < kMaxChannels)
        ss << ",";
    }
    ss << "]";
    if (o + 1 < kMaxChannels)
      ss << ",";
  }
  ss << "]}";
  return ss.str();
}

bool ChannelMatrixProcessor::deserialize(const std::string &json) {
  if (json.empty() || json.find("\"matrix\"") == std::string::npos) {
    return false;
  }

  // Token parser for serialized JSON matrix and channel configuration
  std::lock_guard<std::mutex> lock(m_mutex);

  auto parseField = [&](const std::string &key) -> int {
    size_t pos = json.find("\"" + key + "\":");
    if (pos == std::string::npos) {
      pos = json.find("\"" + key + "\" :");
    }
    if (pos == std::string::npos)
      return -1;
    size_t colon = json.find(':', pos);
    if (colon == std::string::npos)
      return -1;
    size_t start = colon + 1;
    while (start < json.size() && (json[start] == ' ' || json[start] == '\t'))
      ++start;
    size_t end = start;
    while (end < json.size() &&
           ((json[end] >= '0' && json[end] <= '9') || json[end] == '-'))
      ++end;
    if (end > start) {
      try {
        return std::stoi(json.substr(start, end - start));
      } catch (...) {
      }
    }
    return -1;
  };

  int inCh = parseField("inChannels");
  if (inCh >= 1 && inCh <= 8) {
    m_inChannels = static_cast<uint8_t>(inCh);
  }
  int outCh = parseField("outChannels");
  if (outCh >= 1 && outCh <= 8) {
    m_outChannels = static_cast<uint8_t>(outCh);
  }

  size_t matPos = json.find("\"matrix\":[");
  if (matPos == std::string::npos)
    return false;

  size_t cur = matPos + 10;
  for (size_t o = 0; o < kMaxChannels; ++o) {
    size_t openBracket = json.find('[', cur);
    if (openBracket == std::string::npos)
      break;
    size_t closeBracket = json.find(']', openBracket);
    if (closeBracket == std::string::npos)
      break;

    std::string rowStr =
        json.substr(openBracket + 1, closeBracket - openBracket - 1);
    std::stringstream rowStream(rowStr);
    std::string valStr;
    size_t i = 0;
    while (std::getline(rowStream, valStr, ',') && i < kMaxChannels) {
      try {
        m_matrix[o][i] = std::stof(valStr);
      } catch (...) {
        m_matrix[o][i] = 0.0f;
      }
      ++i;
    }
    cur = closeBracket + 1;
  }

  m_preset = MatrixPreset::Custom;
  return true;
}

} // namespace adapters
