#include "LoudnessAndDitherProcessor.hpp"
#include <algorithm>
#include <cmath>

namespace adapters {

LoudnessAndDitherProcessor::LoudnessAndDitherProcessor() {
  recalculateGainLocked();
}

void LoudnessAndDitherProcessor::setReplayGainMode(ReplayGainMode mode) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_mode = mode;
  recalculateGainLocked();
}

ReplayGainMode LoudnessAndDitherProcessor::getReplayGainMode() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_mode;
}

void LoudnessAndDitherProcessor::setPreampTaggedDb(float db) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_preampTaggedDb = std::clamp(db, -12.0f, 12.0f);
  recalculateGainLocked();
}

float LoudnessAndDitherProcessor::getPreampTaggedDb() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_preampTaggedDb;
}

void LoudnessAndDitherProcessor::setPreampUntaggedDb(float db) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_preampUntaggedDb = std::clamp(db, -12.0f, 12.0f);
  recalculateGainLocked();
}

float LoudnessAndDitherProcessor::getPreampUntaggedDb() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_preampUntaggedDb;
}

void LoudnessAndDitherProcessor::setTruePeakLimiterEnabled(bool enabled) {
  m_limiterEnabled.store(enabled, std::memory_order_release);
}

bool LoudnessAndDitherProcessor::isTruePeakLimiterEnabled() const {
  return m_limiterEnabled.load(std::memory_order_relaxed);
}

void LoudnessAndDitherProcessor::setCurrentTrackLoudness(
    bool hasReplayGain, float trackGainDb, float trackPeak, float albumGainDb,
    float albumPeak, bool isAlbumPlayback) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_hasReplayGain = hasReplayGain;
  m_trackGainDb = trackGainDb;
  m_trackPeak = (trackPeak > 0.0f) ? trackPeak : 1.0f;
  m_albumGainDb = albumGainDb;
  m_albumPeak = (albumPeak > 0.0f) ? albumPeak : 1.0f;
  m_isAlbumPlayback = isAlbumPlayback;
  recalculateGainLocked();
}

void LoudnessAndDitherProcessor::recalculateGainLocked() {
  if (m_mode == ReplayGainMode::Disabled) {
    m_appliedGainDb.store(0.0f, std::memory_order_release);
    m_targetLinearGain.store(1.0f, std::memory_order_release);
    return;
  }

  float effectiveGainDb = 0.0f;
  if (m_hasReplayGain) {
    if (m_mode == ReplayGainMode::TrackGain) {
      effectiveGainDb = m_trackGainDb + m_preampTaggedDb;
    } else if (m_mode == ReplayGainMode::AlbumGain) {
      effectiveGainDb = m_albumGainDb + m_preampTaggedDb;
    } else { // SmartGain
      effectiveGainDb = (m_isAlbumPlayback ? m_albumGainDb : m_trackGainDb) +
                        m_preampTaggedDb;
    }
  } else {
    // Untagged track
    effectiveGainDb = m_preampUntaggedDb;
  }

  m_appliedGainDb.store(effectiveGainDb, std::memory_order_release);
  float linear = std::pow(10.0f, effectiveGainDb / 20.0f);
  m_targetLinearGain.store(linear, std::memory_order_release);
}

void LoudnessAndDitherProcessor::setDitherMode(DitherMode mode) {
  m_ditherMode.store(mode, std::memory_order_release);
}

DitherMode LoudnessAndDitherProcessor::getDitherMode() const {
  return m_ditherMode.load(std::memory_order_relaxed);
}

void LoudnessAndDitherProcessor::setTargetBitDepth(uint8_t bitDepth) {
  m_targetBitDepth.store(bitDepth, std::memory_order_release);
}

uint8_t LoudnessAndDitherProcessor::getTargetBitDepth() const {
  return m_targetBitDepth.load(std::memory_order_relaxed);
}

float LoudnessAndDitherProcessor::getCurrentAppliedGainDb() const {
  return m_appliedGainDb.load(std::memory_order_relaxed);
}

float LoudnessAndDitherProcessor::getCurrentAppliedGainLinear() const {
  return m_currentLinearGain.load(std::memory_order_relaxed);
}

void LoudnessAndDitherProcessor::process(float *buffer, size_t frames,
                                         uint8_t channels) {
  if (!buffer || frames == 0 || channels == 0)
    return;

  const float targetGain = m_targetLinearGain.load(std::memory_order_relaxed);
  float currentGain = m_currentLinearGain.load(std::memory_order_relaxed);
  const bool hasGain = (std::abs(targetGain - 1.0f) > 0.0001f ||
                        std::abs(currentGain - 1.0f) > 0.0001f);
  const bool limiter = m_limiterEnabled.load(std::memory_order_relaxed);
  const DitherMode dither = m_ditherMode.load(std::memory_order_relaxed);

  if (!hasGain && !limiter && dither == DitherMode::None) {
    return; // Bit-perfect passthrough
  }

  const size_t totalSamples = frames * channels;
  const float gainStep =
      (targetGain - currentGain) / static_cast<float>(frames);

  // Compute LSB amplitude for 64-bit TPDF dithering based on target bit depth
  const uint8_t bitDepth = m_targetBitDepth.load(std::memory_order_relaxed);
  double lsb =
      1.0 / static_cast<double>(1ULL << (bitDepth > 0 ? (bitDepth - 1) : 15));

  constexpr float kThreshold = 0.988553f; // -0.1 dBFS
  constexpr float kThresholdInv = 1.0f / (1.0f - kThreshold);

  size_t idx = 0;
  for (size_t f = 0; f < frames; ++f) {
    currentGain += gainStep;

    for (uint8_t c = 0; c < channels; ++c) {
      float s = buffer[idx] * currentGain;

      // True-Peak Anti-Clipping Limiter (Soft Knee)
      if (limiter) {
        float absVal = std::abs(s);
        if (absVal > kThreshold) {
          float over = (absVal - kThreshold) * kThresholdInv;
          // Smooth saturation curve: tanh-like soft-knee clamping
          float compressed =
              kThreshold + (1.0f - kThreshold) * (over / (1.0f + over));
          s = (s > 0.0f) ? compressed : -compressed;
        }
      }

      // 64-bit TPDF Dithering
      if (dither != DitherMode::None) {
        double r1 = m_uniformDist(m_prng);
        double r2 = m_uniformDist(m_prng);
        double ditherNoise =
            (r1 + r2) * lsb; // Triangular distribution [-lsb, +lsb]

        if (dither == DitherMode::HighPassTpdf) {
          double shaped = ditherNoise - 0.5 * m_lastDitherNoise;
          m_lastDitherNoise = ditherNoise;
          ditherNoise = shaped;
        }

        s = static_cast<float>(static_cast<double>(s) + ditherNoise);
      }

      buffer[idx++] = std::clamp(s, -1.0f, 1.0f);
    }
  }

  m_currentLinearGain.store(targetGain, std::memory_order_relaxed);
}

} // namespace adapters
