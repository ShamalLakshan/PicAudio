#pragma once

#include <picaudio/plugin/Defines.hpp>
#include <picaudio/plugin/IPlugin.hpp>
#include <picaudio/plugin/IUiPlugin.hpp>
#include <picaudio/plugin/IVisualizerPlugin.hpp>

#include <complex>
#include <mutex>
#include <vector>

namespace picaudio::plugins {

/**
 * @brief Reference real-time FFT spectrum visualizer plugin.
 */
class SpectrumVisualizerPlugin final : public plugin::IPicAudioPlugin,
                                       public plugin::IVisualizerPlugin,
                                       public plugin::IUiPlugin {
public:
  SpectrumVisualizerPlugin();
  ~SpectrumVisualizerPlugin() override = default;

  // IPicAudioPlugin methods
  [[nodiscard]] const char *getId() const noexcept override;
  [[nodiscard]] const char *getName() const noexcept override;
  [[nodiscard]] const char *getVersion() const noexcept override;
  [[nodiscard]] const char *getAuthor() const noexcept override;
  [[nodiscard]] const char *getDescription() const noexcept override;
  [[nodiscard]] uint32_t getApiVersion() const noexcept override;
  [[nodiscard]] uint32_t getCapabilities() const noexcept override;

  bool initialize() override;
  void start() override;
  void stop() override;
  void shutdown() override;

  [[nodiscard]] void *getCapabilityInterface(uint32_t capability) override;

  // IVisualizerPlugin methods
  void processAudioBuffer(const float *buffer, size_t frames,
                          uint32_t sampleRate, uint8_t channels) override;
  size_t getFrequencyBands(float *outBands, size_t maxBands) const override;
  [[nodiscard]] bool isEnabled() const noexcept override;
  void setEnabled(bool enabled) override;

  // IUiPlugin methods
  void
  setHostUiServices(const plugin::PicAudioHostUiServices *services) override;
  size_t getCustomActions(plugin::PicAudioCustomAction *outActions,
                          size_t maxActions) override;
  bool triggerAction(const char *actionId) override;

private:
  void computeFftAndBands();
  static void radix2Fft(std::vector<std::complex<float>> &data);

  static constexpr size_t kFftSize = 1024;
  static constexpr size_t kDefaultBands = 64;

  const plugin::PicAudioHostUiServices *m_hostUiServices{nullptr};
  bool m_enabled{true};
  uint32_t m_sampleRate{48000};
  std::vector<float> m_hannWindow;
  std::vector<float> m_inputRingBuffer;
  size_t m_ringWritePos{0};
  size_t m_accumulatedFrames{0};

  std::vector<float> m_currentBands;
  std::vector<float> m_smoothedBands;
  mutable std::mutex m_mutex;
};

} // namespace picaudio::plugins
