#pragma once

#include <picaudio/plugin/Defines.hpp>
#include <picaudio/plugin/ILibraryPlugin.hpp>
#include <picaudio/plugin/IPlayerPlugin.hpp>
#include <picaudio/plugin/IPlugin.hpp>
#include <picaudio/plugin/IThemePlugin.hpp>
#include <picaudio/plugin/IUiPlugin.hpp>
#include <picaudio/plugin/IVisualizerPlugin.hpp>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibrary>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace adapters {

class ThemeLoader;

/**
 * @brief Descriptor for a custom action button displayed on a plugin's preferences card.
 */
struct PluginCustomAction {
  QString id;
  QString label;
  QString tooltip;
  QString iconName;
  bool isPrimary{false};
};

/**
 * @brief Descriptor for plugin metadata parsed from plugin.json.
 */
struct PluginInfo {
  QString id;
  QString name;
  QString version;
  QString author;
  QString description;
  QString homepage;
  QString targetApiVersion;
  QStringList capabilities;
  QString entryPoint;
  QString minHostVersion;
  QString directory;
  QString libraryPath;
  QVector<PluginCustomAction> customActions;
  bool enabled{true};
  bool loaded{false};
  QString loadError;
};

/**
 * @brief Dynamic loader and manager for PicAudio C-ABI plugins.
 */
class PluginManager : public QObject {
  Q_OBJECT

public:
  explicit PluginManager(QObject *parent = nullptr);
  ~PluginManager() override;

  /**
   * @brief Initialize plugin manager with custom search paths.
   * @param extraSearchPaths Additional directory paths to scan.
   */
  void initialize(const QStringList &extraSearchPaths = {});

  /**
   * @brief Scan configured directories and load all valid plugins.
   */
  void scanAndLoadPlugins();

  /**
   * @brief Unload all active plugins and release dynamic libraries.
   */
  void unloadAllPlugins();

  /**
   * @brief Retrieve metadata for all discovered plugins.
   */
  [[nodiscard]] QVector<PluginInfo> getDiscoveredPlugins() const;

  /**
   * @brief Enable or disable a specific plugin by id.
   * @param id Plugin unique identifier.
   * @param enabled Desired enabled state.
   * @return true if state changed, false if plugin not found.
   */
  bool setPluginEnabled(const QString &id, bool enabled);

  /**
   * @brief Return list of scanned plugin directories.
   */
  [[nodiscard]] QStringList getPluginDirectories() const;

  /**
   * @brief Primary writable user plugin directory.
   */
  [[nodiscard]] QString getPrimaryPluginDirectory() const;

  // --- Capability Routing Hooks ---

  /**
   * @brief Dispatch audio PCM buffer to all active visualizer plugins.
   */
  void routeAudioBuffer(const float *buffer, size_t frames, uint32_t sampleRate,
                        uint8_t channels);

  /**
   * @brief Query frequency spectrum bands from the primary visualizer plugin.
   * @param outBands Caller buffer.
   * @param maxBands Maximum bands to fetch.
   * @return Number of bands populated.
   */
  size_t getFrequencyBands(float *outBands, size_t maxBands);

  /**
   * @brief Dispatch pre-DSP audio buffer to player plugins.
   */
  void routePreDsp(float *buffer, size_t frames, uint32_t sampleRate,
                   uint8_t channels);

  /**
   * @brief Dispatch post-DSP audio buffer to player plugins.
   */
  void routePostDsp(float *buffer, size_t frames, uint32_t sampleRate,
                    uint8_t channels);

  /**
   * @brief Dispatch track playback start event.
   */
  void routeTrackStarted(const picaudio::plugin::PicAudioTrackInfo &track);

  /**
   * @brief Dispatch playback state transition event.
   */
  void routePlaybackStateChanged(picaudio::plugin::PicAudioPlaybackState state);

  /**
   * @brief Query lyrics from library plugins.
   * @param artist Artist name.
   * @param title Track title.
   * @return Lyrics string if found, empty string otherwise.
   */
  std::string routeFetchLyrics(const std::string &artist,
                               const std::string &title);

