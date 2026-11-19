#pragma once

#include <QString>
#include <QStringList>

#include <memory>
#include <string>

namespace adapters {

class PluginManager;

/**
 * @brief Result status code for plugin installation operations.
 */
enum class PluginInstallStatus {
  Success,
  FileNotFound,
  InvalidArchive,
  IncompatiblePlatform,
  MissingManifest,
  InvalidManifest,
  IncompatibleApiVersion,
  MissingLibraryBinary,
  ExtractionFailed,
  DestinationWriteFailed,
  PendingRestartRemoval
};

/**
 * @brief Detailed report returned after an installation attempt.
 */
struct PluginInstallResult {
  PluginInstallStatus status{PluginInstallStatus::InvalidArchive};
  QString pluginId;
  QString pluginName;
  QString version;
  QString message;
  bool isUpgrade{false};

  [[nodiscard]] bool isSuccess() const noexcept {
    return status == PluginInstallStatus::Success;
  }
};

/**
 * @brief Handles extraction, packaging validation, installation, and removal
 *        of PicAudio plugin bundles (.paplugin and .zip).
 */
class PluginInstaller {
public:
  /**
   * @brief Current operating system and architecture platform identifier
   *        (e.g., "linux-x64", "windows-x64", "macos-arm64").
   */
  [[nodiscard]] static QString currentPlatformTag();

  /**
   * @brief Install a plugin bundle archive (.paplugin or .zip) into the target
   * directory.
   * @param bundleFilePath Absolute path to the bundle archive on disk.
   * @param destinationPluginsDir Root directory where plugins are stored.
   * @param outResult Populated result structure detailing installation outcome.
   * @return true if installation succeeded, false otherwise.
   */
  static bool installBundle(const QString &bundleFilePath,
                            const QString &destinationPluginsDir,
                            PluginInstallResult &outResult);

  /**
   * @brief Safely remove an installed plugin from disk.
   *        If a library is locked by the OS (common on Windows), stages the
   *        plugin for removal on next application startup.
   * @param pluginId Unique identifier of the plugin.
   * @param pluginDirectory Absolute path to the plugin folder.
   * @param outError Error message if removal fails.
   * @return true if deleted immediately or successfully staged for pending
   * removal.
   */
  static bool removePlugin(const QString &pluginId,
                           const QString &pluginDirectory, QString &outError);

  /**
   * @brief Check whether a plugin is staged for removal on next restart.
   * @param pluginId Unique identifier of the plugin.
   */
  [[nodiscard]] static bool isPendingRemoval(const QString &pluginId);

  /**
   * @brief Retrieve list of plugin IDs pending removal on next restart.
   */
  [[nodiscard]] static QStringList getPendingRemovals();

  /**
   * @brief Process all staged plugin removals.
   *        Should be called once during startup before plugins are loaded.
   * @param pluginsDir Root directory where user plugins are stored.
   * @return Number of plugin directories successfully deleted.
   */
  static int processPendingRemovals(const QString &pluginsDir);

private:
  /**
   * @brief Validate that an archive entry relative path does not attempt path
   * traversal.
   */
  [[nodiscard]] static bool isSafeRelativePath(const QString &relativePath);

  /**
   * @brief Determine if a directory name matches a known platform directory.
   */
  [[nodiscard]] static bool isKnownPlatformDirectory(const QString &dirName);
};

} // namespace adapters
