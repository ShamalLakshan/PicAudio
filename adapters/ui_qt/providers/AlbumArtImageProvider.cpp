#include "AlbumArtImageProvider.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <iostream>

AlbumArtImageProvider::AlbumArtImageProvider(
    core::IMetadataExtractor *extractor, core::IMusicDatabasePort *dbPort,
    const QString &cacheDir)
    : QQuickImageProvider(QQuickImageProvider::Pixmap), m_extractor(extractor),
      m_dbPort(dbPort), m_cacheDir(cacheDir) {
  if (!m_cacheDir.isEmpty()) {
    QDir().mkpath(m_cacheDir);
  }
}

void AlbumArtImageProvider::setExtractor(core::IMetadataExtractor *extractor) {
  m_extractor = extractor;
}

void AlbumArtImageProvider::setDatabasePort(core::IMusicDatabasePort *dbPort) {
  m_dbPort = dbPort;
}

void AlbumArtImageProvider::setCacheDir(const QString &cacheDir) {
  m_cacheDir = cacheDir;
  if (!m_cacheDir.isEmpty()) {
    QDir().mkpath(m_cacheDir);
  }
}

QString AlbumArtImageProvider::safeDiskFileName(const QString &id) const {
  QString safe = id;
  safe.replace('/', '_')
      .replace('\\', '_')
      .replace(':', '_')
      .replace('?', '_')
      .replace('*', '_')
      .replace('"', '_')
      .replace('<', '_')
      .replace('>', '_')
      .replace('|', '_');
  return safe;
}

void AlbumArtImageProvider::cacheArtwork(const QString &artHash,
                                         const QByteArray &data) {
  QPixmap pix;
  if (pix.loadFromData(data)) {
    {
      std::lock_guard<std::mutex> lock(m_cacheMutex);
      m_pixmapCache[artHash.toStdString()] = pix;
    }
    if (!m_cacheDir.isEmpty()) {
      QString outPath = m_cacheDir + "/" + safeDiskFileName(artHash) + ".jpg";
      QFile file(outPath);
      if (file.open(QIODevice::WriteOnly)) {
        file.write(data);
        file.close();
      }
    }
  }
}

