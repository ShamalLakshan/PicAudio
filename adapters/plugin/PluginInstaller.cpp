#include "PluginInstaller.hpp"
#include <picaudio/plugin/Defines.hpp>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSettings>
#include <QSysInfo>
#include <QTemporaryDir>

#include <archive.h>
#include <archive_entry.h>

#include <algorithm>
#include <array>
#include <filesystem>

namespace adapters {

namespace {

constexpr qint64 MaxSingleFileSize = 256LL * 1024 * 1024;
constexpr qint64 MaxTotalExtractedSize = 512LL * 1024 * 1024;
constexpr int MaxArchiveEntries = 4096;
const QString PendingRemovalSettingsKey =
    QStringLiteral("plugins/pendingRemoval");

struct ArchiveReadCloser {
  void operator()(archive *a) const noexcept {
    if (a) {
      archive_read_close(a);
      archive_read_free(a);
    }
  }
};
using ArchivePtr = std::unique_ptr<archive, ArchiveReadCloser>;

bool copyDirectoryRecursively(const QString &sourceDir,
                              const QString &destDir) {
  QDir src(sourceDir);
  if (!src.exists()) {
    return false;
  }
  QDir dst(destDir);
  if (!dst.exists()) {
    if (!dst.mkpath(QStringLiteral("."))) {
      return false;
    }
  }

  const QFileInfoList entries =
      src.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
  for (const auto &info : entries) {
    const QString srcPath = info.absoluteFilePath();
    const QString dstPath = dst.filePath(info.fileName());
    if (info.isDir()) {
      if (!copyDirectoryRecursively(srcPath, dstPath)) {
        return false;
      }
    } else {
      if (QFile::exists(dstPath)) {
        QFile::remove(dstPath);
      }
      if (!QFile::copy(srcPath, dstPath)) {
        return false;
      }
    }
  }
  return true;
}

} // namespace

QString PluginInstaller::currentPlatformTag() {
  QString os;
#if defined(Q_OS_WIN)
  os = QStringLiteral("windows");
#elif defined(Q_OS_MACOS)
  os = QStringLiteral("macos");
#elif defined(Q_OS_LINUX)
  os = QStringLiteral("linux");
#elif defined(Q_OS_FREEBSD)
  os = QStringLiteral("freebsd");
#else
  os = QStringLiteral("unknown");
#endif

  const QString cpu = QSysInfo::buildCpuArchitecture().toLower();
  QString arch = QStringLiteral("x64");
  if (cpu == QStringLiteral("arm64") || cpu == QStringLiteral("aarch64")) {
    arch = QStringLiteral("arm64");
  } else if (cpu == QStringLiteral("x86_64") ||
             cpu == QStringLiteral("amd64") || cpu == QStringLiteral("x64")) {
    arch = QStringLiteral("x64");
  }

  return os + QLatin1Char('-') + arch;
}

bool PluginInstaller::isKnownPlatformDirectory(const QString &dirName) {
  static const std::array<QStringView, 8> knownPlatforms = {
      u"linux-x64", u"linux-arm64", u"windows-x64", u"windows-arm64",
      u"macos-x64", u"macos-arm64", u"freebsd-x64", u"freebsd-arm64"};

  return std::ranges::any_of(knownPlatforms, [dirName](QStringView item) {
    return item.compare(dirName, Qt::CaseInsensitive) == 0;
  });
}

bool PluginInstaller::isSafeRelativePath(const QString &relativePath) {
  if (relativePath.isEmpty()) {
    return false;
  }

  QString norm = QDir::fromNativeSeparators(relativePath);
  if (norm.startsWith(QLatin1Char('/')) ||
      norm.startsWith(QLatin1String("//"))) {
    return false;
  }

  static const QRegularExpression driveRegex(QStringLiteral("^[A-Za-z]:"));
  if (driveRegex.match(norm).hasMatch()) {
    return false;
  }

  const QStringList parts = norm.split(QLatin1Char('/'), Qt::KeepEmptyParts);
  for (const auto &part : parts) {
    if (part.isEmpty() || part == QStringLiteral(".") ||
        part == QStringLiteral("..")) {
      return false;
    }
    for (const QChar ch : part) {
      if (ch.unicode() < 0x20 || QStringLiteral("<>:\"|?*").contains(ch)) {
        return false;
      }
    }
  }

  return true;
}

bool PluginInstaller::installBundle(const QString &bundleFilePath,
                                    const QString &destinationPluginsDir,
                                    PluginInstallResult &outResult) {
  outResult = {};

  const QFileInfo bundleInfo(bundleFilePath);
  if (!bundleInfo.exists() || !bundleInfo.isFile()) {
    outResult.status = PluginInstallStatus::FileNotFound;
    outResult.message =
        QStringLiteral("Bundle file does not exist: %1").arg(bundleFilePath);
    return false;
  }

  QTemporaryDir tempDir;
  if (!tempDir.isValid()) {
    outResult.status = PluginInstallStatus::ExtractionFailed;
    outResult.message =
        QStringLiteral("Failed to allocate temporary staging directory.");
    return false;
  }

  const QString currentPlatform = currentPlatformTag();
  const QString currentPlatformPrefix = currentPlatform + QLatin1Char('/');

  ArchivePtr reader(archive_read_new());
  archive_read_support_format_zip(reader.get());
  archive_read_support_filter_none(reader.get());

  const QByteArray encodedPath = QFile::encodeName(bundleFilePath);
  if (archive_read_open_filename(reader.get(), encodedPath.constData(),
                                 10240) != ARCHIVE_OK) {
    outResult.status = PluginInstallStatus::InvalidArchive;
    outResult.message =
        QStringLiteral("Could not read plugin archive: %1")
            .arg(QString::fromUtf8(archive_error_string(reader.get())));
    return false;
  }

  archive_entry *entry = nullptr;
  qint64 totalExtractedBytes = 0;
  int entryCount = 0;
  bool foundMatchingPlatformBinary = false;

  while (archive_read_next_header(reader.get(), &entry) == ARCHIVE_OK) {
    if (++entryCount > MaxArchiveEntries) {
      outResult.status = PluginInstallStatus::InvalidArchive;
      outResult.message =
          QStringLiteral("Archive contains too many entries (> %1).")
              .arg(MaxArchiveEntries);
      return false;
    }

    const char *rawPath = archive_entry_pathname_utf8(entry);
    if (!rawPath) {
      rawPath = archive_entry_pathname(entry);
    }
    if (!rawPath) {
      continue;
    }

    const QString rawEntryPath = QString::fromUtf8(rawPath);
    QString cleanPath = QDir::fromNativeSeparators(rawEntryPath);
    while (cleanPath.startsWith(QLatin1Char('/'))) {
      cleanPath.remove(0, 1);
    }

    const auto fileType = archive_entry_filetype(entry);
    const bool isDir = (fileType == AE_IFDIR);

    if (isDir) {
      continue;
    }

    // Determine target relative path inside temp directory
    QString targetSubPath;
    const QString firstSegment = cleanPath.section(QLatin1Char('/'), 0, 0);

    if (firstSegment.compare(currentPlatform, Qt::CaseInsensitive) == 0) {
      // Platform-specific file for current system: strip prefix
      targetSubPath = cleanPath.section(QLatin1Char('/'), 1);
      foundMatchingPlatformBinary = true;
    } else if (isKnownPlatformDirectory(firstSegment)) {
      // Platform-specific file for another OS/architecture: skip
      archive_read_data_skip(reader.get());
      continue;
    } else {
      // General bundle file (plugin.json, README, assets, etc.)
      targetSubPath = cleanPath;
      if (cleanPath.endsWith(QStringLiteral(".so"), Qt::CaseInsensitive) ||
          cleanPath.endsWith(QStringLiteral(".dll"), Qt::CaseInsensitive) ||
          cleanPath.endsWith(QStringLiteral(".dylib"), Qt::CaseInsensitive)) {
        foundMatchingPlatformBinary = true;
      }
    }

    if (!isSafeRelativePath(targetSubPath)) {
      outResult.status = PluginInstallStatus::InvalidArchive;
      outResult.message =
          QStringLiteral("Malicious or unsafe path in archive: %1")
              .arg(cleanPath);
      return false;
    }

    const la_int64_t declaredSize = archive_entry_size(entry);
    if (declaredSize > MaxSingleFileSize) {
      outResult.status = PluginInstallStatus::InvalidArchive;
      outResult.message =
          QStringLiteral("File exceeds size limit: %1").arg(cleanPath);
      return false;
    }

    const QString destFilePath = tempDir.filePath(targetSubPath);
    QDir().mkpath(QFileInfo(destFilePath).absolutePath());

    QFile outFile(destFilePath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      outResult.status = PluginInstallStatus::ExtractionFailed;
      outResult.message = QStringLiteral("Failed to write extracted file: %1")
                              .arg(destFilePath);
      return false;
    }

    const void *buff = nullptr;
    size_t size = 0;
    la_int64_t offset = 0;

    while (archive_read_data_block(reader.get(), &buff, &size, &offset) ==
           ARCHIVE_OK) {
      totalExtractedBytes += static_cast<qint64>(size);
      if (totalExtractedBytes > MaxTotalExtractedSize) {
        outFile.close();
        outResult.status = PluginInstallStatus::InvalidArchive;
        outResult.message =
            QStringLiteral("Archive exceeds total decompression size limit.");
        return false;
      }
      outFile.write(reinterpret_cast<const char *>(buff),
                    static_cast<qint64>(size));
    }
    outFile.close();
  }

  // Check if plugin.json exists at root or in single top-level folder
  QString manifestPath = tempDir.filePath(QStringLiteral("plugin.json"));
  QString stagingRoot = tempDir.path();

  if (!QFile::exists(manifestPath)) {
    // Check 1 level down
    const QDir tempDirObj(tempDir.path());
    const QFileInfoList subDirs =
        tempDirObj.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const auto &sub : subDirs) {
      const QString candidate =
          sub.absoluteFilePath() + QStringLiteral("/plugin.json");
      if (QFile::exists(candidate)) {
        manifestPath = candidate;
        stagingRoot = sub.absoluteFilePath();
        break;
      }
    }
  }

