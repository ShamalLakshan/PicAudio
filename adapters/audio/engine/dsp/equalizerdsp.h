#pragma once
#ifndef PICAUDIO_ENGINE_EQUALIZERDSP_H
#define PICAUDIO_ENGINE_EQUALIZERDSP_H

#include "fycore_export.h"
#include <array>
#include <atomic>
#include "dspnode.h"
#include <mutex>
#include <vector>

namespace PicAudioEngine {

/**
 * @brief Filter type for parametric equalizer bands.
 */
enum class PeqFilterType : uint8_t {
  Peaking = 0,
  LowShelf,
  HighShelf,
  LowPass,
  HighPass,
  BandPass,
  Notch
};

/**
 * @brief Configuration for a single parametric biquad equalizer filter.
 */
struct PeqFilterSpec {
  PeqFilterType type{PeqFilterType::Peaking};
  float frequency{1000.0f};
  float gainDb{0.0f};
  float q{1.414f};
  bool enabled{true};
};

/**
 * @brief Real-time 64-bit double precision parametric biquad equalizer node.
 * Supports 10, 15, and 31 ISO standard octave frequency bands, as well as
 * fully arbitrary parametric (PEQ) biquad configurations.
 */
class PICAUDIO_ENGINE_EXPORT EqualizerDsp : public DspNode {
public:
  static constexpr size_t kMaxChannels = 8;
  static constexpr size_t kMaxBands = 32;

  EqualizerDsp();
  ~EqualizerDsp() override = default;

  [[nodiscard]] QString name() const override;
  [[nodiscard]] QString id() const override;
  [[nodiscard]] bool supportsLiveSettings() const override { return true; }

  AudioFormat outputFormat(const AudioFormat &input) const override;
  void prepare(const AudioFormat &format) override;
  void process(ProcessingBufferList &chunks) override;
  void reset() override;

  [[nodiscard]] QByteArray saveSettings() const override;
  bool loadSettings(const QByteArray &preset) override;

  void setEqMode(int mode);
  [[nodiscard]] int eqMode() const;

  void setBandCount(size_t count);
  [[nodiscard]] size_t bandCount() const;

  void setBandGain(size_t bandIndex, float gainDb);
  [[nodiscard]] float bandGain(size_t bandIndex) const;
  void setBandGains(const std::vector<float> &gains);
  [[nodiscard]] std::vector<float> bandGains() const;

  void setPeqFilters(const std::vector<PeqFilterSpec> &filters);
  [[nodiscard]] std::vector<PeqFilterSpec> peqFilters() const;

  void setPreampGain(float gainDb);
  [[nodiscard]] float preampGain() const;

  void setBypassed(bool bypass);
  [[nodiscard]] bool isBypassed() const;

  void setPreset(const QString &presetName);
  [[nodiscard]] QString preset() const;

  static std::vector<float> presetGains(const QString &presetName,
                                        size_t bandCount);
  static std::vector<QString> availablePresets();
  static std::vector<float> bandFrequenciesForCount(size_t count);
  static std::vector<QString> bandFrequencyLabelsForCount(size_t count);

  static std::vector<float>
  calculateResponse(const std::vector<PeqFilterSpec> &filters, float preampDb,
                    const std::vector<float> &frequencies,
                    int sampleRate = 44100);

private:
  struct BiquadCoeffs {
    double b0{1.0};
    double b1{0.0};
    double b2{0.0};
    double a1{0.0};
    double a2{0.0};
    bool isPassthrough{true};
  };

  struct BiquadState {
    double s1{0.0};
    double s2{0.0};
  };

  struct FilterConfig {
    std::array<BiquadCoeffs, kMaxBands> coeffs{};
    size_t bandCount{10};
    int channels{2};
    double preampLinear{1.0};
    bool bypassed{false};
  };

  void recalculateCoefficientsLocked();
  void publishConfigLocked();

  mutable std::recursive_mutex m_mutex;
  int m_sampleRate{44100};
  int m_channels{2};
  int m_mode{0}; // 0: Graphic, 1: Parametric
  size_t m_bandCount{10};
  float m_filterQ{1.41421356f};

  std::vector<float> m_bandFrequencies;
  std::vector<float> m_bandGainsDb;
  std::vector<PeqFilterSpec> m_peqFilters;
  std::array<BiquadCoeffs, kMaxBands> m_stagingCoeffs{};

  float m_preampDb{0.0f};
  double m_preampLinear{1.0};
  std::atomic<bool> m_bypassed{false};
  QString m_preset{QStringLiteral("Flat")};

  // Lock-free double buffering for audio thread
  std::array<FilterConfig, 2> m_configs{};
  std::atomic<uint32_t> m_activeConfigIdx{0};

  // Audio-thread state
  std::array<std::array<BiquadState, kMaxChannels>, kMaxBands> m_states{};
  std::atomic<bool> m_resetPending{false};
};

} // namespace PicAudioEngine
#endif // PICAUDIO_ENGINE_EQUALIZERDSP_H
