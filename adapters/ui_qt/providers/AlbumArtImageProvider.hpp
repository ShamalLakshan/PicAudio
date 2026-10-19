#pragma once

#include "core/ports/IMetadataExtractor.hpp"
#include "core/ports/IMusicDatabasePort.hpp"
#include <QPainter>
#include <QPixmap>
#include <QQuickImageProvider>
#include <QString>
#include <memory>
#include <mutex>
#include <unordered_map>

/**
 * @brief Clean, high-performance album art provider with disk caching and
 * elegant monochrome fallback.
 */
class AlbumArtImageProvider : public QQuickImageProvider {
public:
  explicit AlbumArtImageProvider(core::IMetadataExtractor *extractor = nullptr,
                                 core::IMusicDatabasePort *dbPort = nullptr,
                                 const QString &cacheDir = QString());
  ~AlbumArtImageProvider() override = default;

  QPixmap requestPixmap(const QString &id, QSize *size,
                        const QSize &requestedSize) override;

  void setExtractor(core::IMetadataExtractor *extractor);
  void setDatabasePort(core::IMusicDatabasePort *dbPort);
  void setCacheDir(const QString &cacheDir);
  void cacheArtwork(const QString &artHash, const QByteArray &data);

private:
  QPixmap generatePlaceholder(const QString &id, int width, int height);
  QString safeDiskFileName(const QString &id) const;

  core::IMetadataExtractor *m_extractor{nullptr};
  core::IMusicDatabasePort *m_dbPort{nullptr};
  QString m_cacheDir;
  mutable std::mutex m_cacheMutex;
  std::unordered_map<std::string, QPixmap> m_pixmapCache;
};
