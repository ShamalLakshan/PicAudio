#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace adapters {

/**
 * @brief Audiophile parametric biquad IIR equalizer supporting 10-band,
 * 15-band, and 31-band ISO standard octave layouts with lock-free parameter
 * double-buffering, per-channel state, preamp gain, and anti-denormal guards.
 */
class ParametricEqualizer {
public:
  static constexpr size_t kMaxChannels = 8;
  static constexpr size_t kMaxBands = 32;

  ParametricEqualizer();
  ~ParametricEqualizer() = default;

  /**
   * @brief Configures filter sample rate and channel layout.
   */
  void configure(uint32_t sampleRate, uint8_t channels);

  /**
   * @brief Sets active band count (10, 15, or 31).
   */
  void setBandCount(size_t count);

  /**
   * @brief Gets current band count.
   */
  size_t getBandCount() const;

  /**
   * @brief Gets center frequencies for current band layout in Hz.
   */
  std::vector<float> getBandFrequencies() const;

  /**
   * @brief Gets displayable frequency labels for current band layout.
   */
  std::vector<std::string> getBandFrequencyLabels() const;

  /**
   * @brief Sets gain in dB for a specific band (-12.0 to +12.0 dB).
   */
  void setBandGain(size_t bandIndex, float gainDb);

  /**
   * @brief Gets current gain in dB for a band.
   */
  float getBandGain(size_t bandIndex) const;

  /**
   * @brief Sets all band gains simultaneously.
   */
  void setBandGains(const std::vector<float> &gains);

  /**
   * @brief Gets all current band gains.
   */
  std::vector<float> getBandGains() const;

  /**
   * @brief Sets master preamp gain in dB (-12.0 to +12.0 dB).
   */
  void setPreampGain(float gainDb);

  /**
   * @brief Gets master preamp gain in dB.
   */
  float getPreampGain() const;

  /**
   * @brief Enables or bypasses the equalizer.
   */
  void setBypassed(bool bypass);

  /**
   * @brief Returns true if equalizer is currently bypassed.
   */
  bool isBypassed() const;

  /**
   * @brief Sets active preset name and updates band gains.
   */
  void setPreset(const std::string &presetName);

  /**
   * @brief Gets active preset name.
   */
  std::string getPreset() const;

  /**
   * @brief Resets all bands and preamp to flat (0 dB).
   */
  void reset();

  /**
   * @brief Processes interleaved 32-bit floating point PCM audio frames
   * in-place. Real-time audio safe and 100% lock-free.
   * @param buffer Pointer to interleaved audio samples.
   * @param frames Number of audio frames.
   */
  void process(float *buffer, size_t frames);

  /**
   * @brief Serializes current EQ settings to JSON format.
   */
  std::string serialize() const;

  /**
   * @brief Deserializes EQ settings from JSON format.
   */
  bool deserialize(const std::string &json);

private:
  struct BiquadCoefficients {
    float b0{1.0f};
    float b1{0.0f};
    float b2{0.0f};
    float a1{0.0f};
    float a2{0.0f};
    bool isPassthrough{true};
  };

  struct BiquadState {
    float s1{0.0f};
    float s2{0.0f};
  };

  struct FilterConfiguration {
    std::array<BiquadCoefficients, kMaxBands> coeffs{};
    size_t bandCount{10};
    uint8_t channels{2};
    float preampLinear{1.0f};
    bool bypassed{false};
  };

  void applyBandCountLocked(size_t count);
  void recalculateBandLocked(size_t bandIndex);
  void recalculateCoefficientsLocked();
  void publishConfigurationLocked();

  mutable std::mutex m_mutex;
  uint32_t m_sampleRate{44100};
  uint8_t m_channels{2};
  size_t m_bandCount{10};
  float m_filterQ{1.41421356f};

  std::vector<float> m_bandFrequencies;
  std::vector<std::string> m_bandLabels;
  std::vector<float> m_bandGainsDb;
  std::array<BiquadCoefficients, kMaxBands> m_stagingCoeffs{};

  float m_preampDb{0.0f};
  float m_preampLinear{1.0f};
  std::atomic<bool> m_bypassed{false};
  std::string m_preset{"Flat"};

  // Double-buffered lock-free configurations for real-time DSP
  std::array<FilterConfiguration, 2> m_configs{};
  std::atomic<uint32_t> m_activeConfigIndex{0};

  // State accessed exclusively on the real-time audio thread
  std::array<std::array<BiquadState, kMaxChannels>, kMaxBands> m_states{};
  std::atomic<bool> m_resetStatePending{false};
};

} // namespace adapters
