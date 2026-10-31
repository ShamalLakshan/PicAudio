#pragma once

#include "Defines.hpp"

namespace picaudio::plugin {

/**
 * @brief Function pointer signature for theme update callbacks.
 * @param hostContext Opaque pointer to host application context.
 * @param tokens Array of color tokens to apply.
 * @param count Number of tokens in array.
 */
typedef void (*PicAudioThemeUpdateCallback)(void *hostContext,
                                            const PicAudioColorToken *tokens,
                                            size_t count);

/**
 * @brief Extension interface for plugins customizing or dynamically providing
 * themes (e.g. Pywal palettes, desktop wallpaper color extractors, system
 * accent sync).
 */
class IThemePlugin {
public:
  virtual ~IThemePlugin() = default;

  /**
   * @brief Registers the host callback allowing the plugin to push dynamic
   * color updates at runtime.
   * @param hostContext Opaque pointer passed back in callback invocations.
   * @param callback Function to call when theme colors change.
   */
  virtual void setHostThemeCallback(void *hostContext,
                                    PicAudioThemeUpdateCallback callback) {
    (void)hostContext;
    (void)callback;
  }

  /**
   * @brief Query active theme color overrides provided by this plugin.
   * @param outTokens Caller buffer to populate with key-value color pairs.
   * @param maxTokens Maximum capacity of outTokens.
   * @return Number of tokens written.
   */
  virtual size_t getThemeOverrides(PicAudioColorToken *outTokens,
                                   size_t maxTokens) {
    (void)outTokens;
    (void)maxTokens;
    return 0;
  }

  /**
   * @brief Called when the host switches base themes.
   * @param themeId Base theme identifier.
   */
  virtual void onBaseThemeChanged(const char *themeId) { (void)themeId; }
};

} // namespace picaudio::plugin