QPixmap AlbumArtImageProvider::requestPixmap(const QString &id, QSize *size,
                                             const QSize &requestedSize) {
  int w = requestedSize.width() > 0 ? requestedSize.width() : 300;
  int h = requestedSize.height() > 0 ? requestedSize.height() : 300;

  if (size) {
    *size = QSize(w, h);
  }

  if (id.isEmpty() || id == "default") {
    return generatePlaceholder("default", w, h);
  }

  std::string key = id.toStdString();

  // 1. In-memory cache
  {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    auto it = m_pixmapCache.find(key);
    if (it != m_pixmapCache.end()) {
      return it->second.scaled(w, h, Qt::KeepAspectRatioByExpanding,
                               Qt::SmoothTransformation);
    }
  }

  // 2. Disk cache
  if (!m_cacheDir.isEmpty()) {
    QString safeId = safeDiskFileName(id);
    QString diskPathJpg = m_cacheDir + "/" + safeId + ".jpg";
    QString diskPathPng = m_cacheDir + "/" + safeId + ".png";
    QString path = QFile::exists(diskPathJpg)
                       ? diskPathJpg
                       : (QFile::exists(diskPathPng) ? diskPathPng : "");
    if (!path.isEmpty()) {
      QPixmap pix;
      if (pix.load(path)) {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        m_pixmapCache[key] = pix;
        return pix.scaled(w, h, Qt::KeepAspectRatioByExpanding,
                          Qt::SmoothTransformation);
      }
    }
  }

  // 3. Direct audio file path check
  if (m_extractor && QFile::exists(id)) {
    auto artOpt = m_extractor->extractArtwork(key);
    if (artOpt.has_value() && !artOpt->data.empty()) {
      QPixmap pix;
      if (pix.loadFromData(reinterpret_cast<const uchar *>(artOpt->data.data()),
                           static_cast<uint>(artOpt->data.size()))) {
        {
          std::lock_guard<std::mutex> lock(m_cacheMutex);
          m_pixmapCache[key] = pix;
          if (!artOpt->artHash.empty()) {
            m_pixmapCache[artOpt->artHash] = pix;
          }
        }
        if (!m_cacheDir.isEmpty()) {
          auto writeDisk = [&](const QString &fname) {
            if (fname.isEmpty())
              return;
            QString diskPath =
                m_cacheDir + "/" + safeDiskFileName(fname) + ".jpg";
            if (!QFile::exists(diskPath)) {
              QFile file(diskPath);
              if (file.open(QIODevice::WriteOnly)) {
                file.write(reinterpret_cast<const char *>(artOpt->data.data()),
                           static_cast<qint64>(artOpt->data.size()));
                file.close();
              }
            }
          };
          writeDisk(id);
          if (!artOpt->artHash.empty() &&
              QString::fromStdString(artOpt->artHash) != id) {
            writeDisk(QString::fromStdString(artOpt->artHash));
          }
        }
        return pix.scaled(w, h, Qt::KeepAspectRatioByExpanding,
                          Qt::SmoothTransformation);
      }
    }
  }

  // 4. Extract from database track matching artHash, id, filePath, or albumId
  if (m_dbPort && m_extractor) {
    auto allTracks = m_dbPort->getAllTracks();
    for (const auto &t : allTracks) {
      bool matches = (t.artHash == key || t.id == key || t.filePath == key);
      if (!matches && key.rfind("alb_", 0) == 0) {
        std::string albId = "alb_" + t.album + "_" +
                            (t.albumArtist.empty() ? t.artist : t.albumArtist);
        matches =
            (albId == key || ("alb_" + t.album + "_Unknown Artist") == key ||
             ("alb_" + t.album + "_Various Artists") == key ||
             ("alb_" + t.album + "_") == key || ("alb_" + t.album) == key ||
             ("alb_" + t.album + "_" + t.artist) == key ||
             key.rfind("alb_" + t.album + "_", 0) == 0);
      }

      if (matches) {
        auto artOpt = m_extractor->extractArtwork(t.filePath);
        if (artOpt.has_value() && !artOpt->data.empty()) {
          QPixmap pix;
          if (pix.loadFromData(
                  reinterpret_cast<const uchar *>(artOpt->data.data()),
                  static_cast<uint>(artOpt->data.size()))) {
            {
              std::lock_guard<std::mutex> lock(m_cacheMutex);
              m_pixmapCache[key] = pix;
              if (!artOpt->artHash.empty()) {
                m_pixmapCache[artOpt->artHash] = pix;
              }
            }
            if (!m_cacheDir.isEmpty()) {
              auto writeDisk = [&](const QString &fname) {
                if (fname.isEmpty())
                  return;
                QString diskPath =
                    m_cacheDir + "/" + safeDiskFileName(fname) + ".jpg";
                if (!QFile::exists(diskPath)) {
                  QFile file(diskPath);
                  if (file.open(QIODevice::WriteOnly)) {
                    file.write(
                        reinterpret_cast<const char *>(artOpt->data.data()),
                        static_cast<qint64>(artOpt->data.size()));
                    file.close();
                  }
                }
              };
              writeDisk(id);
              if (!artOpt->artHash.empty() &&
                  QString::fromStdString(artOpt->artHash) != id) {
                writeDisk(QString::fromStdString(artOpt->artHash));
              }
            }
            return pix.scaled(w, h, Qt::KeepAspectRatioByExpanding,
                              Qt::SmoothTransformation);
          }
        }
      }
    }
  }

  // 5. Placeholder fallback
  return generatePlaceholder(id, w, h);
}

QPixmap AlbumArtImageProvider::generatePlaceholder(const QString &id, int width,
                                                   int height) {
  QPixmap pix(width, height);
  pix.fill(QColor(24, 24, 28)); // Solid dark slate

  QPainter p(&pix);
  p.setRenderHint(QPainter::Antialiasing);
  p.setRenderHint(QPainter::TextAntialiasing);

  // Subtle 1px inner border
  p.setPen(QPen(QColor(42, 42, 50), 1));
  p.setBrush(Qt::NoBrush);
  p.drawRect(0, 0, width - 1, height - 1);

  int cx = width / 2;
  int cy = height / 2;

  // Crisp monochrome vinyl / disc motif (solid tones, no gradients)
  int radius = qMin(width, height) / 3;
  if (radius > 15) {
    p.setPen(QPen(QColor(36, 36, 44), 1.5));
    p.setBrush(QColor(20, 20, 24));
    p.drawEllipse(QPoint(cx, cy - 8), radius, radius);

    p.setPen(QPen(QColor(48, 48, 58), 1));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QPoint(cx, cy - 8), radius - 8, radius - 8);
    p.drawEllipse(QPoint(cx, cy - 8), radius - 16, radius - 16);

    // Center hub
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(60, 60, 72));
    p.drawEllipse(QPoint(cx, cy - 8), 10, 10);

    // Spindle hole
    p.setBrush(QColor(24, 24, 28));
    p.drawEllipse(QPoint(cx, cy - 8), 4, 4);
  }

  // Clean, understated typography
  p.setPen(QColor(110, 110, 125));
  QFont font("Inter", 8, QFont::DemiBold);
  font.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
  p.setFont(font);
  p.drawText(QRect(8, height - 26, width - 16, 18), Qt::AlignCenter,
             "PICAUDIO");

  return pix;
}
