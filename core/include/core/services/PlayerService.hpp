#pragma once

#include "core/entities/Track.hpp"
#include "core/ports/IAudioEnginePort.hpp"
#include "core/services/QueueService.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace core {

/**
 * @brief Playback states.
 */
enum class PlaybackState { Stopped, Playing, Paused };

/**
 * @brief Manages playback, audio engine state, and the queue.
 */
class PlayerService {
public:
  explicit PlayerService(
      std::shared_ptr<IAudioEnginePort> audioEngine = nullptr);
  ~PlayerService() = default;

  /** @brief Sets the active audio engine. */
  void setAudioEngine(std::shared_ptr<IAudioEnginePort> audioEngine);

  /** @brief Plays a single track directly. */
  void play(const Track &track);

  /** @brief Loads tracks into queue and starts playback at the given index. */
  void playQueue(const std::vector<Track> &tracks, size_t startIndex = 0);

  /**
   * @brief Restores a saved playback session state.
   * @param currentTrack Active track to restore.
   * @param positionMs Playback position in milliseconds.
   * @param upNext Up next queue tracks.
   * @param mainStream Main stream tracks.
   * @param mainStreamIndex Index in main stream.
   * @param history Previous tracks history stack.
   * @param autoResume True to start playing immediately; false to load paused.
   */
  void restoreSession(const std::optional<Track> &currentTrack,
                      uint64_t positionMs, const std::vector<Track> &upNext,
                      const std::vector<Track> &mainStream,
                      size_t mainStreamIndex, const std::vector<Track> &history,
                      bool autoResume = false);

  void pause();
  void resume();
  void togglePlayPause();
  void stop();
  void next();
  void previous();
  void seek(uint64_t positionMs);
  void setVolume(float volume);

  // Queue actions
  void playNext(const Track &track);
  void playNext(const std::vector<Track> &tracks);
  void queueLast(const Track &track);
  void queueLast(const std::vector<Track> &tracks);
  bool removeFromQueue(size_t index);
  bool moveQueueItem(size_t fromIndex, size_t toIndex);
  void clearQueue();
  void shuffleRemaining();
  void clearHistory() { m_queueService.clearHistory(); }
  void setHistoryLimit(size_t limit) { m_queueService.setHistoryLimit(limit); }
  [[nodiscard]] size_t getHistoryLimit() const {
    return m_queueService.getHistoryLimit();
  }

  // Mode controls
  void setRepeatMode(RepeatMode mode);
  [[nodiscard]] RepeatMode getRepeatMode() const {
    return m_queueService.getRepeatMode();
  }
  void cycleRepeatMode();

  void setShuffleMode(ShuffleMode mode);
  [[nodiscard]] ShuffleMode getShuffleMode() const {
    return m_queueService.getShuffleMode();
  }

  void setPlaybackRate(float rate) {
    if (m_audioEngine) {
      m_audioEngine->setPlaybackRate(rate);
    }
  }
  [[nodiscard]] float getPlaybackRate() const {
    if (m_audioEngine) {
      return m_audioEngine->getPlaybackRate();
    }
    return 1.0f;
  }

  void setAudioBackend(const std::string &backend) {
    if (m_audioEngine) {
      m_audioEngine->setAudioBackend(backend);
    }
  }
  [[nodiscard]] std::string getAudioBackend() const {
    return m_audioEngine ? m_audioEngine->getAudioBackend() : "";
  }

  void setAudioDevice(const std::string &device) {
    if (m_audioEngine) {
      m_audioEngine->setAudioDevice(device);
    }
  }
  [[nodiscard]] std::string getAudioDevice() const {
    return m_audioEngine ? m_audioEngine->getAudioDevice() : "";
  }

  void setBufferLatencyMs(uint32_t ms) {
    if (m_audioEngine) {
      m_audioEngine->setBufferLatencyMs(ms);
    }
  }
  [[nodiscard]] uint32_t getBufferLatencyMs() const {
    return m_audioEngine ? m_audioEngine->getBufferLatencyMs() : 50;
  }

  void setBitPerfectExclusive(bool exclusive) {
    if (m_audioEngine) {
      m_audioEngine->setBitPerfectExclusive(exclusive);
    }
  }
  [[nodiscard]] bool isBitPerfectExclusive() const {
    return m_audioEngine ? m_audioEngine->isBitPerfectExclusive() : true;
  }

