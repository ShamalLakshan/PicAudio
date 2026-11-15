#include "ArtistArtImageProvider.hpp"
#include <QUrl>

namespace adapters {

ArtistArtImageProvider::ArtistArtImageProvider(ArtistImageManager *manager)
    : QQuickImageProvider(QQuickImageProvider::Pixmap), m_manager(manager) {}

void ArtistArtImageProvider::setManager(ArtistImageManager *manager) {
  m_manager = manager;
}

QPixmap ArtistArtImageProvider::requestPixmap(const QString &id, QSize *size,
                                              const QSize &requestedSize) {
  int w = requestedSize.width() > 0 ? requestedSize.width() : 300;
  int h = requestedSize.height() > 0 ? requestedSize.height() : 300;

  if (size) {
    *size = QSize(w, h);
  }

  if (id.trimmed().isEmpty() || id == "default") {
    return QPixmap();
  }

  // Strip query string (e.g. revision buster "?v=1")
  QString rawId = id;
  int qIdx = rawId.indexOf('?');
  if (qIdx != -1) {
    rawId = rawId.left(qIdx);
  }

  // URL-decode artist name
  QString artist = QUrl::fromPercentEncoding(rawId.toUtf8()).trimmed();
  if (artist.isEmpty() ||
      artist.compare("Unknown Artist", Qt::CaseInsensitive) == 0 ||
      artist.compare("Unknown", Qt::CaseInsensitive) == 0) {
    return QPixmap();
  }

  if (m_manager) {
    // 1. Check in-memory / disk cache
    auto cachedOpt = m_manager->getCachedPixmap(artist);
    if (cachedOpt.has_value() && !cachedOpt->isNull()) {
      return cachedOpt->scaled(w, h, Qt::KeepAspectRatioByExpanding,
                               Qt::SmoothTransformation);
    }

    QString diskPath = m_manager->getCachedDiskPath(artist);
    if (!diskPath.isEmpty()) {
      QPixmap pix;
      if (pix.load(diskPath)) {
        return pix.scaled(w, h, Qt::KeepAspectRatioByExpanding,
                          Qt::SmoothTransformation);
      }
    }

    // 2. Not cached; trigger asynchronous discovery pipeline
    m_manager->requestArtistImage(artist);
  }

  return QPixmap();
}

} // namespace adapters
