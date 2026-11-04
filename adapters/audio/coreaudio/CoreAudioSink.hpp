#pragma once

#include "audio/IAudioSink.hpp"
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#if defined(__APPLE__)
#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>
#endif

namespace adapters {

/**
 * @brief macOS CoreAudio HAL output sink supporting Integer Mode, Hog Mode
 * exclusivity, and automatic DAC nominal sample rate switching.
 */
class CoreAudioSink : public IAudioSink {
public:
  CoreAudioSink();
  ~CoreAudioSink() override;

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

  bool setHogMode(bool enable);
  [[nodiscard]] bool isHogModeActive() const;

private:
  mutable std::recursive_mutex m_mutex;
  std::atomic<bool> m_isOpen{false};
  std::atomic<bool> m_isPaused{false};
  std::atomic<bool> m_hogModeActive{false};
  std::atomic<uint64_t> m_xrunCount{0};
  uint32_t m_latencyMs{15};
  AudioSinkConfig m_activeConfig;
  size_t m_frameBytes{4};

#if defined(__APPLE__)
  AudioComponentInstance m_audioUnit{nullptr};
  AudioDeviceID m_deviceID{kAudioObjectUnknown};
  pid_t m_hogPid{-1};
#endif
};

} // namespace adapters
