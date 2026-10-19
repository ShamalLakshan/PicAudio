#include "core/services/QueueService.hpp"
#include <map>

namespace core {

QueueService::QueueService() : m_rng(std::random_device{}()) {}

QueueService::QueueService(uint32_t randomSeed) : m_rng(randomSeed) {}

void QueueService::playNow(const Track &track,
                           const std::vector<Track> &contextTracks,
                           size_t trackIndex) {
  if (m_currentTrack.has_value()) {
    m_history.push_back(*m_currentTrack);
    while (m_history.size() > m_historyLimit) {
      m_history.erase(m_history.begin());
    }
  }
  m_currentTrack = track;
  m_upNext.clear();

  if (!contextTracks.empty()) {
    m_originalMainStream = contextTracks;
    m_mainStream = contextTracks;
    m_mainStreamIndex = (trackIndex < m_mainStream.size()) ? trackIndex : 0;
  } else {
    m_originalMainStream = {track};
    m_mainStream = {track};
    m_mainStreamIndex = 0;
  }

  if (m_shuffleMode == ShuffleMode::Tracks) {
    shuffleTracks();
  } else if (m_shuffleMode == ShuffleMode::Albums) {
    shuffleAlbums();
  }
}

void QueueService::playNext(const Track &track) {
  m_upNext.insert(m_upNext.begin(), track);
}

void QueueService::playNext(const std::vector<Track> &tracks) {
  m_upNext.insert(m_upNext.begin(), tracks.begin(), tracks.end());
}

void QueueService::queueLast(const Track &track) { m_upNext.push_back(track); }

void QueueService::queueLast(const std::vector<Track> &tracks) {
  m_upNext.insert(m_upNext.end(), tracks.begin(), tracks.end());
}

std::optional<Track> QueueService::next() {
  if (m_repeatMode == RepeatMode::One && m_currentTrack.has_value()) {
    return m_currentTrack;
  }

  if (m_currentTrack.has_value()) {
    m_history.push_back(*m_currentTrack);
    while (m_history.size() > m_historyLimit) {
      m_history.erase(m_history.begin());
    }
  }

  // Check manual queue first
  if (!m_upNext.empty()) {
    Track nextTrack = m_upNext.front();
    m_upNext.erase(m_upNext.begin());
    m_currentTrack = nextTrack;
    return nextTrack;
  }

  // Advance main stream
  if (m_mainStreamIndex + 1 < m_mainStream.size()) {
    m_mainStreamIndex++;
    m_currentTrack = m_mainStream[m_mainStreamIndex];
    return m_currentTrack;
  }

  // Handle repeat all
  if (m_repeatMode == RepeatMode::All && !m_mainStream.empty()) {
    if (m_shuffleMode == ShuffleMode::Tracks) {
      std::shuffle(m_mainStream.begin(), m_mainStream.end(), m_rng);
    } else if (m_shuffleMode == ShuffleMode::Albums) {
      m_mainStreamIndex = 0;
      shuffleAlbums();
    }
    m_mainStreamIndex = 0;
    m_currentTrack = m_mainStream[0];
    return m_currentTrack;
  }

  // Nothing left to play
  m_currentTrack = std::nullopt;
  return std::nullopt;
}

std::optional<Track> QueueService::previous() {
  // Check history stack
  if (!m_history.empty()) {
    Track prevTrack = m_history.back();
    m_history.pop_back();

    // Put current track back in queue
    if (m_currentTrack.has_value()) {
      m_upNext.insert(m_upNext.begin(), *m_currentTrack);
    }

    m_currentTrack = prevTrack;

    // Sync stream index if track is in stream
    for (size_t i = 0; i < m_mainStream.size(); ++i) {
      if (m_mainStream[i].id == prevTrack.id) {
        m_mainStreamIndex = i;
        break;
      }
    }
    return m_currentTrack;
  }

  // Step back in stream
  if (m_mainStreamIndex > 0) {
    if (m_currentTrack.has_value()) {
      m_upNext.insert(m_upNext.begin(), *m_currentTrack);
    }
    m_mainStreamIndex--;
    m_currentTrack = m_mainStream[m_mainStreamIndex];
    return m_currentTrack;
  }

  // Wrap around for repeat all
  if (m_repeatMode == RepeatMode::All && !m_mainStream.empty()) {
    m_mainStreamIndex = m_mainStream.size() - 1;
    m_currentTrack = m_mainStream[m_mainStreamIndex];
    return m_currentTrack;
  }

  return m_currentTrack;
}

bool QueueService::removeFromQueue(size_t index) {
  if (index < m_upNext.size()) {
    m_upNext.erase(m_upNext.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
  }

  size_t streamUpcomingIndex = index - m_upNext.size();
  size_t streamTarget = m_mainStreamIndex + 1 + streamUpcomingIndex;
  if (streamTarget < m_mainStream.size()) {
    m_mainStream.erase(m_mainStream.begin() +
                       static_cast<std::ptrdiff_t>(streamTarget));
    return true;
  }

  return false;
}

bool QueueService::moveQueueItem(size_t fromIndex, size_t toIndex) {
  auto upcoming = getUpcomingQueue();
  if (fromIndex >= upcoming.size() || toIndex >= upcoming.size() ||
      fromIndex == toIndex) {
    return false;
  }

  if (fromIndex < toIndex) {
    std::rotate(upcoming.begin() + fromIndex, upcoming.begin() + fromIndex + 1,
                upcoming.begin() + toIndex + 1);
  } else {
    std::rotate(upcoming.begin() + toIndex, upcoming.begin() + fromIndex,
                upcoming.begin() + fromIndex + 1);
  }

  // Update queue order
  m_upNext = upcoming;
  if (m_mainStreamIndex + 1 < m_mainStream.size()) {
    m_mainStream.erase(m_mainStream.begin() +
                           static_cast<std::ptrdiff_t>(m_mainStreamIndex + 1),
                       m_mainStream.end());
  }
  return true;
}

void QueueService::clearQueue() {
  m_upNext.clear();
  if (m_mainStreamIndex + 1 < m_mainStream.size()) {
    m_mainStream.erase(m_mainStream.begin() +
                           static_cast<std::ptrdiff_t>(m_mainStreamIndex + 1),
                       m_mainStream.end());
  }
}

void QueueService::shuffleRemaining() {
  if (!m_upNext.empty()) {
    std::shuffle(m_upNext.begin(), m_upNext.end(), m_rng);
  }
  if (m_mainStreamIndex + 1 < m_mainStream.size()) {
    std::shuffle(m_mainStream.begin() +
                     static_cast<std::ptrdiff_t>(m_mainStreamIndex + 1),
                 m_mainStream.end(), m_rng);
  }
}

void QueueService::restoreState(const std::optional<Track> &currentTrack,
                                const std::vector<Track> &upNext,
                                const std::vector<Track> &mainStream,
                                size_t mainStreamIndex,
                                const std::vector<Track> &history) {
  m_currentTrack = currentTrack;
  m_upNext = upNext;
  m_mainStream = mainStream;
  m_originalMainStream = mainStream;
  m_mainStreamIndex =
      (!m_mainStream.empty() && mainStreamIndex < m_mainStream.size())
          ? mainStreamIndex
          : 0;
  m_history = history;
  while (m_history.size() > m_historyLimit) {
    m_history.erase(m_history.begin());
  }
}

void QueueService::setRepeatMode(RepeatMode mode) { m_repeatMode = mode; }

void QueueService::cycleRepeatMode() {
  switch (m_repeatMode) {
  case RepeatMode::Off:
    m_repeatMode = RepeatMode::All;
    break;
  case RepeatMode::All:
    m_repeatMode = RepeatMode::One;
    break;
  case RepeatMode::One:
    m_repeatMode = RepeatMode::Off;
    break;
  }
}

void QueueService::setShuffleMode(ShuffleMode mode) {
  if (m_shuffleMode == mode)
    return;

  m_shuffleMode = mode;
  switch (m_shuffleMode) {
  case ShuffleMode::Off:
    restoreOriginalOrder();
    break;
  case ShuffleMode::Tracks:
    shuffleTracks();
    break;
  case ShuffleMode::Albums:
    shuffleAlbums();
    break;
  }
}

void QueueService::restoreOriginalOrder() {
  if (m_originalMainStream.empty())
    return;

  // Keep current track position
  std::string currentId = m_currentTrack.has_value() ? m_currentTrack->id : "";

  m_mainStream = m_originalMainStream;
  m_mainStreamIndex = 0;

  if (!currentId.empty()) {
    for (size_t i = 0; i < m_mainStream.size(); ++i) {
      if (m_mainStream[i].id == currentId) {
        m_mainStreamIndex = i;
        break;
      }
    }
  }
}

void QueueService::shuffleTracks() {
  if (m_mainStream.size() <= m_mainStreamIndex + 1)
    return;

  // Shuffle tracks after current position
  auto startIt =
      m_mainStream.begin() + static_cast<std::ptrdiff_t>(m_mainStreamIndex + 1);
  std::shuffle(startIt, m_mainStream.end(), m_rng);
}

void QueueService::shuffleAlbums() {
  if (m_mainStream.size() <= m_mainStreamIndex + 1)
    return;

  // Remaining tracks after current track
  std::vector<Track> remaining(
      m_mainStream.begin() + static_cast<std::ptrdiff_t>(m_mainStreamIndex + 1),
      m_mainStream.end());
  if (remaining.empty())
    return;

  // Group remaining tracks by album key: artist|album
  std::map<std::string, std::vector<Track>> albumGroups;
  std::vector<std::string> albumKeys;

  for (const auto &t : remaining) {
    std::string key =
        (t.albumArtist.empty() ? t.artist : t.albumArtist) + "|" + t.album;
    if (albumGroups.find(key) == albumGroups.end()) {
      albumKeys.push_back(key);
    }
    albumGroups[key].push_back(t);
  }

  // Sort tracks within each album strictly in track-number order
  for (auto &[key, tracks] : albumGroups) {
    std::sort(tracks.begin(), tracks.end(), [](const Track &a, const Track &b) {
      if (a.discNumber != b.discNumber)
        return a.discNumber < b.discNumber;
      return a.trackNumber < b.trackNumber;
    });
  }

  // Shuffle album order
  std::shuffle(albumKeys.begin(), albumKeys.end(), m_rng);

  // Reconstruct remaining main stream
  m_mainStream.erase(m_mainStream.begin() +
                         static_cast<std::ptrdiff_t>(m_mainStreamIndex + 1),
                     m_mainStream.end());
  for (const auto &key : albumKeys) {
    const auto &albumTracks = albumGroups[key];
    m_mainStream.insert(m_mainStream.end(), albumTracks.begin(),
                        albumTracks.end());
  }
}

std::vector<Track> QueueService::getUpcomingQueue() const {
  std::vector<Track> result;
  result.reserve(m_upNext.size() +
                 (m_mainStream.size() > m_mainStreamIndex + 1
                      ? m_mainStream.size() - (m_mainStreamIndex + 1)
                      : 0));

  // Up next priority tracks
  result.insert(result.end(), m_upNext.begin(), m_upNext.end());

  // Remaining main stream tracks
  if (m_mainStreamIndex + 1 < m_mainStream.size()) {
    result.insert(result.end(),
                  m_mainStream.begin() +
                      static_cast<std::ptrdiff_t>(m_mainStreamIndex + 1),
                  m_mainStream.end());
  }

  return result;
}

size_t QueueService::getRemainingCount() const {
  size_t count = m_upNext.size();
  if (m_mainStreamIndex + 1 < m_mainStream.size()) {
    count += (m_mainStream.size() - (m_mainStreamIndex + 1));
  }
  return count;
}

uint64_t QueueService::getRemainingDurationMs() const {
  uint64_t total = 0;
  for (const auto &t : m_upNext) {
    total += t.durationMs;
  }
  if (m_mainStreamIndex + 1 < m_mainStream.size()) {
    for (size_t i = m_mainStreamIndex + 1; i < m_mainStream.size(); ++i) {
      total += m_mainStream[i].durationMs;
    }
  }
  return total;
}

std::vector<Track> QueueService::exportQueueAsPlaylist() const {
  std::vector<Track> result;
  if (m_currentTrack.has_value()) {
    result.push_back(*m_currentTrack);
  }
  auto upcoming = getUpcomingQueue();
  result.insert(result.end(), upcoming.begin(), upcoming.end());
  return result;
}

void QueueService::clearHistory() { m_history.clear(); }

void QueueService::setHistoryLimit(size_t limit) {
  m_historyLimit = limit;
  while (m_history.size() > m_historyLimit && !m_history.empty()) {
    m_history.erase(m_history.begin());
  }
}

} // namespace core
