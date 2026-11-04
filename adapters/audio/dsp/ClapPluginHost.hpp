#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "clap/clap.h"
#include "clap/ext/gui.h"
#include "clap/ext/thread-check.h"
#include "clap/ext/timer-support.h"

class QTimer;

namespace adapters {

/**
 * @brief Metadata descriptor for a discovered or instantiated CLAP audio
 * plugin.
 */
struct ClapPluginMetadata {
  std::string id;
  std::string name;
  std::string vendor;
  std::string version;
  std::string description;
  std::string libraryPath;
  bool hasGui{false};
  bool enabled{false};
  bool isLoaded{false};
};

/**
 * @brief Host wrapper for an external CLAP (CLever Audio Plugin) dynamic audio
 * processor.
 *
 * Provides real-time PCM audio processing and standalone floating window
 * lifecycle management.
 */
class ClapPluginHost {
public:
  ClapPluginHost();
  ~ClapPluginHost();

  ClapPluginHost(const ClapPluginHost &) = delete;
  ClapPluginHost &operator=(const ClapPluginHost &) = delete;

  /**
   * @brief Enumerates available plugin descriptors inside a .clap shared
   * library.
   * @param libraryPath Path to .clap file.
   * @return List of discovered plugin descriptors.
   */
  static std::vector<ClapPluginMetadata>
  enumeratePlugins(const std::string &libraryPath);

  /**
   * @brief Loads and instantiates a CLAP plugin from a shared library.
   * @param libraryPath Path to .clap file.
   * @param pluginId Specific plugin identifier to instantiate (or empty for the
   * first one).
   * @param sampleRate Initial processing sample rate in Hz.
   * @param maxFrames Maximum expected frame count per block.
   * @return True if initialized and activated successfully.
   */
  bool load(const std::string &libraryPath, const std::string &pluginId = "",
            uint32_t sampleRate = 44100, uint32_t maxFrames = 2048);

  /**
   * @brief Deactivates and unloads the plugin and associated dynamic library.
   */
  void unload();

  /**
   * @brief Returns true if a plugin is currently loaded and active.
   */
  bool isLoaded() const;

  /**
   * @brief Queries descriptor metadata for the loaded plugin.
   */
  ClapPluginMetadata getMetadata() const;

  /**
   * @brief Enables or bypasses processing for this plugin instance.
   */
  void setEnabled(bool enabled);

  /**
   * @brief Returns true if processing is enabled.
   */
  bool isEnabled() const;

  /**
   * @brief Shows the plugin's native standalone floating window.
   */
  bool showWindow();

  /**
   * @brief Hides the plugin's native window.
   */
  bool hideWindow();

  /**
   * @brief Destroys and cleans up GUI resources for the plugin window.
   */
  void destroyGui();

  /**
   * @brief Returns true if the plugin's window is currently displayed.
   */
  bool isWindowVisible() const;

  using WindowVisibilityCallback =
      std::function<void(const std::string &pluginId, bool visible)>;

  void setWindowVisibilityCallback(WindowVisibilityCallback cb) {
    m_windowVisibilityCallback = std::move(cb);
  }

  /**
   * @brief Processes interleaved 32-bit floating point PCM audio frames
   * in-place.
   * @param buffer Interleaved float PCM audio samples.
   * @param frames Number of frames (samples per channel).
   * @param sampleRate Sampling rate in Hz.
   * @param channels Channel count (up to 8).
   */
  void process(float *buffer, size_t frames, uint32_t sampleRate,
               uint8_t channels);

private:
  void *m_libHandle{nullptr};
  const clap_plugin_entry_t *m_entry{nullptr};
  const clap_plugin_t *m_plugin{nullptr};
  const clap_plugin_gui_t *m_gui{nullptr};
  const clap_plugin_timer_support_t *m_timerSupport{nullptr};

  clap_host_t m_clapHost{};
  clap_host_gui_t m_clapHostGui{};
  clap_host_thread_check_t m_clapHostThreadCheck{};
  clap_host_timer_support_t m_clapHostTimerSupport{};

  mutable std::mutex m_guiMutex;
  mutable std::mutex m_audioMutex;
  mutable std::mutex m_metaMutex;

  std::atomic<bool> m_enabled{false};
  std::atomic<bool> m_isLoaded{false};
  std::atomic<bool> m_guiVisible{false};
  std::atomic<bool> m_guiCreated{false};

  ClapPluginMetadata m_meta;
  uint32_t m_sampleRate{44100};
  uint32_t m_maxFrames{2048};

  WindowVisibilityCallback m_windowVisibilityCallback;
  void notifyWindowVisibility(bool visible);

  std::unordered_map<clap_id, std::unique_ptr<QTimer>> m_timers;
  clap_id m_nextTimerId{1};

  // Planar channel buffers for CLAP format
  std::vector<std::vector<float>> m_channelInBuffers;
  std::vector<std::vector<float>> m_channelOutBuffers;
  std::vector<float *> m_channelInPtrs;
  std::vector<float *> m_channelOutPtrs;

  static const void *hostGetExtension(const clap_host_t *host,
                                      const char *extensionId);
  static void hostRequestRestart(const clap_host_t *host);
  static void hostRequestProcess(const clap_host_t *host);
  static void hostRequestCallback(const clap_host_t *host);

  static void hostGuiResizeHintsChanged(const clap_host_t *host);
  static bool hostGuiRequestResize(const clap_host_t *host, uint32_t width,
                                   uint32_t height);
  static bool hostGuiRequestShow(const clap_host_t *host);
  static bool hostGuiRequestHide(const clap_host_t *host);
  static void hostGuiClosed(const clap_host_t *host, bool wasDestroyed);

  static bool hostIsMainThread(const clap_host_t *host);
  static bool hostIsAudioThread(const clap_host_t *host);

  static bool hostRegisterTimer(const clap_host_t *host, uint32_t period_ms,
                                clap_id *timer_id);
  static bool hostUnregisterTimer(const clap_host_t *host, clap_id timer_id);
};

} // namespace adapters
