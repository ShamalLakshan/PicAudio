#pragma once

#include <string>

namespace adapters {

/**
 * @brief Thread scheduling policy for audio rendering.
 */
enum class AudioPriorityMode {
  Normal,
  RealtimeRoundRobin, ///< SCHED_RR
  RealtimeFifo        ///< SCHED_FIFO
};

/**
 * @brief Result of audio thread priority configuration.
 */
struct AudioPriorityResult {
  bool success{false};
  AudioPriorityMode activeMode{AudioPriorityMode::Normal};
  int activePriority{0};
  std::string message;
};

/**
 * @brief Elevates the calling thread to real-time priority if permitted.
 * @param mode Desired scheduling policy (RealtimeRoundRobin or RealtimeFifo).
 * @param priority Real-time priority level (typically 50-80).
 * @return Struct with applied priority status and diagnostic message.
 */
AudioPriorityResult configureAudioThreadPriority(
    AudioPriorityMode mode = AudioPriorityMode::RealtimeRoundRobin,
    int priority = 70);

/**
 * @brief Queries current scheduling policy and priority of the calling thread.
 */
AudioPriorityResult getCurrentAudioThreadPriority();

} // namespace adapters
