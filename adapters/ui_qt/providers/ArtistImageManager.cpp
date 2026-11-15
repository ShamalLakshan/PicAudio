#include "ArtistImageManager.hpp"
#include "DeezerArtistImageService.hpp"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QThread>

namespace adapters {

ArtistImageManager::ArtistImageManager(const QString &cacheDir, QObject *parent)
    : QObject(parent), m_cacheDir(cacheDir) {
  if (m_cacheDir.isEmpty()) {
    m_cacheDir =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
        "/artists";
  }
  QDir().mkpath(m_cacheDir);

  // Register Deezer as default image provider
  registerService(std::make_shared<DeezerArtistImageService>());
}

void ArtistImageManager::registerService(
    std::shared_ptr<IArtistImageService> service) {
  if (service) {
    m_services.push_back(std::move(service));
  }
}

QStringList ArtistImageManager::registeredServiceNames() const {
  QStringList names;
  for (const auto &svc : m_services) {
    if (svc) {
      names.append(svc->serviceName());
    }
  }
  return names;
}

bool ArtistImageManager::hasCachedImage(const QString &artistName) const {
  return !getCachedDiskPath(artistName).isEmpty();
}

QString ArtistImageManager::getCachedDiskPath(const QString &artistName) const {
  if (artistName.trimmed().isEmpty() || m_cacheDir.isEmpty()) {
    return QString();
  }

  QString safeId = safeDiskFileName(artistName);
  QString jpgPath = m_cacheDir + "/" + safeId + ".jpg";
  if (QFile::exists(jpgPath)) {
    return jpgPath;
  }
  QString pngPath = m_cacheDir + "/" + safeId + ".png";
  if (QFile::exists(pngPath)) {
    return pngPath;
  }
  return QString();
}

std::optional<QPixmap>
ArtistImageManager::getCachedPixmap(const QString &artistName) const {
  std::string key = artistName.trimmed().toLower().toStdString();
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_memoryCache.find(key);
    if (it != m_memoryCache.end()) {
      return it->second;
    }
  }

  QString diskPath = getCachedDiskPath(artistName);
  if (!diskPath.isEmpty()) {
    QPixmap pix;
    if (pix.load(diskPath)) {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_memoryCache[key] = pix;
      return pix;
    }
  }

  return std::nullopt;
}

void ArtistImageManager::cacheArtistImage(const QString &artistName,
                                          const QByteArray &data) {
  if (artistName.trimmed().isEmpty() || data.isEmpty()) {
    return;
  }

  QPixmap pix;
  if (!pix.loadFromData(data)) {
    return;
  }

  std::string key = artistName.trimmed().toLower().toStdString();
  QString safeId = safeDiskFileName(artistName);
  QString diskPath = m_cacheDir + "/" + safeId + ".jpg";

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_memoryCache[key] = pix;
    m_negativeLookupCache.erase(key);
  }

  if (!m_cacheDir.isEmpty()) {
    QFile file(diskPath);
    if (file.open(QIODevice::WriteOnly)) {
      file.write(data);
      file.close();
    }
  }

  emit artistImageReady(artistName, diskPath);
}

void ArtistImageManager::requestArtistImage(
    const QString &artistName,
    std::function<void(bool, const QString &)> callback) {
  if (QThread::currentThread() != thread()) {
    QMetaObject::invokeMethod(
        this,
        [this, artistName, callback]() {
          requestArtistImage(artistName, callback);
        },
        Qt::QueuedConnection);
    return;
  }

  QString cleanArtist = artistName.trimmed();
  if (cleanArtist.isEmpty() ||
      cleanArtist.compare("Unknown Artist", Qt::CaseInsensitive) == 0 ||
      cleanArtist.compare("Unknown", Qt::CaseInsensitive) == 0) {
    if (callback) {
      callback(false, QString());
    }
    return;
  }

  // 1. Check existing disk cache
  QString diskPath = getCachedDiskPath(cleanArtist);
  if (!diskPath.isEmpty()) {
    if (callback) {
      callback(true, diskPath);
    }
    emit artistImageReady(cleanArtist, diskPath);
    return;
  }

  if (!m_onlineFetchingEnabled) {
    if (callback) {
      callback(false, QString());
    }
    return;
  }

  std::string key = cleanArtist.toLower().toStdString();
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_negativeLookupCache.find(key) != m_negativeLookupCache.end()) {
      if (callback) {
        callback(false, QString());
      }
      return;
    }

    auto inFlightIt = m_inFlightRequests.find(key);
    if (inFlightIt != m_inFlightRequests.end()) {
      if (callback) {
        inFlightIt->second.push_back(callback);
      }
      return;
    }

    m_inFlightRequests[key] = {};
    if (callback) {
      m_inFlightRequests[key].push_back(callback);
    }
  }

  // 2. Begin provider service pipeline
  tryService(cleanArtist, 0);
}