  if (!QFile::exists(manifestPath)) {
    outResult.status = PluginInstallStatus::MissingManifest;
    outResult.message =
        QStringLiteral("Plugin bundle does not contain plugin.json.");
    return false;
  }

  // Parse and validate plugin.json
  QFile manifestFile(manifestPath);
  if (!manifestFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    outResult.status = PluginInstallStatus::InvalidManifest;
    outResult.message =
        QStringLiteral("Failed to open plugin.json for parsing.");
    return false;
  }

  QJsonParseError parseErr;
  const QJsonDocument doc =
      QJsonDocument::fromJson(manifestFile.readAll(), &parseErr);
  manifestFile.close();

  if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
    outResult.status = PluginInstallStatus::InvalidManifest;
    outResult.message = QStringLiteral("JSON syntax error in plugin.json: %1")
                            .arg(parseErr.errorString());
    return false;
  }

  const QJsonObject obj = doc.object();
  const QString id = obj.value(QStringLiteral("id")).toString().trimmed();
  const QString name = obj.value(QStringLiteral("name")).toString().trimmed();
  const QString version =
      obj.value(QStringLiteral("version")).toString().trimmed();
  const QString targetApi =
      obj.value(QStringLiteral("targetApiVersion")).toString().trimmed();
  const QString entryPoint =
      obj.value(QStringLiteral("entryPoint")).toString().trimmed();

  if (id.isEmpty() || name.isEmpty() || version.isEmpty() ||
      targetApi.isEmpty()) {
    outResult.status = PluginInstallStatus::InvalidManifest;
    outResult.message = QStringLiteral("Manifest missing required fields (id, "
                                       "name, version, targetApiVersion).");
    return false;
  }

  static const QRegularExpression idRegex(QStringLiteral("^[a-zA-Z0-9._-]+$"));
  if (!idRegex.match(id).hasMatch()) {
    outResult.status = PluginInstallStatus::InvalidManifest;
    outResult.message = QStringLiteral("Invalid plugin ID format. Must contain "
                                       "only alphanumeric, dot, or hyphen.");
    return false;
  }

  // Validate API major version
  const QStringList apiParts = targetApi.split(QLatin1Char('.'));
  bool apiOk = false;
  const int apiMajor = apiParts.isEmpty() ? 0 : apiParts[0].toInt(&apiOk);
  if (!apiOk || apiMajor != PICAUDIO_PLUGIN_API_VERSION_MAJOR) {
    outResult.status = PluginInstallStatus::IncompatibleApiVersion;
    outResult.message =
        QStringLiteral("Plugin requires API v%1, but this host runs API v%2.")
            .arg(apiMajor)
            .arg(PICAUDIO_PLUGIN_API_VERSION_MAJOR);
    return false;
  }

  // Verify library binary existence
  bool hasBinary = false;
  if (!entryPoint.isEmpty()) {
    hasBinary = QFile::exists(stagingRoot + QLatin1Char('/') + entryPoint);
  } else {
    const QStringList candidates = {
        stagingRoot + QStringLiteral("/lib%1.so").arg(id),
        stagingRoot + QStringLiteral("/%1.so").arg(id),
        stagingRoot + QStringLiteral("/%1.dll").arg(id),
        stagingRoot + QStringLiteral("/lib%1.dylib").arg(id),
        stagingRoot + QStringLiteral("/plugin.so"),
        stagingRoot + QStringLiteral("/plugin.dll")};
    for (const auto &cand : candidates) {
      if (QFile::exists(cand)) {
        hasBinary = true;
        break;
      }
    }
    if (!hasBinary) {
      // Check any shared library in staging root
      const QFileInfoList libs =
          QDir(stagingRoot)
              .entryInfoList(QStringList{QStringLiteral("*.so"),
                                         QStringLiteral("*.dll"),
                                         QStringLiteral("*.dylib")},
                             QDir::Files);
      hasBinary = !libs.isEmpty();
    }
  }

  if (!hasBinary) {
    outResult.status = PluginInstallStatus::MissingLibraryBinary;
    outResult.message =
        QStringLiteral(
            "Bundle does not contain a compatible binary for platform '%1'.")
            .arg(currentPlatform);
    return false;
  }

  // Prepare final destination
  QDir destParentDir(destinationPluginsDir);
  if (!destParentDir.exists()) {
    if (!destParentDir.mkpath(QStringLiteral("."))) {
      outResult.status = PluginInstallStatus::DestinationWriteFailed;
      outResult.message =
          QStringLiteral("Failed to create plugins root directory: %1")
              .arg(destinationPluginsDir);
      return false;
    }
  }

  const QString finalPluginDir = destParentDir.filePath(id);
  const bool alreadyExists = QDir(finalPluginDir).exists();

  if (alreadyExists) {
    // Overwrite existing folder
    QDir(finalPluginDir).removeRecursively();
  }

  if (!copyDirectoryRecursively(stagingRoot, finalPluginDir)) {
    outResult.status = PluginInstallStatus::DestinationWriteFailed;
    outResult.message =
        QStringLiteral(
            "Failed to copy plugin files to destination directory: %1")
            .arg(finalPluginDir);
    return false;
  }

  // Check if plugin was marked pending removal, clear it on successful install
  QStringList pending = getPendingRemovals();
  if (pending.contains(id)) {
    pending.removeAll(id);
    QSettings settings(QStringLiteral("PicAudio"), QStringLiteral("PicAudio"));
    settings.setValue(PendingRemovalSettingsKey, pending);
  }

  outResult.status = PluginInstallStatus::Success;
  outResult.pluginId = id;
  outResult.pluginName = name;
  outResult.version = version;
  outResult.isUpgrade = alreadyExists;
  outResult.message =
      alreadyExists
          ? QStringLiteral("Plugin '%1' updated to v%2.").arg(name, version)
          : QStringLiteral("Plugin '%1' v%2 installed successfully.")
                .arg(name, version);
  return true;
}

