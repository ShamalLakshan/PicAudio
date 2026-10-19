#include "core/services/PlayerService.hpp"
#include <iostream>

namespace core {

PlayerService::PlayerService(std::shared_ptr<IAudioEnginePort> audioEngine)
    : m_audioEngine(std::move(audioEngine)) {}

void PlayerService::setAudioEngine(
    std::shared_ptr<IAudioEnginePort> audioEngine) {
  m_audioEngine = std::move(audioEngine);
}

void PlayerService::play(const Track &track) {
  m_currentTrack = track;
  m_queueService.playNow(track);
  m_state = PlaybackState::Playing;

  if (m_audioEngine) {
    m_audioEngine->load(track.filePath);
    m_audioEngine->play();
  }

  if (m_trackChangedCallback) {
    m_trackChangedCallback(track);
  }
  if (m_stateChangedCallback) {
    m_stateChangedCallback(m_state);
  }
}

void PlayerService::playQueue(const std::vector<Track> &tracks,
                              size_t startIndex) {
  if (tracks.empty())
    return;
  size_t idx = (startIndex < tracks.size()) ? startIndex : 0;
  m_currentTrack = tracks[idx];
  m_queueService.playNow(tracks[idx], tracks, idx);
  m_state = PlaybackState::Playing;

  if (m_audioEngine) {
    m_audioEngine->load(m_currentTrack->filePath);
    m_audioEngine->play();
  }

  if (m_trackChangedCallback) {
    m_trackChangedCallback(*m_currentTrack);
  }
  if (m_stateChangedCallback) {
    m_stateChangedCallback(m_state);
  }
}

void PlayerService::restoreSession(const std::optional<Track> &currentTrack,
                                   uint64_t positionMs,
                                   const std::vector<Track> &upNext,
                                   const std::vector<Track> &mainStream,
                                   size_t mainStreamIndex,
                                   const std::vector<Track> &history,
                                   bool autoResume) {
  m_queueService.restoreState(currentTrack, upNext, mainStream, mainStreamIndex,
                              history);
  m_currentTrack = currentTrack;

  if (currentTrack.has_value()) {
    if (m_audioEngine) {
      m_audioEngine->load(currentTrack->filePath);
      if (positionMs > 0) {
        m_audioEngine->seek(positionMs);
      }
      if (autoResume) {
        m_audioEngine->play();
        m_state = PlaybackState::Playing;
      } else {
        m_audioEngine->pause();
        m_state = PlaybackState::Paused;
      }
    } else {
      m_state = autoResume ? PlaybackState::Playing : PlaybackState::Paused;
    }

    if (m_trackChangedCallback) {
      m_trackChangedCallback(*currentTrack);
    }
  } else {
    m_state = PlaybackState::Stopped;
  }

  if (m_stateChangedCallback) {
    m_stateChangedCallback(m_state);
  }
}

void PlayerService::pause() {
  if (m_state == PlaybackState::Playing) {
    m_state = PlaybackState::Paused;
    if (m_audioEngine) {
      m_audioEngine->pause();
    }
    if (m_stateChangedCallback) {
      m_stateChangedCallback(m_state);
    }
  }
}

void PlayerService::resume() {
  if (m_state == PlaybackState::Paused) {
    m_state = PlaybackState::Playing;
    if (m_audioEngine) {
      m_audioEngine->play();
    }
    if (m_stateChangedCallback) {
      m_stateChangedCallback(m_state);
    }
  } else if (m_state == PlaybackState::Stopped && m_currentTrack.has_value()) {
    play(m_currentTrack.value());
  }
}

void PlayerService::togglePlayPause() {
  if (m_state == PlaybackState::Playing) {
    pause();
  } else {
    resume();
  }
}

void PlayerService::stop() {
  m_state = PlaybackState::Stopped;
  if (m_audioEngine) {
    m_audioEngine->stop();
  }
  if (m_stateChangedCallback) {
    m_stateChangedCallback(m_state);
  }
}

void PlayerService::next() {
  auto nextTrackOpt = m_queueService.next();
  if (nextTrackOpt.has_value()) {
    m_currentTrack = *nextTrackOpt;
    m_state = PlaybackState::Playing;
    if (m_audioEngine) {
      m_audioEngine->load(m_currentTrack->filePath);
      m_audioEngine->play();
    }
    if (m_trackChangedCallback) {
      m_trackChangedCallback(*m_currentTrack);
    }
    if (m_stateChangedCallback) {
      m_stateChangedCallback(m_state);
    }
  } else {
    stop();
  }
}

void PlayerService::previous() {
  auto prevTrackOpt = m_queueService.previous();
  if (prevTrackOpt.has_value()) {
    m_currentTrack = *prevTrackOpt;
    m_state = PlaybackState::Playing;
    if (m_audioEngine) {
      m_audioEngine->load(m_currentTrack->filePath);
      m_audioEngine->play();
    }
    if (m_trackChangedCallback) {
      m_trackChangedCallback(*m_currentTrack);
    }
    if (m_stateChangedCallback) {
      m_stateChangedCallback(m_state);
    }
  }
}

void PlayerService::seek(uint64_t positionMs) {
  if (m_audioEngine) {
    m_audioEngine->seek(positionMs);
  }
}

void PlayerService::setVolume(float volume) {
  m_volume = std::clamp(volume, 0.0f, 1.0f);
  if (m_audioEngine) {
    m_audioEngine->setVolume(m_volume);
  }
}

void PlayerService::playNext(const Track &track) {
  m_queueService.playNext(track);
}

void PlayerService::playNext(const std::vector<Track> &tracks) {
  m_queueService.playNext(tracks);
}

void PlayerService::queueLast(const Track &track) {
  m_queueService.queueLast(track);
}

void PlayerService::queueLast(const std::vector<Track> &tracks) {
  m_queueService.queueLast(tracks);
}

bool PlayerService::removeFromQueue(size_t index) {
  return m_queueService.removeFromQueue(index);
}

bool PlayerService::moveQueueItem(size_t fromIndex, size_t toIndex) {
  return m_queueService.moveQueueItem(fromIndex, toIndex);
}

void PlayerService::clearQueue() { m_queueService.clearQueue(); }

void PlayerService::shuffleRemaining() { m_queueService.shuffleRemaining(); }

void PlayerService::setRepeatMode(RepeatMode mode) {
  m_queueService.setRepeatMode(mode);
}

void PlayerService::cycleRepeatMode() { m_queueService.cycleRepeatMode(); }

void PlayerService::setShuffleMode(ShuffleMode mode) {
  m_queueService.setShuffleMode(mode);
}

uint64_t PlayerService::getPositionMs() const {
  if (m_audioEngine) {
    return m_audioEngine->getPositionMs();
  }
  return 0;
}

uint64_t PlayerService::getDurationMs() const {
  if (m_audioEngine) {
    return m_audioEngine->getDurationMs();
  }
  if (m_currentTrack.has_value()) {
    return m_currentTrack->durationMs;
  }
  return 0;
}

} // namespace core