void ArtistImageManager::tryService(const QString &artistName,
                                    size_t serviceIndex) {
  std::string key = artistName.trimmed().toLower().toStdString();

  if (serviceIndex >= m_services.size()) {
    // All providers exhausted with no match
    std::vector<std::function<void(bool, const QString &)>> pendingCallbacks;
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_negativeLookupCache.insert(key);
      auto it = m_inFlightRequests.find(key);
      if (it != m_inFlightRequests.end()) {
        pendingCallbacks = std::move(it->second);
        m_inFlightRequests.erase(it);
      }
    }
    for (const auto &cb : pendingCallbacks) {
      if (cb) {
        cb(false, QString());
      }
    }
    return;
  }

  auto &service = m_services[serviceIndex];
  if (!service) {
    tryService(artistName, serviceIndex + 1);
    return;
  }

  service->fetchArtistImage(artistName, [this, artistName, serviceIndex,
                                         key](bool success,
                                              const QString & /*imageUrl*/,
                                              const QByteArray &data) {
    if (success && !data.isEmpty()) {
      QPixmap pix;
      if (pix.loadFromData(data)) {
        QString safeId = safeDiskFileName(artistName);
        QString diskPath = m_cacheDir + "/" + safeId + ".jpg";

        QFile file(diskPath);
        if (file.open(QIODevice::WriteOnly)) {
          file.write(data);
          file.close();
        }

        std::vector<std::function<void(bool, const QString &)>>
            pendingCallbacks;
        {
          std::lock_guard<std::mutex> lock(m_mutex);
          m_memoryCache[key] = pix;
          m_negativeLookupCache.erase(key);
          auto it = m_inFlightRequests.find(key);
          if (it != m_inFlightRequests.end()) {
            pendingCallbacks = std::move(it->second);
            m_inFlightRequests.erase(it);
          }
        }

        for (const auto &cb : pendingCallbacks) {
          if (cb) {
            cb(true, diskPath);
          }
        }

        emit artistImageReady(artistName, diskPath);
        return;
      }
    }

    // Try next service in chain
    tryService(artistName, serviceIndex + 1);
  });
}

bool ArtistImageManager::isOnlineFetchingEnabled() const {
  return m_onlineFetchingEnabled;
}

void ArtistImageManager::setOnlineFetchingEnabled(bool enabled) {
  m_onlineFetchingEnabled = enabled;
}

void ArtistImageManager::setCacheDir(const QString &dir) {
  m_cacheDir = dir;
  if (!m_cacheDir.isEmpty()) {
    QDir().mkpath(m_cacheDir);
  }
}

void ArtistImageManager::clearCache() {
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_memoryCache.clear();
    m_negativeLookupCache.clear();
  }

  if (!m_cacheDir.isEmpty()) {
    QDir dir(m_cacheDir);
    const auto files = dir.entryInfoList({"*.jpg", "*.png", "*.jpeg"},
                                         QDir::Files | QDir::NoDotAndDotDot);
    for (const auto &fi : files) {
      QFile::remove(fi.absoluteFilePath());
    }
  }
}

QString ArtistImageManager::safeDiskFileName(const QString &artistName) {
  QString safe = artistName.trimmed();
  safe.replace('/', '_')
      .replace('\\', '_')
      .replace(':', '_')
      .replace('?', '_')
      .replace('*', '_')
      .replace('"', '_')
      .replace('<', '_')
      .replace('>', '_')
      .replace('|', '_');

  if (safe.length() > 80) {
    QByteArray hash =
        QCryptographicHash::hash(artistName.toUtf8(), QCryptographicHash::Sha1)
            .toHex();
    safe = safe.left(50) + "_" + QString::fromLatin1(hash.left(10));
  }
  return safe;
}

} // namespace adapters
