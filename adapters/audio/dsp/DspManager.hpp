#pragma once

#include <QObject>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "ClapPluginHost.hpp"

namespace adapters {

/**
 * @brief Manages external DSP plugin discovery, auto-creation of user drop-in
 * folders, and the real-time DSP audio processing chain.
 */
class DspManager : public QObject {
  Q_OBJECT

public:
  enum class RoutingOrder {
    EqThenDsp = 0, ///< Parametric EQ -> External DSP Plugins
    DspThenEq = 1  ///< External DSP Plugins -> Parametric EQ
  };
  Q_ENUM(RoutingOrder)

  explicit DspManager(QObject *parent = nullptr);
  ~DspManager() override;

  DspManager(const DspManager &) = delete;
  DspManager &operator=(const DspManager &) = delete;

  /**
   * @brief Initializes search paths, auto-creates runtime directories, and
   * scans plugins.
   * @param extraSearchPaths Optional additional directory paths.
   */
  void initialize(const std::vector<std::string> &extraSearchPaths = {});

  /**
   * @brief Gets the primary writable drop-in DSP directory.
   */
  std::string getPrimaryDspDirectory() const;

  /**
   * @brief Gets all scanned DSP plugin directories.
   */
  std::vector<std::string> getDspDirectories() const;

  /**
   * @brief Scans directories and instantiates newly discovered .clap plugins.
   */
  void scanAndLoadPlugins();

  /**
   * @brief Unloads all active DSP plugins.
   */
  void unloadAllPlugins();

  /**
   * @brief Returns metadata list for all discovered DSP plugins.
   */
  std::vector<ClapPluginMetadata> getDiscoveredPlugins() const;

  /**
   * @brief Toggles enabled state for a plugin.
   */
  bool setPluginEnabled(const std::string &pluginId, bool enabled);

  /**
   * @brief Enables or disables all loaded plugins in the DSP chain.
   */
  void setAllPluginsEnabled(bool enabled);

  /**
   * @brief Requests display of a plugin's native standalone window.
   */
  bool showPluginWindow(const std::string &pluginId);

  /**
   * @brief Requests hiding of a plugin's window.
   */
  bool hidePluginWindow(const std::string &pluginId);

  /**
   * @brief Checks if a plugin window is currently visible.
   */
  bool isPluginWindowVisible(const std::string &pluginId) const;

  /**
   * @brief Master toggle for external DSP plugins processing.
   */
  void setDspMasterEnabled(bool enabled);

  /**
   * @brief Returns true if external DSP chain is active.
   */
  bool isDspMasterEnabled() const;

  /**
   * @brief Sets processing order between EQ and DSP plugins.
   */
  void setRoutingOrder(RoutingOrder order);

  /**
   * @brief Gets active routing order.
   */
  RoutingOrder getRoutingOrder() const;

  /**
   * @brief Processes interleaved 32-bit float PCM audio through the active DSP
   * chain.
   */
  void processDspChain(float *buffer, size_t frames, uint32_t sampleRate,
                       uint8_t channels);

signals:
  void dspPluginsChanged();
  void dspMasterEnabledChanged(bool enabled);
  void routingOrderChanged(int order);

private:
  void ensureRuntimeDirectories();
  void scanDirectory(const std::string &dirPath);
  void rebuildEnabledChainLocked();
  void saveEnabledSettingsLocked();
  ClapPluginHost *getOrCreateHostLocked(const std::string &pluginId);

  mutable std::mutex m_mutex;
  std::string m_primaryDspDir;
  std::vector<std::string> m_searchPaths;
  std::vector<ClapPluginMetadata> m_discoveredPlugins;
  std::unordered_map<std::string, std::unique_ptr<ClapPluginHost>>
      m_loadedInstances;
  struct EnabledChainSnapshot {
    std::array<ClapPluginHost *, 32> hosts{};
    size_t count{0};
  };
  std::array<EnabledChainSnapshot, 2> m_chainSnapshots{};
  std::atomic<uint32_t> m_activeChainIndex{0};
  std::vector<ClapPluginHost *> m_enabledChain;
  std::atomic<bool> m_masterEnabled{true};
  std::atomic<RoutingOrder> m_routingOrder{RoutingOrder::EqThenDsp};
};

} // namespace adapters
