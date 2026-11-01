#include "PluginManager.hpp"
#include "theming/ThemeLoader.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QProcessEnvironment>
#include <QSettings>
#include <QStandardPaths>

#include <iostream>

namespace adapters {

PluginManager::PluginManager(QObject *parent) : QObject(parent) {
  m_hostUiServices.hostContext = this;
  m_hostUiServices.navigateToView = &PluginManager::hostNavigateToView;
  m_hostUiServices.showToast = &PluginManager::hostShowToast;
}

PluginManager::~PluginManager() { unloadAllPlugins(); }

void PluginManager::initialize(const QStringList &extraSearchPaths) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_searchPaths.clear();

  for (const auto &path : extraSearchPaths) {
    if (!path.trimmed().isEmpty() && !m_searchPaths.contains(path)) {
      m_searchPaths.append(path);
    }
  }

  const auto envPath = QProcessEnvironment::systemEnvironment().value(
      QStringLiteral("PICAUDIO_PLUGIN_DIR"));
  if (!envPath.trimmed().isEmpty() && !m_searchPaths.contains(envPath)) {
    m_searchPaths.append(envPath);
  }

  const QString appDirPlugins =
      QCoreApplication::applicationDirPath() + QStringLiteral("/plugins");
  if (!m_searchPaths.contains(appDirPlugins)) {
    m_searchPaths.append(appDirPlugins);
  }

  const QString appDataPlugins =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
      QStringLiteral("/plugins");
  if (!m_searchPaths.contains(appDataPlugins)) {
    m_searchPaths.append(appDataPlugins);
  }

  const QString localSharePicAudio =
      QDir::homePath() + QStringLiteral("/.local/share/PicAudio/plugins");
  if (!m_searchPaths.contains(localSharePicAudio)) {
    m_searchPaths.append(localSharePicAudio);
  }

  const QString localShareParakeet =
      QDir::homePath() +
      QStringLiteral("/.local/share/ParakeetAudio/Parakeet/plugins");
  if (!m_searchPaths.contains(localShareParakeet)) {
    m_searchPaths.append(localShareParakeet);
  }

  // Ensure the primary directory exists
  QDir().mkpath(getPrimaryPluginDirectory());
}

QString PluginManager::getPrimaryPluginDirectory() const {
  return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
         QStringLiteral("/plugins");
}

QStringList PluginManager::getPluginDirectories() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_searchPaths;
}

bool PluginManager::validateApiVersion(const QString &targetVersion,
                                       QString &outError) {
  const QStringList parts = targetVersion.split(QLatin1Char('.'));
  if (parts.isEmpty()) {
    outError = QStringLiteral("Malformed targetApiVersion string.");
    return false;
  }

  bool ok = false;
  const int major = parts[0].toInt(&ok);
  if (!ok) {
    outError = QStringLiteral("Invalid API major version number.");
    return false;
  }

  if (major != PICAUDIO_PLUGIN_API_VERSION_MAJOR) {
    outError = QStringLiteral("Incompatible SDK API major version: plugin "
                              "targets v%1, host supports v%2")
                   .arg(major)
                   .arg(PICAUDIO_PLUGIN_API_VERSION_MAJOR);
    return false;
  }

  return true;
}

