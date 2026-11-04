#include "DspManager.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace adapters {

DspManager::DspManager(QObject *parent) : QObject(parent) {
  // Determine standard user-writable DSP directory
  QString appData =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  if (appData.isEmpty()) {
    appData = QDir::homePath() + "/.config/PicAudio";
  }

  m_primaryDspDir = (appData + "/dsp").toStdString();
  m_searchPaths.push_back(m_primaryDspDir);

  // Standard user config, data, and system CLAP search directories
  std::vector<QString> candidateDirs = {
      QDir::homePath() + "/.config/PicAudio/dsp",
      QDir::homePath() + "/.config/PicAudio/PicAudio/dsp",
      QDir::homePath() + "/.local/share/PicAudio/dsp",
      QDir::homePath() + "/.local/share/PicAudio/PicAudio/dsp",
      QDir::homePath() + "/.clap",
#if defined(__APPLE__)
      QDir::homePath() + "/Library/Audio/Plug-Ins/CLAP",
      QStringLiteral("/Library/Audio/Plug-Ins/CLAP"),
#elif defined(_WIN32)
      QDir::homePath() + "/AppData/Local/Programs/Common/CLAP",
      QStringLiteral("C:/Program Files/Common Files/CLAP"),
#else
      QStringLiteral("/usr/lib/clap"),
      QStringLiteral("/usr/local/lib/clap"),
#endif
  };

  for (const auto &c : candidateDirs) {
    std::string s = c.toStdString();
    if (std::find(m_searchPaths.begin(), m_searchPaths.end(), s) ==
        m_searchPaths.end()) {
      m_searchPaths.push_back(s);
    }
  }

  // Adjacent portable directories
  QString appDir = QCoreApplication::applicationDirPath();
  m_searchPaths.push_back((appDir + "/dsp").toStdString());
  m_searchPaths.push_back((appDir + "/../dsp").toStdString());

  ensureRuntimeDirectories();
}

DspManager::~DspManager() { unloadAllPlugins(); }

void DspManager::initialize(const std::vector<std::string> &extraSearchPaths) {
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto &p : extraSearchPaths) {
      if (!p.empty() && std::find(m_searchPaths.begin(), m_searchPaths.end(),
                                  p) == m_searchPaths.end()) {
        m_searchPaths.push_back(p);
      }
    }
  }

  ensureRuntimeDirectories();
  scanAndLoadPlugins();
}

std::string DspManager::getPrimaryDspDirectory() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_primaryDspDir;
}

void DspManager::ensureRuntimeDirectories() {
  std::lock_guard<std::mutex> lock(m_mutex);
  for (const auto &dirPath : m_searchPaths) {
    std::error_code ec;
    std::filesystem::path p(dirPath);
    if (!std::filesystem::exists(p, ec)) {
      std::filesystem::create_directories(p, ec);
    }
  }

  // Create helpful README in primary DSP directory
  if (!m_primaryDspDir.empty()) {
    std::filesystem::path readmePath =
        std::filesystem::path(m_primaryDspDir) / "README.txt";
    std::error_code ec;
    if (!std::filesystem::exists(readmePath, ec)) {
      std::ofstream out(readmePath);
      if (out.is_open()) {
        out << "========================================================\n"
            << "PicAudio - External Audio DSP Plugins Directory\n"
            << "========================================================\n\n"
            << "Place external audio DSP plugins here (.clap files).\n"
            << "They will be automatically scanned and loaded by PicAudio.\n\n"
            << "Supported format: Native CLAP (CLever Audio Plugin)\n"
            << "Examples: LSP Plugins, Surge DSP, ChowDSP, Airwindows CLAP\n\n"
            << "Once copied into this directory, open PicAudio and click\n"
            << "'Rescan' in the Equalizer & DSP settings to load them.\n"
            << "You can enable/disable individual plugins and open their\n"
            << "native graphical control panels directly from the player.\n";
      }
    }
  }
}

std::vector<std::string> DspManager::getDspDirectories() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_searchPaths;
}

