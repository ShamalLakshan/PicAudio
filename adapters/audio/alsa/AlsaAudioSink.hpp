#pragma once

#include "audio/IAudioSink.hpp"
#include <atomic>
#include <mutex>
#include <string>
#include <vector>

#if defined(PICAUDIO_ENABLE_ALSA)
#include <alsa/asoundlib.h>
#endif

namespace adapters {

/**
 * @brief Dedicated ALSA direct hardware output sink (hw: and plughw:).
 *
 * Bypasses software mixers (PulseAudio, pipewire-pulse, dmix) to deliver
 * bit-perfect integer PCM streams directly to audio hardware devices.
 */
class AlsaAudioSink : public IAudioSink {
public:
  AlsaAudioSink();
  ~AlsaAudioSink() override;

  bool open(const AudioSinkConfig &config) override;
  void close() override;
  [[nodiscard]] bool isOpen() const override;

  size_t write(const void *buffer, size_t frames) override;
  void pause() override;
  void resume() override;
  void flush() override;

  [[nodiscard]] uint32_t getLatencyMs() const override;
  [[nodiscard]] uint64_t getXrunCount() const override;
  [[nodiscard]] bool isBitPerfect() const override;
  [[nodiscard]] std::string getName() const override;
  std::vector<AudioDeviceInfo> enumerateDevices() override;

private:
#if defined(PICAUDIO_ENABLE_ALSA)
  snd_pcm_t *m_pcm{nullptr};
  snd_pcm_format_t m_format{SND_PCM_FORMAT_S16_LE};
#endif
  mutable std::mutex m_mutex;
  std::atomic<bool> m_isOpen{false};
  std::atomic<bool> m_isPaused{false};
  std::atomic<uint64_t> m_xrunCount{0};
  uint32_t m_latencyMs{50};
  bool m_isBitPerfect{true};
  size_t m_frameBytes{4};
  AudioSinkConfig m_config;
};

} // namespace adapters