bool PluginManager::parseManifest(const QString &manifestPath,
                                  PluginInfo &info) {
  QFile file(manifestPath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    info.loadError = QStringLiteral("Failed to open plugin.json: %1")
                         .arg(file.errorString());
    return false;
  }

  QJsonParseError parseError;
  const QJsonDocument doc =
      QJsonDocument::fromJson(file.readAll(), &parseError);
  if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
    info.loadError =
        QStringLiteral("JSON syntax error: %1").arg(parseError.errorString());
    return false;
  }

  const QJsonObject obj = doc.object();
  info.id = obj.value(QStringLiteral("id")).toString().trimmed();
  info.name = obj.value(QStringLiteral("name")).toString().trimmed();
  info.version = obj.value(QStringLiteral("version")).toString().trimmed();
  info.author = obj.value(QStringLiteral("author")).toString().trimmed();
  info.description =
      obj.value(QStringLiteral("description")).toString().trimmed();
  info.homepage = obj.value(QStringLiteral("homepage")).toString().trimmed();
  info.targetApiVersion =
      obj.value(QStringLiteral("targetApiVersion")).toString().trimmed();
  info.entryPoint =
      obj.value(QStringLiteral("entryPoint")).toString().trimmed();
  info.minHostVersion =
      obj.value(QStringLiteral("minHostVersion")).toString().trimmed();

  if (info.id.isEmpty() || info.name.isEmpty() || info.version.isEmpty() ||
      info.targetApiVersion.isEmpty()) {
    info.loadError = QStringLiteral("Missing required manifest fields (id, "
                                    "name, version, targetApiVersion)");
    return false;
  }

  info.capabilities.clear();
  const QJsonArray caps = obj.value(QStringLiteral("capabilities")).toArray();
  for (const auto &capVal : caps) {
    info.capabilities.append(capVal.toString().trimmed().toLower());
  }

  info.customActions.clear();
  if (obj.contains(QStringLiteral("actions")) &&
      obj.value(QStringLiteral("actions")).isArray()) {
    const QJsonArray actionsArr = obj.value(QStringLiteral("actions")).toArray();
    for (const auto &actVal : actionsArr) {
      if (actVal.isObject()) {
        const QJsonObject actObj = actVal.toObject();
        PluginCustomAction act;
        act.id = actObj.value(QStringLiteral("id")).toString().trimmed();
        act.label = actObj.value(QStringLiteral("label")).toString().trimmed();
        act.tooltip =
            actObj.value(QStringLiteral("tooltip")).toString().trimmed();
        act.iconName =
            actObj.value(QStringLiteral("iconName")).toString().trimmed();
        act.isPrimary = actObj.value(QStringLiteral("isPrimary")).toBool(false);
        if (!act.id.isEmpty() && !act.label.isEmpty()) {
          info.customActions.append(act);
        }
      }
    }
  }

  QString apiErr;
  if (!validateApiVersion(info.targetApiVersion, apiErr)) {
    info.loadError = apiErr;
    return false;
  }

  return true;
}

void PluginManager::discoverInDirectory(const QString &dirPath) {
  QDir dir(dirPath);
  if (!dir.exists()) {
    return;
  }

  const QFileInfoList entries =
      dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable);
  for (const auto &entry : entries) {
    const QString subDirPath = entry.absoluteFilePath();
    const QString manifestPath = subDirPath + QStringLiteral("/plugin.json");
    if (!QFile::exists(manifestPath)) {
      continue;
    }

    PluginInfo info;
    info.directory = subDirPath;

    const bool parsed = parseManifest(manifestPath, info);
    if (!parsed && info.id.isEmpty()) {
      continue;
    }

    // Check if plugin is already tracked
    auto it = std::find_if(
        m_plugins.begin(), m_plugins.end(),
        [&](const LoadedPluginRecord &r) { return r.info.id == info.id; });
    if (it != m_plugins.end()) {
      continue;
    }

    QSettings s(QStringLiteral("PicAudio"), QStringLiteral("PicAudio"));
    const QString settingKey =
        QStringLiteral("plugins/%1/enabled").arg(info.id);
    info.enabled = s.value(settingKey, true).toBool();

    // Locate library binary
    QString libraryCandidate;
    if (!info.entryPoint.isEmpty()) {
      libraryCandidate = subDirPath + QLatin1Char('/') + info.entryPoint;
    } else {
      // Auto-detect standard library names based on directory or id
      const QString baseName = entry.fileName();
      const QStringList candidateNames = {
          subDirPath + QStringLiteral("/lib%1.so").arg(baseName),
          subDirPath + QStringLiteral("/%1.so").arg(baseName),
          subDirPath + QStringLiteral("/%1.dll").arg(baseName),
          subDirPath + QStringLiteral("/lib%1.dylib").arg(baseName),
          subDirPath + QStringLiteral("/plugin.so"),
          subDirPath + QStringLiteral("/plugin.dll")};
      for (const auto &cand : candidateNames) {
        if (QFile::exists(cand)) {
          libraryCandidate = cand;
          break;
        }
      }
    }

    info.libraryPath = libraryCandidate;

    LoadedPluginRecord record;
    record.info = info;

    if (parsed) {
      if (info.libraryPath.isEmpty() || !QFile::exists(info.libraryPath)) {
        record.info.loadError =
            QStringLiteral("Dynamic library file not found in plugin folder.");
      } else if (record.info.enabled) {
        loadPluginFromRecord(record);
      }
    }

    m_plugins.push_back(std::move(record));
  }
}