  /**
   * @brief Dispatch track scrobble event to scrobbler plugins.
   */
  void routeTrackScrobbled(const picaudio::plugin::PicAudioTrackInfo &track,
                           uint64_t timestamp);

  /**
   * @brief Dispatch track index notification to library plugins.
   */
  void routeTrackIndexed(const picaudio::plugin::PicAudioTrackInfo &track);

  /**
   * @brief Connects theme loader to allow plugins to update color tokens.
   */
  void setThemeLoader(ThemeLoader *themeLoader);

  /**
   * @brief Dispatches base theme change to active theme plugins.
   */
  void routeBaseThemeChanged(const QString &themeId);

  // --- UI Plugin & Custom Actions ---

  /**
   * @brief Trigger a custom action on a loaded plugin.
   * @param pluginId Unique identifier of plugin.
   * @param actionId Action identifier matching PluginCustomAction::id.
   * @return true if handled successfully.
   */
  bool triggerPluginAction(const QString &pluginId, const QString &actionId);

  /**
   * @brief Request a loaded plugin to display its standalone window/dialog.
   * @param pluginId Unique identifier of plugin.
   * @return true if window displayed.
   */
  bool showPluginWindow(const QString &pluginId);

  /**
   * @brief Request a loaded plugin to hide its standalone window.
   * @param pluginId Unique identifier of plugin.
   * @return true if window hidden.
   */
  bool hidePluginWindow(const QString &pluginId);

  /**
   * @brief Query whether a plugin's standalone window is visible.
   */
  [[nodiscard]] bool isPluginWindowVisible(const QString &pluginId) const;

  /**
   * @brief Connects host view navigation callback for UI plugins.
   */
  void setNavigateCallback(std::function<void(int)> cb);

  /**
   * @brief Connects host toast notification callback for UI plugins.
   */
  void setToastCallback(
      std::function<void(const QString &, const QString &)> cb);

signals:
  /**
   * @brief Emitted when plugins are discovered, loaded, or their state toggled.
   */
  void pluginsChanged();

private:
  struct LoadedPluginRecord {
    PluginInfo info;
    std::unique_ptr<QLibrary> library;
    picaudio::plugin::IPicAudioPlugin *instance{nullptr};
    picaudio::plugin::PicAudioDestroyPluginFunc destroyFunc{nullptr};
    picaudio::plugin::IVisualizerPlugin *visualizer{nullptr};
    picaudio::plugin::ILibraryPlugin *libraryPort{nullptr};
    picaudio::plugin::IPlayerPlugin *playerPort{nullptr};
    picaudio::plugin::IThemePlugin *themePort{nullptr};
    picaudio::plugin::IUiPlugin *uiPort{nullptr};
  };

  void applyThemePluginOverrides(picaudio::plugin::IThemePlugin *themePlugin);
  static void
  onPluginThemeUpdate(void *hostContext,
                      const picaudio::plugin::PicAudioColorToken *tokens,
                      size_t count);

  static void hostNavigateToView(void *hostContext, int viewMode);
  static void hostShowToast(void *hostContext, const char *message,
                            const char *level);

  void discoverInDirectory(const QString &dirPath);
  bool loadPluginFromRecord(LoadedPluginRecord &record);
  void unloadPluginRecord(LoadedPluginRecord &record);
  static bool parseManifest(const QString &manifestPath, PluginInfo &info);
  static bool validateApiVersion(const QString &targetVersion,
                                 QString &outError);

  mutable std::mutex m_mutex;
  QStringList m_searchPaths;
  std::vector<LoadedPluginRecord> m_plugins;
  ThemeLoader *m_themeLoader{nullptr};
  std::function<void(int)> m_navigateCallback;
  std::function<void(const QString &, const QString &)> m_toastCallback;
  picaudio::plugin::PicAudioHostUiServices m_hostUiServices;
};

} // namespace adapters
