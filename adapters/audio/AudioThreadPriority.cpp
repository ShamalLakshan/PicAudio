#include "AudioThreadPriority.hpp"

#include <algorithm>

#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#include <cerrno>
#include <cstring>
#include <pthread.h>
#include <sched.h>
#include <sys/resource.h>
#include <sys/time.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

namespace adapters {

AudioPriorityResult configureAudioThreadPriority(AudioPriorityMode mode,
                                                 int priority) {
  AudioPriorityResult result;

#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
  int policy = SCHED_OTHER;
  sched_param param{};

  if (mode == AudioPriorityMode::RealtimeFifo) {
    policy = SCHED_FIFO;
    int minPri = sched_get_priority_min(policy);
    int maxPri = sched_get_priority_max(policy);
    param.sched_priority = std::clamp(priority, minPri, maxPri);
  } else if (mode == AudioPriorityMode::RealtimeRoundRobin) {
    policy = SCHED_RR;
    int minPri = sched_get_priority_min(policy);
    int maxPri = sched_get_priority_max(policy);
    param.sched_priority = std::clamp(priority, minPri, maxPri);
  } else {
    policy = SCHED_OTHER;
    param.sched_priority = 0;
  }

  int ret = pthread_setschedparam(pthread_self(), policy, &param);
  if (ret == 0) {
    result.success = (policy != SCHED_OTHER);
    result.activeMode = mode;
    result.activePriority = param.sched_priority;
    result.message =
        "Successfully set audio thread real-time scheduling priority";
    return result;
  }

  // Fallback to elevated nice level if unprivileged
  int niceRet = setpriority(PRIO_PROCESS, 0, -10);
  result.success = false;
  result.activeMode = AudioPriorityMode::Normal;
  result.activePriority = 0;
  result.message = "Insufficient permissions for real-time scheduling (" +
                   std::string(std::strerror(ret)) +
                   (niceRet == 0 ? "); elevated nice level to -10"
                                 : "); kept normal priority");
  return result;

#elif defined(_WIN32)
  if (mode != AudioPriorityMode::Normal) {
    BOOL ok =
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    if (ok) {
      result.success = true;
      result.activeMode = mode;
      result.activePriority = THREAD_PRIORITY_TIME_CRITICAL;
      result.message = "Set Windows thread priority to TIME_CRITICAL";
      return result;
    }
  }
  result.success = false;
  result.activeMode = AudioPriorityMode::Normal;
  result.activePriority = THREAD_PRIORITY_NORMAL;
  result.message = "Standard Windows thread priority applied";
  return result;

#else
  result.success = false;
  result.activeMode = AudioPriorityMode::Normal;
  result.activePriority = 0;
  result.message =
      "Platform does not support custom real-time audio thread priority";
  return result;
#endif
}

AudioPriorityResult getCurrentAudioThreadPriority() {
  AudioPriorityResult result;

#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
  int policy = 0;
  sched_param param{};
  int ret = pthread_getschedparam(pthread_self(), &policy, &param);
  if (ret == 0) {
    result.activePriority = param.sched_priority;
    if (policy == SCHED_FIFO) {
      result.activeMode = AudioPriorityMode::RealtimeFifo;
      result.success = true;
      result.message = "Active policy: SCHED_FIFO";
    } else if (policy == SCHED_RR) {
      result.activeMode = AudioPriorityMode::RealtimeRoundRobin;
      result.success = true;
      result.message = "Active policy: SCHED_RR";
    } else {
      result.activeMode = AudioPriorityMode::Normal;
      result.success = false;
      result.message = "Active policy: SCHED_OTHER";
    }
  } else {
    result.success = false;
    result.message = "Failed to query thread scheduling parameters";
  }
  return result;

#elif defined(_WIN32)
  int pri = GetThreadPriority(GetCurrentThread());
  result.activePriority = pri;
  if (pri >= THREAD_PRIORITY_HIGHEST) {
    result.activeMode = AudioPriorityMode::RealtimeRoundRobin;
    result.success = true;
    result.message = "Windows high priority thread";
  } else {
    result.activeMode = AudioPriorityMode::Normal;
    result.success = false;
    result.message = "Windows normal priority thread";
  }
  return result;

#else
  result.success = false;
  result.activeMode = AudioPriorityMode::Normal;
  result.message = "Unsupported platform";
  return result;
#endif
}

} // namespace adapters