bool PluginManager::loadPluginFromRecord(LoadedPluginRecord &record) {
  if (record.info.libraryPath.isEmpty() ||
      !QFile::exists(record.info.libraryPath)) {
    record.info.loadError = QStringLiteral("Plugin library file missing.");
    record.info.loaded = false;
    return false;
  }

  auto lib = std::make_unique<QLibrary>(record.info.libraryPath);
  if (!lib->load()) {
    record.info.loadError =
        QStringLiteral("Dynamic load failed: %1").arg(lib->errorString());
    record.info.loaded = false;
    return false;
  }

  // Resolve factory entry points
  auto createFunc =
      reinterpret_cast<picaudio::plugin::PicAudioCreatePluginFunc>(
          lib->resolve(PICAUDIO_PLUGIN_ENTRY_CREATE));
  if (!createFunc) {
    createFunc = reinterpret_cast<picaudio::plugin::PicAudioCreatePluginFunc>(
        lib->resolve("create_picaudio_plugin"));
  }
  if (!createFunc) {
    createFunc = reinterpret_cast<picaudio::plugin::PicAudioCreatePluginFunc>(
        lib->resolve("create_parakeet_plugin"));
  }

  if (!createFunc) {
    record.info.loadError =
        QStringLiteral("Failed to resolve 'create_plugin' symbol.");
    lib->unload();
    record.info.loaded = false;
    return false;
  }

  auto destroyFunc =
      reinterpret_cast<picaudio::plugin::PicAudioDestroyPluginFunc>(
          lib->resolve(PICAUDIO_PLUGIN_ENTRY_DESTROY));
  if (!destroyFunc) {
    destroyFunc = reinterpret_cast<picaudio::plugin::PicAudioDestroyPluginFunc>(
        lib->resolve("destroy_picaudio_plugin"));
  }
  if (!destroyFunc) {
    destroyFunc = reinterpret_cast<picaudio::plugin::PicAudioDestroyPluginFunc>(
        lib->resolve("destroy_parakeet_plugin"));
  }

  picaudio::plugin::IPicAudioPlugin *instance = nullptr;
  try {
    instance = createFunc();
  } catch (const std::exception &ex) {
    record.info.loadError =
        QStringLiteral("Exception in create_plugin: %1").arg(ex.what());
    lib->unload();
    record.info.loaded = false;
    return false;
  } catch (...) {
    record.info.loadError =
        QStringLiteral("Unknown exception in create_plugin.");
    lib->unload();
    record.info.loaded = false;
    return false;
  }

  if (!instance) {
    record.info.loadError =
        QStringLiteral("create_plugin returned null pointer.");
    lib->unload();
    record.info.loaded = false;
    return false;
  }

  // Verify ABI version returned by instance
  try {
    const uint32_t instApiVer = instance->getApiVersion();
    const uint32_t instMajor = (instApiVer >> 16) & 0xFF;
    if (instMajor != PICAUDIO_PLUGIN_API_VERSION_MAJOR) {
      record.info.loadError =
          QStringLiteral("Instance returned incompatible API major %1")
              .arg(instMajor);
      if (destroyFunc) {
        destroyFunc(instance);
      }
      lib->unload();
      record.info.loaded = false;
      return false;
    }

    if (!instance->initialize()) {
      record.info.loadError =
          QStringLiteral("Plugin initialize() returned false.");
      if (destroyFunc) {
        destroyFunc(instance);
      }
      lib->unload();
      record.info.loaded = false;
      return false;
    }

    instance->start();
  } catch (const std::exception &ex) {
    record.info.loadError =
        QStringLiteral("Exception during initialization: %1").arg(ex.what());
    if (destroyFunc) {
      try {
        destroyFunc(instance);
      } catch (...) {
      }
    }
    lib->unload();
    record.info.loaded = false;
    return false;
  } catch (...) {
    record.info.loadError =
        QStringLiteral("Unknown crash during initialization.");
    if (destroyFunc) {
      try {
        destroyFunc(instance);
      } catch (...) {
      }
    }
    lib->unload();
    record.info.loaded = false;
    return false;
  }

  record.library = std::move(lib);
  record.instance = instance;
  record.destroyFunc = destroyFunc;
  record.info.loaded = true;
  record.info.loadError.clear();

  // Query capability interfaces
  try {
    if (instance->getCapabilities() &
        picaudio::plugin::PICAUDIO_CAP_VISUALIZER) {
      record.visualizer = static_cast<picaudio::plugin::IVisualizerPlugin *>(
          instance->getCapabilityInterface(
              picaudio::plugin::PICAUDIO_CAP_VISUALIZER));
    }
    if (instance->getCapabilities() &
        (picaudio::plugin::PICAUDIO_CAP_LYRICS |
         picaudio::plugin::PICAUDIO_CAP_SCROBBLER |
         picaudio::plugin::PICAUDIO_CAP_LIBRARY)) {
      record.libraryPort = static_cast<picaudio::plugin::ILibraryPlugin *>(
          instance->getCapabilityInterface(
              picaudio::plugin::PICAUDIO_CAP_LIBRARY));
      if (!record.libraryPort && (instance->getCapabilities() &
                                  picaudio::plugin::PICAUDIO_CAP_LYRICS)) {
        record.libraryPort = static_cast<picaudio::plugin::ILibraryPlugin *>(
            instance->getCapabilityInterface(
                picaudio::plugin::PICAUDIO_CAP_LYRICS));
      }
      if (!record.libraryPort && (instance->getCapabilities() &
                                  picaudio::plugin::PICAUDIO_CAP_SCROBBLER)) {
        record.libraryPort = static_cast<picaudio::plugin::ILibraryPlugin *>(
            instance->getCapabilityInterface(
                picaudio::plugin::PICAUDIO_CAP_SCROBBLER));
      }
    }
    if (instance->getCapabilities() & (picaudio::plugin::PICAUDIO_CAP_DSP)) {
      record.playerPort = static_cast<picaudio::plugin::IPlayerPlugin *>(
          instance->getCapabilityInterface(picaudio::plugin::PICAUDIO_CAP_DSP));
    }
    if (instance->getCapabilities() & (picaudio::plugin::PICAUDIO_CAP_THEME)) {
      record.themePort = static_cast<picaudio::plugin::IThemePlugin *>(
          instance->getCapabilityInterface(
              picaudio::plugin::PICAUDIO_CAP_THEME));
      if (record.themePort) {
        record.themePort->setHostThemeCallback(
            this, &PluginManager::onPluginThemeUpdate);
        applyThemePluginOverrides(record.themePort);
      }
    }
    if (instance->getCapabilities() & (picaudio::plugin::PICAUDIO_CAP_UI)) {
      record.uiPort = static_cast<picaudio::plugin::IUiPlugin *>(
          instance->getCapabilityInterface(picaudio::plugin::PICAUDIO_CAP_UI));
      if (record.uiPort) {
        record.uiPort->setHostUiServices(&m_hostUiServices);

        // Query dynamic custom actions from C-ABI interface
        picaudio::plugin::PicAudioCustomAction dynActions[16];
        const size_t numDyn = record.uiPort->getCustomActions(dynActions, 16);
        for (size_t i = 0; i < numDyn; ++i) {
          if (dynActions[i].id && dynActions[i].label) {
            PluginCustomAction act;
            act.id = QString::fromUtf8(dynActions[i].id);
            act.label = QString::fromUtf8(dynActions[i].label);
            act.tooltip = dynActions[i].tooltip
                              ? QString::fromUtf8(dynActions[i].tooltip)
                              : QString();
            act.iconName = dynActions[i].iconName
                               ? QString::fromUtf8(dynActions[i].iconName)
                               : QString();
            act.isPrimary = dynActions[i].isPrimary;

            auto it = std::find_if(
                record.info.customActions.begin(),
                record.info.customActions.end(),
                [&](const PluginCustomAction &a) { return a.id == act.id; });
            if (it != record.info.customActions.end()) {
              *it = act;
            } else {
              record.info.customActions.append(act);
            }
          }
        }
      }
    }
  } catch (...) {
    // Log query warning without failing overall plugin load
  }

  return true;
}

