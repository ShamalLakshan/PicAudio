#pragma once

#include "audio/IAudioSink.hpp"
#include "audio/LockFreeRingBuffer.hpp"
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#if defined(PICAUDIO_ENABLE_PIPEWIRE)
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <spa/param/props.h>
#endif

namespace adapters {

/**
 * @brief Native PipeWire lock-free audio output client sink.
 *
 * Direct libpipewire client utilizing SPSC ring buffering and real-time
 * low-latency stream processing.
 */
class PipeWireAudioSink : public IAudioSink {
public:
  PipeWireAudioSink();
  ~PipeWireAudioSink() override;

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

  void processAudio();

private:
#if defined(PICAUDIO_ENABLE_PIPEWIRE)
  pw_thread_loop *m_loop{nullptr};
  pw_context *m_context{nullptr};
  pw_stream *m_stream{nullptr};
  spa_hook m_streamListener{};
#endif
  mutable std::mutex m_mutex;
  std::atomic<bool> m_isOpen{false};
  std::atomic<bool> m_isPaused{false};
  std::atomic<uint64_t> m_xrunCount{0};
  uint32_t m_latencyMs{20};
  size_t m_frameBytes{8};
  AudioSinkConfig m_config;
  LockFreeRingBuffer<uint8_t> m_ringBuffer{131072};
};

} // namespace adapters
