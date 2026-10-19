#pragma once

#include "core/entities/Track.hpp"
#include <algorithm>
#include <cstdint>
#include <deque>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace core {

enum class RepeatMode { Off = 0, All = 1, One = 2 };

enum class ShuffleMode { Off = 0, Tracks = 1, Albums = 2 };

/**
 * @brief Manages the playback queue, upcoming tracks, history, and
 * shuffle/repeat modes.
 */
class QueueService {
public:
  QueueService();
  explicit QueueService(uint32_t randomSeed);
  ~QueueService() = default;

  // Playback control
  void playNow(const Track &track, const std::vector<Track> &contextTracks = {},
               size_t trackIndex = 0);
  void playNext(const Track &track);
  void playNext(const std::vector<Track> &tracks);
  void queueLast(const Track &track);
  void queueLast(const std::vector<Track> &tracks);

  [[nodiscard]] std::optional<Track> next();
  [[nodiscard]] std::optional<Track> previous();

  // Queue mutations
  bool removeFromQueue(size_t index);
  bool moveQueueItem(size_t fromIndex, size_t toIndex);
  void clearQueue();
  void shuffleRemaining();

  /**
   * @brief Restores the queue state, up next sequence, and history stack.
   * @param currentTrack Active track if any.
   * @param upNext List of upcoming manually queued tracks.
   * @param mainStream Main playback stream tracks.
   * @param mainStreamIndex Current index in the main stream.
   * @param history Previous tracks history stack.
   */
  void restoreState(const std::optional<Track> &currentTrack,
                    const std::vector<Track> &upNext,
                    const std::vector<Track> &mainStream,
                    size_t mainStreamIndex, const std::vector<Track> &history);

  // Modes
  void setRepeatMode(RepeatMode mode);
  [[nodiscard]] RepeatMode getRepeatMode() const noexcept {
    return m_repeatMode;
  }
  void cycleRepeatMode();

  void setShuffleMode(ShuffleMode mode);
  [[nodiscard]] ShuffleMode getShuffleMode() const noexcept {
    return m_shuffleMode;
  }

  // State queries
  [[nodiscard]] std::optional<Track> getCurrentTrack() const noexcept {
    return m_currentTrack;
  }
  [[nodiscard]] const std::vector<Track> &getUpNext() const noexcept {
    return m_upNext;
  }
  [[nodiscard]] const std::vector<Track> &getMainStream() const noexcept {
    return m_mainStream;
  }
  [[nodiscard]] size_t getMainStreamIndex() const noexcept {
    return m_mainStreamIndex;
  }
  [[nodiscard]] const std::vector<Track> &getHistory() const noexcept {
    return m_history;
  }

  /**
   * @brief Gets all upcoming tracks (manual queue followed by main stream).
   */
  [[nodiscard]] std::vector<Track> getUpcomingQueue() const;

  /**
   * @brief Number of tracks left in the queue.
   */
  [[nodiscard]] size_t getRemainingCount() const;

  /**
   * @brief Total duration in milliseconds for remaining tracks.
   */
  [[nodiscard]] uint64_t getRemainingDurationMs() const;

  /**
   * @brief Exports current and upcoming tracks as a playlist.
   */
  [[nodiscard]] std::vector<Track> exportQueueAsPlaylist() const;

  /** @brief Clears playback history. */
  void clearHistory();

  /** @brief Sets maximum number of tracks kept in history. */
  void setHistoryLimit(size_t limit);

  /** @brief Gets history retention limit. */
  [[nodiscard]] size_t getHistoryLimit() const noexcept {
    return m_historyLimit;
  }

private:
  void applyShuffle();
  void restoreOriginalOrder();
  void shuffleTracks();
  void shuffleAlbums();

  std::optional<Track> m_currentTrack;
  std::vector<Track> m_upNext;
  std::vector<Track> m_mainStream;
  std::vector<Track> m_originalMainStream;
  size_t m_mainStreamIndex{0};
  std::vector<Track> m_history;
  size_t m_historyLimit{200};

  RepeatMode m_repeatMode{RepeatMode::Off};
  ShuffleMode m_shuffleMode{ShuffleMode::Off};

  std::mt19937 m_rng;
};

} // namespace core