void PluginManager::unloadPluginRecord(LoadedPluginRecord &record) {
  if (record.instance) {
    try {
      record.instance->stop();
      record.instance->shutdown();
    } catch (...) {
      // Suppress crash during shutdown
    }

    if (record.destroyFunc) {
      try {
        record.destroyFunc(record.instance);
      } catch (...) {
      }
    }
    record.instance = nullptr;
  }

  record.visualizer = nullptr;
  record.libraryPort = nullptr;
  record.playerPort = nullptr;
  record.themePort = nullptr;
  record.uiPort = nullptr;
  record.destroyFunc = nullptr;

  if (record.library) {
    record.library->unload();
    record.library.reset();
  }

  record.info.loaded = false;
}

void PluginManager::unloadAllPlugins() {
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto &record : m_plugins) {
    unloadPluginRecord(record);
  }
  m_plugins.clear();
}

void PluginManager::scanAndLoadPlugins() {
  std::lock_guard<std::mutex> lock(m_mutex);
  for (const auto &path : m_searchPaths) {
    discoverInDirectory(path);
  }
  emit pluginsChanged();
}

QVector<PluginInfo> PluginManager::getDiscoveredPlugins() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  QVector<PluginInfo> result;
  result.reserve(static_cast<qsizetype>(m_plugins.size()));
  for (const auto &record : m_plugins) {
    result.append(record.info);
  }
  return result;
}

