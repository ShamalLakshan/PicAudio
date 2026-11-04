#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace adapters {

/**
 * @brief Standard surround channel layout identifiers.
 */
enum class ChannelLayoutId : uint8_t {
  Mono = 1,         ///< 1.0 (FC)
  Stereo = 2,       ///< 2.0 (FL, FR)
  Stereo21 = 3,     ///< 2.1 (FL, FR, LFE)
  Quadraphonic = 4, ///< 4.0 (FL, FR, RL, RR)
  Surround50 = 5,   ///< 5.0 (FL, FR, FC, RL, RR)
  Surround51 = 6,   ///< 5.1 (FL, FR, FC, LFE, RL, RR)
  Surround71 = 8    ///< 7.1 (FL, FR, FC, LFE, RL, RR, SL, SR)
};

/**
 * @brief Channel names corresponding to index 0..7.
 */
inline const char *const kChannelNames[8] = {
    "FL",  // 0: Front Left
    "FR",  // 1: Front Right
    "FC",  // 2: Front Center
    "LFE", // 3: Low Frequency Effects / Subwoofer
    "RL",  // 4: Rear Left
    "RR",  // 5: Rear Right
    "SL",  // 6: Side Left
    "SR"   // 7: Side Right
};

inline const char *const kChannelFullNames[8] = {
    "Front Left", "Front Right", "Front Center", "Subwoofer (LFE)",
    "Rear Left",  "Rear Right",  "Side Left",    "Side Right"};

/**
 * @brief Standard channel mixing matrix presets.
 */
enum class MatrixPreset {
  Passthrough,
  DownmixToStereo,
  DownmixToMono,
  UpmixTo51,
  UpmixTo71,
  Custom
};

/**
 * @brief Audiophile multi-channel matrix processor for Mono to 7.1 audio.
 *
 * Provides real-time downmixing, upmixing, channel routing, and custom
 * matrix coefficients with anti-clipping limiting and live VU level metering.
 */
class ChannelMatrixProcessor {
public:
  static constexpr size_t kMaxChannels = 8;

  ChannelMatrixProcessor();
  ~ChannelMatrixProcessor() = default;

  /**
   * @brief Configures channel counts and loads default mapping.
   * @param inChannels Number of input channels (1..8).
   * @param outChannels Number of output channels (1..8).
   */
  void configure(uint8_t inChannels, uint8_t outChannels);

  void setInputChannels(uint8_t channels);
  [[nodiscard]] uint8_t getInputChannels() const;

  void setOutputChannels(uint8_t channels);
  [[nodiscard]] uint8_t getOutputChannels() const;

  /**
   * @brief Sets matrix gain coefficient from inChannel to outChannel.
   * @param outChannel Output channel index (0..7).
   * @param inChannel Input channel index (0..7).
   * @param gain Linear gain multiplier [0.0f, 4.0f].
   */
  void setCoefficient(size_t outChannel, size_t inChannel, float gain);

  /**
   * @brief Gets current matrix gain coefficient.
   */
  [[nodiscard]] float getCoefficient(size_t outChannel, size_t inChannel) const;

  /**
   * @brief Applies standard mixing preset.
   */
  void loadPreset(MatrixPreset preset);
  [[nodiscard]] MatrixPreset getActivePreset() const;

  /**
   * @brief Normalizes row gains so no output channel sum exceeds 1.0f.
   */
  void normalize();

  /**
   * @brief Resets matrix to direct passthrough identity.
   */
  void resetToIdentity();

  /**
   * @brief Processes interleaved float PCM frames through the channel matrix.
   * @param input Pointer to interleaved input frames.
   * @param output Pointer to interleaved output frames.
   * @param frames Number of audio frames to process.
   */
  void process(const float *input, float *output, size_t frames);

  /**
   * @brief Returns current RMS levels for all 8 input channels in [0.0f, 1.0f].
   */
  [[nodiscard]] std::vector<float> getInputLevels() const;

  /**
   * @brief Returns current RMS levels for all 8 output channels in
   * [0.0f, 1.0f].
   */
  [[nodiscard]] std::vector<float> getOutputLevels() const;

  /**
   * @brief Serializes matrix configuration to JSON.
   */
  [[nodiscard]] std::string serialize() const;

  /**
   * @brief Deserializes matrix configuration from JSON.
   */
  bool deserialize(const std::string &json);

private:
  void applyDownmixToStereo();
  void applyDownmixToMono();
  void applyUpmixTo51();
  void applyUpmixTo71();

  mutable std::mutex m_mutex;
  uint8_t m_inChannels{2};
  uint8_t m_outChannels{2};
  MatrixPreset m_preset{MatrixPreset::Passthrough};

  // Matrix: m_matrix[outChannel][inChannel]
  std::array<std::array<float, kMaxChannels>, kMaxChannels> m_matrix{};

  // Channel metering
  mutable std::mutex m_meterMutex;
  std::array<float, kMaxChannels> m_inputLevels{};
  std::array<float, kMaxChannels> m_outputLevels{};
};

} // namespace adapters
