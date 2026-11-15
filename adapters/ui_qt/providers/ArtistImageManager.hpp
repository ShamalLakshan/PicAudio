#pragma once

#include "IArtistImageService.hpp"
#include <QByteArray>
#include <QObject>
#include <QPixmap>
#include <QString>
#include <QStringList>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace adapters {

/**
 * @brief Central manager coordinating pluggable artist image discovery
 * services, persistent disk caching, memory caching, and request deduplication.
 */
class ArtistImageManager : public QObject {
  Q_OBJECT

public:
  explicit ArtistImageManager(const QString &cacheDir = QString(),
                              QObject *parent = nullptr);
  ~ArtistImageManager() override = default;

  /** @brief Registers an image discovery service (e.g. Deezer). */
  void registerService(std::shared_ptr<IArtistImageService> service);

  /** @brief List registered provider names. */
  [[nodiscard]] QStringList registeredServiceNames() const;

  /** @brief Checks if an artist image is already cached on disk. */
  [[nodiscard]] bool hasCachedImage(const QString &artistName) const;

  /** @brief Returns local disk file path for an artist image if cached. */
  [[nodiscard]] QString getCachedDiskPath(const QString &artistName) const;

  /** @brief Returns in-memory pixmap if cached. */
  [[nodiscard]] std::optional<QPixmap>
  getCachedPixmap(const QString &artistName) const;

  /** @brief Saves image bytes to disk cache and memory cache for the artist. */
  void cacheArtistImage(const QString &artistName, const QByteArray &data);

  /**
   * @brief Requests an artist image. If cached, invokes callback immediately.
   * If not cached and online fetching is enabled, queries registered services.
   */
  void requestArtistImage(
      const QString &artistName,
      std::function<void(bool success, const QString &diskPath)> callback =
          nullptr);

  /** @brief Enable/disable online fetching of artist images. */
  [[nodiscard]] bool isOnlineFetchingEnabled() const;
  void setOnlineFetchingEnabled(bool enabled);

  /** @brief Set cache directory path. */
  void setCacheDir(const QString &dir);
  [[nodiscard]] QString cacheDir() const { return m_cacheDir; }

  /** @brief Clears all cached artist images on disk and in memory. */
  void clearCache();

  /** @brief Sanitizes artist name into a safe filesystem filename. */
  static QString safeDiskFileName(const QString &artistName);

signals:
  void artistImageReady(const QString &artistName, const QString &diskPath);

private:
  void tryService(const QString &artistName, size_t serviceIndex);

  QString m_cacheDir;
  bool m_onlineFetchingEnabled{true};
  std::vector<std::shared_ptr<IArtistImageService>> m_services;

  mutable std::mutex m_mutex;
  mutable std::unordered_map<std::string, QPixmap> m_memoryCache;
  std::unordered_set<std::string> m_negativeLookupCache;
  std::unordered_map<std::string,
                     std::vector<std::function<void(bool, const QString &)>>>
      m_inFlightRequests;
};

} // namespace adapters