bool PluginManager::setPluginEnabled(const QString &id, bool enabled) {
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto &record : m_plugins) {
    if (record.info.id == id) {
      if (record.info.enabled == enabled) {
        return true;
      }

      record.info.enabled = enabled;
      QSettings s(QStringLiteral("PicAudio"), QStringLiteral("PicAudio"));
      s.setValue(QStringLiteral("plugins/%1/enabled").arg(id), enabled);

      if (enabled && !record.info.loaded) {
        loadPluginFromRecord(record);
      } else if (!enabled && record.info.loaded) {
        unloadPluginRecord(record);
      }

      emit pluginsChanged();
      return true;
    }
  }
  return false;
}

void PluginManager::routeAudioBuffer(const float *buffer, size_t frames,
                                     uint32_t sampleRate, uint8_t channels) {
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto &record : m_plugins) {
    if (record.info.loaded && record.info.enabled && record.visualizer) {
      try {
        if (record.visualizer->isEnabled()) {
          record.visualizer->processAudioBuffer(buffer, frames, sampleRate,
                                                channels);
        }
      } catch (...) {
        // Safeguard against visualizer buffer crash
      }
    }
  }
}

size_t PluginManager::getFrequencyBands(float *outBands, size_t maxBands) {
  if (!outBands || maxBands == 0) {
    return 0;
  }

  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto &record : m_plugins) {
    if (record.info.loaded && record.info.enabled && record.visualizer) {
      try {
        if (record.visualizer->isEnabled()) {
          return record.visualizer->getFrequencyBands(outBands, maxBands);
        }
      } catch (...) {
        return 0;
      }
    }
  }
  return 0;
}

