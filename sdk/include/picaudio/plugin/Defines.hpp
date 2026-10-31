#pragma once

#include <cstddef>
#include <cstdint>

#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(PICAUDIO_PLUGIN_BUILD)
#define PICAUDIO_PLUGIN_EXPORT __declspec(dllexport)
#else
#define PICAUDIO_PLUGIN_EXPORT
#endif
#else
#define PICAUDIO_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

#define PARAKEET_PLUGIN_EXPORT PICAUDIO_PLUGIN_EXPORT

#define PICAUDIO_PLUGIN_API_VERSION_MAJOR 1
#define PICAUDIO_PLUGIN_API_VERSION_MINOR 0
#define PICAUDIO_PLUGIN_API_VERSION_PATCH 0
#define PICAUDIO_PLUGIN_API_VERSION                                            \
  ((PICAUDIO_PLUGIN_API_VERSION_MAJOR << 16) |                                 \
   (PICAUDIO_PLUGIN_API_VERSION_MINOR << 8) |                                  \
   PICAUDIO_PLUGIN_API_VERSION_PATCH)

#define PARAKEET_PLUGIN_API_VERSION PICAUDIO_PLUGIN_API_VERSION

namespace picaudio::plugin {

/**
 * @brief Bitmask capabilities that a plugin can advertise and implement.
 */
enum PicAudioCapability : uint32_t {
  PICAUDIO_CAP_NONE = 0,
  PICAUDIO_CAP_VISUALIZER = 1 << 0,
  PICAUDIO_CAP_LYRICS = 1 << 1,
  PICAUDIO_CAP_SCROBBLER = 1 << 2,
  PICAUDIO_CAP_DSP = 1 << 3,
  PICAUDIO_CAP_LIBRARY = 1 << 4,
  PICAUDIO_CAP_THEME = 1 << 5,
  PICAUDIO_CAP_UI = 1 << 6
};

/**
 * @brief Dynamic color token descriptor for runtime theme customization.
 */
struct PicAudioColorToken {
  const char *key;
  const char *hexColor;
};

/**
 * @brief Custom action button descriptor displayed on a plugin's preferences card.
 */
struct PicAudioCustomAction {
  const char *id;
  const char *label;
  const char *tooltip;
  const char *iconName;
  bool isPrimary;
};

/**
 * @brief Host-provided UI and navigation services accessible by UI plugins.
 */
struct PicAudioHostUiServices {
  void *hostContext;
  void (*navigateToView)(void *hostContext, int viewMode);
  void (*showToast)(void *hostContext, const char *message, const char *level);
};

/**
 * @brief Audio buffer descriptor passed to visualizer and DSP hooks.
 */
struct PicAudioAudioBuffer {
  const float *samples;
  size_t frames;
  uint32_t sampleRate;
  uint8_t channels;
};

/**
 * @brief Plain C-ABI track metadata structure.
 */
struct PicAudioTrackInfo {
  const char *id;
  const char *title;
  const char *artist;
  const char *album;
  const char *albumArtist;
  const char *genre;
  uint32_t year;
  uint32_t trackNumber;
  uint64_t durationMs;
  const char *filePath;
};

/**
 * @brief Audio playback state machine states.
 */
enum class PicAudioPlaybackState : uint32_t {
  Stopped = 0,
  Playing = 1,
  Paused = 2
};

} // namespace picaudio::plugin

namespace parakeet::plugin {
using namespace picaudio::plugin;
}
