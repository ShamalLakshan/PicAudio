#pragma once

#include "Defines.hpp"

namespace picaudio::plugin {

/**
 * @brief Base interface that all dynamic PicAudio plugins must implement.
 */
class IPicAudioPlugin {
public:
  virtual ~IPicAudioPlugin() = default;

  /**
   * @brief Unique identifier of the plugin (e.g. "com.picaudio.spectrum").
   */
  [[nodiscard]] virtual const char *getId() const noexcept = 0;

  /**
   * @brief User-friendly display name of the plugin.
   */
  [[nodiscard]] virtual const char *getName() const noexcept = 0;

  /**
   * @brief Semantic version string (e.g. "1.0.0").
   */
  [[nodiscard]] virtual const char *getVersion() const noexcept = 0;

  /**
   * @brief Author or organization name.
   */
  [[nodiscard]] virtual const char *getAuthor() const noexcept = 0;

  /**
   * @brief Brief description of plugin capabilities.
   */
  [[nodiscard]] virtual const char *getDescription() const noexcept = 0;

  /**
   * @brief SDK API version targeted by this plugin.
   */
  [[nodiscard]] virtual uint32_t getApiVersion() const noexcept {
    return PICAUDIO_PLUGIN_API_VERSION;
  }

  /**
   * @brief Bitmask combination of PicAudioCapability flags.
   */
  [[nodiscard]] virtual uint32_t getCapabilities() const noexcept = 0;

  /**
   * @brief Initialize plugin resources before activation.
   * @return true if initialization succeeded, false otherwise.
   */
  virtual bool initialize() = 0;

  /**
   * @brief Start plugin background processing or worker threads.
   */
  virtual void start() {}

  /**
   * @brief Stop plugin processing.
   */
  virtual void stop() {}

  /**
   * @brief Release all plugin resources and prepare for unload.
   */
  virtual void shutdown() = 0;

  /**
   * @brief Query capability interface pointer (e.g. IVisualizerPlugin*).
   * @param capability Single PicAudioCapability flag.
   * @return Raw pointer to capability interface or nullptr if not supported.
   */
  [[nodiscard]] virtual void *getCapabilityInterface(uint32_t capability) {
    (void)capability;
    return nullptr;
  }
};

using IParakeetPlugin = IPicAudioPlugin;

} // namespace picaudio::plugin

#define PICAUDIO_PLUGIN_ENTRY_CREATE "create_plugin"
#define PICAUDIO_PLUGIN_ENTRY_DESTROY "destroy_plugin"

extern "C" {
/**
 * @brief Factory function signature for instantiating the plugin.
 */
typedef picaudio::plugin::IPicAudioPlugin *(*PicAudioCreatePluginFunc)();

/**
 * @brief Cleanup function signature for destroying the plugin instance.
 */
typedef void (*PicAudioDestroyPluginFunc)(picaudio::plugin::IPicAudioPlugin *);
}

namespace picaudio::plugin {
using PicAudioCreatePluginFunc = ::PicAudioCreatePluginFunc;
using PicAudioDestroyPluginFunc = ::PicAudioDestroyPluginFunc;
} // namespace picaudio::plugin