void PluginManager::routePreDsp(float *buffer, size_t frames,
                                uint32_t sampleRate, uint8_t channels) {
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto &record : m_plugins) {
    if (record.info.loaded && record.info.enabled && record.playerPort) {
      try {
        record.playerPort->onPreDsp(buffer, frames, sampleRate, channels);
      } catch (...) {
      }
    }
  }
}

void PluginManager::routePostDsp(float *buffer, size_t frames,
                                 uint32_t sampleRate, uint8_t channels) {
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto &record : m_plugins) {
    if (record.info.loaded && record.info.enabled && record.playerPort) {
      try {
        record.playerPort->onPostDsp(buffer, frames, sampleRate, channels);
      } catch (...) {
      }
    }
  }
}

void PluginManager::routeTrackStarted(
    const picaudio::plugin::PicAudioTrackInfo &track) {
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto &record : m_plugins) {
    if (record.info.loaded && record.info.enabled && record.playerPort) {
      try {
        record.playerPort->onTrackStarted(&track);
      } catch (...) {
      }
    }
  }
}

void PluginManager::routePlaybackStateChanged(
    picaudio::plugin::PicAudioPlaybackState state) {
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto &record : m_plugins) {
    if (record.info.loaded && record.info.enabled && record.playerPort) {
      try {
        record.playerPort->onPlaybackStateChanged(state);
      } catch (...) {
      }
    }
  }
}

std::string PluginManager::routeFetchLyrics(const std::string &artist,
                                            const std::string &title) {
  std::lock_guard<std::mutex> lock(m_mutex);
  std::vector<char> buffer(65536, 0);

  for (auto &record : m_plugins) {
    if (record.info.loaded && record.info.enabled && record.libraryPort) {
      try {
        const size_t bytes = record.libraryPort->fetchLyrics(
            artist.c_str(), title.c_str(), buffer.data(), buffer.size());
        if (bytes > 0) {
          return std::string(buffer.data(), std::min(bytes, buffer.size() - 1));
        }
      } catch (...) {
      }
    }
  }
  return {};
}

void PluginManager::routeTrackScrobbled(
    const picaudio::plugin::PicAudioTrackInfo &track, uint64_t timestamp) {
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto &record : m_plugins) {
    if (record.info.loaded && record.info.enabled && record.libraryPort) {
      try {
        record.libraryPort->onTrackScrobbled(&track, timestamp);
      } catch (...) {
      }
    }
  }
}

void PluginManager::routeTrackIndexed(
    const picaudio::plugin::PicAudioTrackInfo &track) {
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto &record : m_plugins) {
    if (record.info.loaded && record.info.enabled && record.libraryPort) {
      try {
        record.libraryPort->onTrackIndexed(&track);
      } catch (...) {
      }
    }
  }
}

void PluginManager::setThemeLoader(ThemeLoader *themeLoader) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_themeLoader = themeLoader;
  if (m_themeLoader) {
    for (auto &record : m_plugins) {
      if (record.info.loaded && record.info.enabled && record.themePort) {
        applyThemePluginOverrides(record.themePort);
      }
    }
  }
}