bool PluginInstaller::removePlugin(const QString &pluginId,
                                   const QString &pluginDirectory,
                                   QString &outError) {
  outError.clear();

  if (pluginDirectory.isEmpty() || !QDir(pluginDirectory).exists()) {
    return true;
  }

  QDir dir(pluginDirectory);
  const bool deleted = dir.removeRecursively();

  if (!deleted) {
    // File handle locked (typical on Windows for loaded DLLs)
    // Mark for removal on next application startup
    QStringList pending = getPendingRemovals();
    if (!pending.contains(pluginId)) {
      pending.append(pluginId);
      QSettings settings(QStringLiteral("PicAudio"),
                         QStringLiteral("PicAudio"));
      settings.setValue(PendingRemovalSettingsKey, pending);
    }
    outError = QStringLiteral("Plugin unloaded. Full folder deletion staged "
                              "for next application restart.");
    return true;
  }

  // Clean pending list if present
  QStringList pending = getPendingRemovals();
  if (pending.contains(pluginId)) {
    pending.removeAll(pluginId);
    QSettings settings(QStringLiteral("PicAudio"), QStringLiteral("PicAudio"));
    settings.setValue(PendingRemovalSettingsKey, pending);
  }

  return true;
}

bool PluginInstaller::isPendingRemoval(const QString &pluginId) {
  return getPendingRemovals().contains(pluginId);
}

QStringList PluginInstaller::getPendingRemovals() {
  QSettings settings(QStringLiteral("PicAudio"), QStringLiteral("PicAudio"));
  return settings.value(PendingRemovalSettingsKey).toStringList();
}

int PluginInstaller::processPendingRemovals(const QString &pluginsDir) {
  QStringList pending = getPendingRemovals();
  if (pending.isEmpty()) {
    return 0;
  }

  int count = 0;
  QStringList remaining;

  for (const auto &id : pending) {
    const QString targetPath = pluginsDir + QLatin1Char('/') + id;
    if (QDir(targetPath).exists()) {
      if (QDir(targetPath).removeRecursively()) {
        ++count;
      } else {
        remaining.append(id);
      }
    }
  }

  QSettings settings(QStringLiteral("PicAudio"), QStringLiteral("PicAudio"));
  settings.setValue(PendingRemovalSettingsKey, remaining);
  return count;
}

} // namespace adapters