void DspManager::unloadAllPlugins() {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_enabledChain.clear();
  m_loadedInstances.clear();
  m_discoveredPlugins.clear();
  uint32_t activeIdx = m_activeChainIndex.load(std::memory_order_relaxed);
  uint32_t inactiveIdx = 1 - activeIdx;
  m_chainSnapshots[inactiveIdx].count = 0;
  m_activeChainIndex.store(inactiveIdx, std::memory_order_release);
}

void DspManager::rebuildEnabledChainLocked() {
  m_enabledChain.clear();
  for (auto &meta : m_discoveredPlugins) {
    if (meta.enabled) {
      auto *host = getOrCreateHostLocked(meta.id);
      if (host) {
        host->setEnabled(true);
        m_enabledChain.push_back(host);
      }
    }
  }

  uint32_t activeIdx = m_activeChainIndex.load(std::memory_order_relaxed);
  uint32_t inactiveIdx = 1 - activeIdx;
  auto &snap = m_chainSnapshots[inactiveIdx];
  snap.count = std::min(m_enabledChain.size(), size_t(32));
  for (size_t i = 0; i < snap.count; ++i) {
    snap.hosts[i] = m_enabledChain[i];
  }
  m_activeChainIndex.store(inactiveIdx, std::memory_order_release);
}

void DspManager::saveEnabledSettingsLocked() {
  QSettings settings;
  QStringList enabledIds;
  for (const auto &meta : m_discoveredPlugins) {
    if (meta.enabled) {
      enabledIds.append(QString::fromStdString(meta.id));
    }
  }
  settings.setValue(QStringLiteral("dsp/enabledPluginIds"), enabledIds);
}

ClapPluginHost *DspManager::getOrCreateHostLocked(const std::string &pluginId) {
  auto it = m_loadedInstances.find(pluginId);
  if (it != m_loadedInstances.end() && it->second) {
    return it->second.get();
  }

  for (const auto &meta : m_discoveredPlugins) {
    if (meta.id == pluginId) {
      auto host = std::make_unique<ClapPluginHost>();
      host->setWindowVisibilityCallback([this](const std::string &, bool) {
        QMetaObject::invokeMethod(
            this, [this]() { emit dspPluginsChanged(); }, Qt::QueuedConnection);
      });
      if (host->load(meta.libraryPath, pluginId)) {
        host->setEnabled(meta.enabled);
        auto *raw = host.get();
        m_loadedInstances[pluginId] = std::move(host);
        return raw;
      }
      break;
    }
  }
  return nullptr;
}

void DspManager::scanDirectory(const std::string &dirPath) {
  std::error_code ec;
  if (!std::filesystem::exists(dirPath, ec) ||
      !std::filesystem::is_directory(dirPath, ec)) {
    return;
  }

  for (const auto &entry : std::filesystem::directory_iterator(dirPath, ec)) {
    if (ec)
      break;

    const auto &p = entry.path();
    if (p.extension() == ".clap" ||
        p.string().find(".clap") != std::string::npos) {
      std::string fullPath = p.string();

      // Check if this library was already enumerated
      bool alreadyEnumerated = false;
      for (const auto &m : m_discoveredPlugins) {
        if (m.libraryPath == fullPath) {
          alreadyEnumerated = true;
          break;
        }
      }
      if (alreadyEnumerated) {
        continue;
      }

      auto descriptors = ClapPluginHost::enumeratePlugins(fullPath);
      for (auto &desc : descriptors) {
        // De-duplicate by plugin ID across search paths
        bool alreadyExists = false;
        for (const auto &m : m_discoveredPlugins) {
          if (m.id == desc.id) {
            alreadyExists = true;
            break;
          }
        }
        if (!alreadyExists) {
          desc.enabled = false;
          m_discoveredPlugins.push_back(std::move(desc));
        }
      }
    }
  }
}

void DspManager::scanAndLoadPlugins() {
  ensureRuntimeDirectories();

  std::vector<std::string> pathsToScan;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    pathsToScan = m_searchPaths;
  }

  for (const auto &dir : pathsToScan) {
    std::lock_guard<std::mutex> lock(m_mutex);
    scanDirectory(dir);
  }

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    QSettings settings;
    QStringList enabledIds =
        settings.value(QStringLiteral("dsp/enabledPluginIds")).toStringList();
    for (const auto &idStr : enabledIds) {
      std::string id = idStr.toStdString();
      for (auto &meta : m_discoveredPlugins) {
        if (meta.id == id) {
          meta.enabled = true;
        }
      }
    }
    rebuildEnabledChainLocked();
  }

  emit dspPluginsChanged();
}