void PluginManager::applyThemePluginOverrides(
    picaudio::plugin::IThemePlugin *themePlugin) {
  if (!themePlugin || !m_themeLoader)
    return;
  std::vector<picaudio::plugin::PicAudioColorToken> tokens(64);
  size_t count = themePlugin->getThemeOverrides(tokens.data(), tokens.size());
  if (count > 0) {
    QMap<QString, QString> overrides;
    for (size_t i = 0; i < count; ++i) {
      if (tokens[i].key && tokens[i].hexColor) {
        overrides[QString::fromUtf8(tokens[i].key)] =
            QString::fromUtf8(tokens[i].hexColor);
      }
    }
    m_themeLoader->applyColorOverrides(overrides);
  }
}

void PluginManager::onPluginThemeUpdate(
    void *hostContext, const picaudio::plugin::PicAudioColorToken *tokens,
    size_t count) {
  if (!hostContext || !tokens || count == 0)
    return;
  auto *self = static_cast<PluginManager *>(hostContext);
  std::lock_guard<std::mutex> lock(self->m_mutex);
  if (!self->m_themeLoader)
    return;

  QMap<QString, QString> overrides;
  for (size_t i = 0; i < count; ++i) {
    if (tokens[i].key && tokens[i].hexColor) {
      overrides[QString::fromUtf8(tokens[i].key)] =
          QString::fromUtf8(tokens[i].hexColor);
    }
  }
  self->m_themeLoader->applyColorOverrides(overrides);
}

void PluginManager::routeBaseThemeChanged(const QString &themeId) {
  std::lock_guard<std::mutex> lock(m_mutex);
  const std::string themeStr = themeId.toStdString();
  for (auto &record : m_plugins) {
    if (record.info.loaded && record.info.enabled && record.themePort) {
      try {
        record.themePort->onBaseThemeChanged(themeStr.c_str());
      } catch (...) {
      }
    }
  }
}

void PluginManager::hostNavigateToView(void *hostContext, int viewMode) {
  auto *self = static_cast<PluginManager *>(hostContext);
  if (self && self->m_navigateCallback) {
    self->m_navigateCallback(viewMode);
  }
}

void PluginManager::hostShowToast(void *hostContext, const char *message,
                                  const char *level) {
  auto *self = static_cast<PluginManager *>(hostContext);
  if (self && self->m_toastCallback && message) {
    self->m_toastCallback(
        QString::fromUtf8(message),
        level ? QString::fromUtf8(level) : QStringLiteral("info"));
  }
}

bool PluginManager::triggerPluginAction(const QString &pluginId,
                                        const QString &actionId) {
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto &record : m_plugins) {
    if (record.info.id == pluginId && record.info.loaded &&
        record.info.enabled && record.uiPort) {
      try {
        const std::string actStr = actionId.toStdString();
        return record.uiPort->triggerAction(actStr.c_str());
      } catch (...) {
        return false;
      }
    }
  }
  return false;
}

bool PluginManager::showPluginWindow(const QString &pluginId) {
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto &record : m_plugins) {
    if (record.info.id == pluginId && record.info.loaded &&
        record.info.enabled && record.uiPort) {
      try {
        return record.uiPort->showWindow();
      } catch (...) {
        return false;
      }
    }
  }
  return false;
}

bool PluginManager::hidePluginWindow(const QString &pluginId) {
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto &record : m_plugins) {
    if (record.info.id == pluginId && record.info.loaded &&
        record.info.enabled && record.uiPort) {
      try {
        return record.uiPort->hideWindow();
      } catch (...) {
        return false;
      }
    }
  }
  return false;
}

bool PluginManager::isPluginWindowVisible(const QString &pluginId) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  for (const auto &record : m_plugins) {
    if (record.info.id == pluginId && record.info.loaded &&
        record.info.enabled && record.uiPort) {
      try {
        return record.uiPort->isWindowVisible();
      } catch (...) {
        return false;
      }
    }
  }
  return false;
}

void PluginManager::setNavigateCallback(std::function<void(int)> cb) {
  m_navigateCallback = std::move(cb);
}

void PluginManager::setToastCallback(
    std::function<void(const QString &, const QString &)> cb) {
  m_toastCallback = std::move(cb);
}

} // namespace adapters
