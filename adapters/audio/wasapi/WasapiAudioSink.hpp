#pragma once

#include "audio/IAudioSink.hpp"
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <windows.h>
#include <wrl/client.h>
#endif

namespace adapters {

/**
 * @brief Windows WASAPI audio sink supporting Exclusive Mode and low-latency
 * Shared Mode fallback.
 *
 * Provides bit-perfect integer and float streaming directly to hardware
 * endpoints, bypassing Windows Audio Engine (audiodg.exe) mixer and resampler.
 */
class WasapiAudioSink : public IAudioSink {
public:
  WasapiAudioSink();
  ~WasapiAudioSink() override;

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

  [[nodiscard]] bool isExclusiveModeActive() const;

private:
  mutable std::recursive_mutex m_mutex;
  std::atomic<bool> m_isOpen{false};
  std::atomic<bool> m_isPaused{false};
  std::atomic<bool> m_isExclusive{true};
  std::atomic<uint64_t> m_xrunCount{0};
  uint32_t m_latencyMs{10};
  AudioSinkConfig m_activeConfig;
  size_t m_frameBytes{4};

#if defined(_WIN32)
  Microsoft::WRL::ComPtr<IMMDeviceEnumerator> m_enumerator;
  Microsoft::WRL::ComPtr<IMMDevice> m_device;
  Microsoft::WRL::ComPtr<IAudioClient> m_audioClient;
  Microsoft::WRL::ComPtr<IAudioRenderClient> m_renderClient;
  HANDLE m_audioEvent{nullptr};
  UINT32 m_bufferFrameCount{0};
  bool m_comInitialized{false};
#else
  uint32_t m_simulatedBufferFrames{1024};
#endif
};

} // namespace adapters
