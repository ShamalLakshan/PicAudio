#pragma once

#include "fycore_export.h"
#include <array>
#include <atomic>
#include <core/engine/dsp/dspnode.h>
#include <mutex>
#include <vector>

namespace PicAudioEngine {

/**
 * @brief Audiophile multi-channel matrix processor for Mono to 7.1 surround
 * sound.
 *
 * Implements real-time channel downmixing, upmixing, routing, and custom
 * matrix coefficients with anti-clipping limiting and lock-free live parameter
 * swaps.
 */
class PICAUDIO_ENGINE_EXPORT ChannelMatrixDsp : public DspNode {
public:
  static constexpr size_t kMaxChannels = 8;

  enum class Preset : int {
    Passthrough = 0,
    DownmixToStereo = 1,
    DownmixToMono = 2,
    UpmixTo51 = 3,
    UpmixTo71 = 4,
    Custom = 5
  };

  ChannelMatrixDsp();
  ~ChannelMatrixDsp() override = default;

  [[nodiscard]] QString name() const override;
  [[nodiscard]] QString id() const override;
  [[nodiscard]] bool supportsLiveSettings() const override { return true; }

  AudioFormat outputFormat(const AudioFormat &input) const override;
  void prepare(const AudioFormat &format) override;
  void process(ProcessingBufferList &chunks) override;
  void reset() override;

  [[nodiscard]] QByteArray saveSettings() const override;
  bool loadSettings(const QByteArray &preset) override;

  void setCoefficient(size_t outCh, size_t inCh, float gain);
  [[nodiscard]] float coefficient(size_t outCh, size_t inCh) const;

  void setPreset(Preset preset);
  [[nodiscard]] Preset activePreset() const;

  void setOutputChannels(int channels);
  [[nodiscard]] int outputChannels() const;

  void normalize();
  void resetToIdentity();

  void setBypassed(bool bypass);
  [[nodiscard]] bool isBypassed() const;

  [[nodiscard]] std::vector<float> inputLevels() const;
  [[nodiscard]] std::vector<float> outputLevels() const;

private:
  struct MatrixConfig {
    std::array<std::array<double, kMaxChannels>, kMaxChannels> matrix{};
    int inChannels{2};
    int outChannels{2};
    Preset preset{Preset::Passthrough};
    bool bypassed{false};
    bool isPassthrough{true};
  };

  void applyDownmixToStereoLocked();
  void applyDownmixToMonoLocked();
  void applyUpmixTo51Locked();
  void applyUpmixTo71Locked();
  void normalizeLocked();
  void resetToIdentityLocked();
  void publishConfigLocked();

  mutable std::recursive_mutex m_mutex;
  AudioFormat m_inputFormat;
  AudioFormat m_outputFormat;
  int m_inChannels{2};
  int m_outChannels{2};
  Preset m_preset{Preset::Passthrough};
  std::atomic<bool> m_bypassed{false};

  std::array<std::array<double, kMaxChannels>, kMaxChannels> m_stagingMatrix{};

  // Lock-free double-buffered configuration for audio thread
  std::array<MatrixConfig, 2> m_configs{};
  std::atomic<uint32_t> m_activeConfigIdx{0};

  // Lock-free atomic channel metering (0.0 .. 1.0 RMS)
  std::array<std::atomic<float>, kMaxChannels> m_inputLevels{};
  std::array<std::atomic<float>, kMaxChannels> m_outputLevels{};
};

} // namespace PicAudioEngine