  [[nodiscard]] uint64_t getXrunCount() const {
    return m_audioEngine ? m_audioEngine->getXrunCount() : 0;
  }

  [[nodiscard]] std::vector<IAudioEnginePort::DeviceInfo>
  getAvailableAudioDevices(const std::string &backend = "") const {
    return m_audioEngine ? m_audioEngine->getAvailableDevices(backend)
                         : std::vector<IAudioEnginePort::DeviceInfo>{};
  }

  void setChannelMatrixCoefficient(size_t outCh, size_t inCh, float gain) {
    if (m_audioEngine) {
      m_audioEngine->setChannelMatrixCoefficient(outCh, inCh, gain);
    }
  }
  [[nodiscard]] float getChannelMatrixCoefficient(size_t outCh,
                                                  size_t inCh) const {
    return m_audioEngine
               ? m_audioEngine->getChannelMatrixCoefficient(outCh, inCh)
               : ((outCh == inCh) ? 1.0f : 0.0f);
  }

  void setChannelMatrixPreset(int preset) {
    if (m_audioEngine) {
      m_audioEngine->setChannelMatrixPreset(preset);
    }
  }
  [[nodiscard]] int getChannelMatrixPreset() const {
    return m_audioEngine ? m_audioEngine->getChannelMatrixPreset() : 0;
  }

  void normalizeChannelMatrix() {
    if (m_audioEngine) {
      m_audioEngine->normalizeChannelMatrix();
    }
  }

  [[nodiscard]] std::vector<float> getChannelLevels() const {
    return m_audioEngine ? m_audioEngine->getChannelLevels()
                         : std::vector<float>{};
  }

  void setOutputChannelCount(uint8_t channels) {
    if (m_audioEngine) {
      m_audioEngine->setOutputChannelCount(channels);
    }
  }
  [[nodiscard]] uint8_t getOutputChannelCount() const {
    return m_audioEngine ? m_audioEngine->getOutputChannelCount() : 2;
  }

  [[nodiscard]] std::string getSerializedChannelMatrix() const {
    return m_audioEngine ? m_audioEngine->getSerializedChannelMatrix() : "";
  }
  bool setSerializedChannelMatrix(const std::string &json) {
    return m_audioEngine ? m_audioEngine->setSerializedChannelMatrix(json)
                         : false;
  }

  [[nodiscard]] PlaybackState getState() const { return m_state; }
  [[nodiscard]] bool isPlaying() const {
    return m_state == PlaybackState::Playing;
  }
  [[nodiscard]] std::optional<Track> getCurrentTrack() const {
    return m_currentTrack;
  }
  [[nodiscard]] uint64_t getPositionMs() const;
  [[nodiscard]] uint64_t getDurationMs() const;
  [[nodiscard]] float getVolume() const { return m_volume; }
  [[nodiscard]] std::vector<Track> getQueue() const {
    return m_queueService.getUpcomingQueue();
  }
  [[nodiscard]] QueueService &getQueueService() { return m_queueService; }
  [[nodiscard]] const QueueService &getQueueService() const {
    return m_queueService;
  }

  [[nodiscard]] std::vector<float>
  getFrequencyBands(size_t bandCount = 36) const {
    if (m_audioEngine) {
      return m_audioEngine->getFrequencyBands(bandCount);
    }
    return {};
  }

  [[nodiscard]] std::vector<float>
  getWaveformSamples(size_t sampleCount = 64) const {
    if (m_audioEngine) {
      return m_audioEngine->getWaveformSamples(sampleCount);
    }
    return {};
  }

  [[nodiscard]] float getAudioLevel() const {
    if (m_audioEngine) {
      return m_audioEngine->getAudioLevel();
    }
    return 0.0f;
  }

  void onTrackChanged(std::function<void(const Track &)> callback) {
    m_trackChangedCallback = callback;
  }
  void onStateChanged(std::function<void(PlaybackState)> callback) {
    m_stateChangedCallback = callback;
  }

private:
  std::shared_ptr<IAudioEnginePort> m_audioEngine;
  QueueService m_queueService;
  PlaybackState m_state{PlaybackState::Stopped};
  std::optional<Track> m_currentTrack;
  float m_volume{0.8f};

  std::function<void(const Track &)> m_trackChangedCallback;
  std::function<void(PlaybackState)> m_stateChangedCallback;
};

} // namespace core
