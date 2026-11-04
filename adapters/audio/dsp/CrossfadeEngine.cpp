#include "CrossfadeEngine.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace adapters {

constexpr double kPiOver2 = 1.57079632679489661923;

CrossfadeEngine::CrossfadeEngine() = default;

void CrossfadeEngine::setEnabled(bool enabled) {
  m_enabled.store(enabled, std::memory_order_release);
}

bool CrossfadeEngine::isEnabled() const {
  return m_enabled.load(std::memory_order_relaxed);
}

void CrossfadeEngine::setDurationSec(float seconds) {
  m_durationSec.store(std::clamp(seconds, 0.5f, 10.0f),
                      std::memory_order_release);
}

float CrossfadeEngine::getDurationSec() const {
  return m_durationSec.load(std::memory_order_relaxed);
}

void CrossfadeEngine::setCurve(CrossfadeCurve curve) {
  m_curve.store(curve, std::memory_order_release);
}

CrossfadeCurve CrossfadeEngine::getCurve() const {
  return m_curve.load(std::memory_order_relaxed);
}

void CrossfadeEngine::setAutoDisableSameAlbum(bool disable) {
  m_autoDisableSameAlbum.store(disable, std::memory_order_release);
}

bool CrossfadeEngine::isAutoDisableSameAlbum() const {
  return m_autoDisableSameAlbum.load(std::memory_order_relaxed);
}

void CrossfadeEngine::setGaplessEnabled(bool enabled) {
  m_gaplessEnabled.store(enabled, std::memory_order_release);
}

bool CrossfadeEngine::isGaplessEnabled() const {
  return m_gaplessEnabled.load(std::memory_order_relaxed);
}

bool CrossfadeEngine::shouldTriggerCrossfade(const std::string &currentAlbum,
                                             const std::string &nextAlbum,
                                             uint64_t remainingMs) const {
  if (!m_enabled.load(std::memory_order_relaxed)) {
    return false;
  }

  // Preserve intended gapless playback when consecutive tracks are on the same
  // album
  if (m_autoDisableSameAlbum.load(std::memory_order_relaxed) &&
      !currentAlbum.empty() && currentAlbum == nextAlbum) {
    return false;
  }

  uint64_t thresholdMs = static_cast<uint64_t>(
      m_durationSec.load(std::memory_order_relaxed) * 1000.0f);
  return (remainingMs <= thresholdMs && remainingMs > 0);
}

void CrossfadeEngine::beginCrossfade(size_t totalTransitionFrames) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_transitionFrames =
      totalTransitionFrames > 0 ? totalTransitionFrames : 44100;
  m_elapsedFrames = 0;
  m_incomingReadPos = 0;
  m_isCrossfading.store(true, std::memory_order_release);
}

void CrossfadeEngine::writeIncomingSamples(const float *samples, size_t count) {
  if (!samples || count == 0)
    return;
  std::lock_guard<std::mutex> lock(m_mutex);
  m_incomingBuffer.insert(m_incomingBuffer.end(), samples, samples + count);
}

void CrossfadeEngine::process(float *outgoingBuffer, size_t frames,
                              uint8_t channels) {
  if (!outgoingBuffer || frames == 0 || channels == 0 ||
      !m_isCrossfading.load(std::memory_order_relaxed)) {
    return;
  }

  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_transitionFrames == 0) {
    m_isCrossfading.store(false, std::memory_order_release);
    return;
  }

  const CrossfadeCurve curve = m_curve.load(std::memory_order_relaxed);
  size_t idx = 0;

  for (size_t f = 0; f < frames; ++f) {
    if (m_elapsedFrames >= m_transitionFrames) {
      m_isCrossfading.store(false, std::memory_order_release);
      break;
    }

    double t = static_cast<double>(m_elapsedFrames) /
               static_cast<double>(m_transitionFrames);
    t = std::clamp(t, 0.0, 1.0);

    float fadeOutGain = 1.0f;
    float fadeInGain = 0.0f;

    switch (curve) {
    case CrossfadeCurve::EqualPower:
      fadeOutGain = static_cast<float>(std::cos(t * kPiOver2));
      fadeInGain = static_cast<float>(std::sin(t * kPiOver2));
      break;
    case CrossfadeCurve::Linear:
      fadeOutGain = static_cast<float>(1.0 - t);
      fadeInGain = static_cast<float>(t);
      break;
    case CrossfadeCurve::Logarithmic:
      fadeOutGain = static_cast<float>(std::pow(1.0 - t, 2.0));
      fadeInGain = static_cast<float>(std::sqrt(t));
      break;
    }

    for (uint8_t c = 0; c < channels; ++c) {
      float outSample = outgoingBuffer[idx] * fadeOutGain;
      float inSample = 0.0f;
      if (m_incomingReadPos < m_incomingBuffer.size()) {
        inSample = m_incomingBuffer[m_incomingReadPos++] * fadeInGain;
      }
      outgoingBuffer[idx++] = std::clamp(outSample + inSample, -1.0f, 1.0f);
    }

    m_elapsedFrames++;
  }
}

bool CrossfadeEngine::isCrossfadeActive() const {
  return m_isCrossfading.load(std::memory_order_relaxed);
}

void CrossfadeEngine::reset() {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_isCrossfading.store(false, std::memory_order_release);
  m_transitionFrames = 0;
  m_elapsedFrames = 0;
  m_incomingBuffer.clear();
  m_incomingReadPos = 0;
}

} // namespace adapters