std::vector<ClapPluginMetadata> DspManager::getDiscoveredPlugins() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_discoveredPlugins;
}

bool DspManager::setPluginEnabled(const std::string &pluginId, bool enabled) {
  bool found = false;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto &meta : m_discoveredPlugins) {
      if (meta.id == pluginId) {
        meta.enabled = enabled;
        found = true;
        if (enabled) {
          auto *host = getOrCreateHostLocked(pluginId);
          if (host) {
            host->setEnabled(true);
          }
        } else {
          auto it = m_loadedInstances.find(pluginId);
          if (it != m_loadedInstances.end() && it->second) {
            it->second->setEnabled(false);
          }
        }
        break;
      }
    }
    if (found) {
      rebuildEnabledChainLocked();
      saveEnabledSettingsLocked();
    }
  }
  if (found) {
    emit dspPluginsChanged();
  }
  return found;
}

void DspManager::setAllPluginsEnabled(bool enabled) {
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto &meta : m_discoveredPlugins) {
      meta.enabled = enabled;
      if (enabled) {
        auto *host = getOrCreateHostLocked(meta.id);
        if (host) {
          host->setEnabled(true);
        }
      } else {
        auto it = m_loadedInstances.find(meta.id);
        if (it != m_loadedInstances.end() && it->second) {
          it->second->setEnabled(false);
        }
      }
    }
    rebuildEnabledChainLocked();
    saveEnabledSettingsLocked();
  }
  emit dspPluginsChanged();
}

bool DspManager::showPluginWindow(const std::string &pluginId) {
  ClapPluginHost *target = nullptr;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    target = getOrCreateHostLocked(pluginId);
  }
  if (target) {
    bool ok = target->showWindow();
    emit dspPluginsChanged();
    return ok;
  }
  return false;
}

bool DspManager::hidePluginWindow(const std::string &pluginId) {
  ClapPluginHost *target = nullptr;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_loadedInstances.find(pluginId);
    if (it != m_loadedInstances.end()) {
      target = it->second.get();
    }
  }
  if (target) {
    bool ok = target->hideWindow();
    emit dspPluginsChanged();
    return ok;
  }
  return false;
}

bool DspManager::isPluginWindowVisible(const std::string &pluginId) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_loadedInstances.find(pluginId);
  if (it != m_loadedInstances.end() && it->second) {
    return it->second->isWindowVisible();
  }
  return false;
}

void DspManager::setDspMasterEnabled(bool enabled) {
  m_masterEnabled.store(enabled, std::memory_order_release);
  emit dspMasterEnabledChanged(enabled);
}

bool DspManager::isDspMasterEnabled() const {
  return m_masterEnabled.load(std::memory_order_acquire);
}

void DspManager::setRoutingOrder(RoutingOrder order) {
  m_routingOrder.store(order, std::memory_order_release);
  emit routingOrderChanged(static_cast<int>(order));
}

DspManager::RoutingOrder DspManager::getRoutingOrder() const {
  return m_routingOrder.load(std::memory_order_acquire);
}

void DspManager::processDspChain(float *buffer, size_t frames,
                                 uint32_t sampleRate, uint8_t channels) {
  if (!buffer || frames == 0)
    return;

  if (!m_masterEnabled.load(std::memory_order_relaxed)) {
    return;
  }

  uint32_t activeIdx = m_activeChainIndex.load(std::memory_order_acquire);
  const auto &snap = m_chainSnapshots[activeIdx];
  const size_t count = snap.count;
  if (count == 0) {
    return;
  }

  for (size_t i = 0; i < count; ++i) {
    auto *host = snap.hosts[i];
    if (host) {
      host->process(buffer, frames, sampleRate, channels);
    }
  }
}

} // namespace adapters
