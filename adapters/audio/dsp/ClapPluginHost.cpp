#include "ClapPluginHost.hpp"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QMetaObject>
#include <QThread>
#include <QTimer>
#include <QWindow>
#include <algorithm>
#include <cstring>
#include <iostream>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace adapters {

namespace {

uint32_t emptyEventsSize(const clap_input_events_t *) { return 0; }
const clap_event_header_t *emptyEventsGet(const clap_input_events_t *,
                                          uint32_t) {
  return nullptr;
}
bool emptyEventsPush(const clap_output_events_t *,
                     const clap_event_header_t *) {
  return true;
}

void *openSharedLibrary(const std::string &path) {
#if defined(_WIN32)
  return static_cast<void *>(LoadLibraryA(path.c_str()));
#else
  return dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
}

void *resolveSymbol(void *handle, const char *symbol) {
  if (!handle || !symbol)
    return nullptr;
#if defined(_WIN32)
  return reinterpret_cast<void *>(
      GetProcAddress(static_cast<HMODULE>(handle), symbol));
#else
  return dlsym(handle, symbol);
#endif
}

void closeSharedLibrary(void *handle) {
  if (!handle)
    return;
#if defined(_WIN32)
  FreeLibrary(static_cast<HMODULE>(handle));
#else
  dlclose(handle);
#endif
}

} // namespace

ClapPluginHost::ClapPluginHost() {
  m_clapHost.clap_version = CLAP_VERSION;
  m_clapHost.host_data = this;
  m_clapHost.name = "PicAudio";
  m_clapHost.vendor = "PicAudio Team";
  m_clapHost.url = "https://github.com/ShamalLakshan/PicAudio";
  m_clapHost.version = "1.0.0";
  m_clapHost.get_extension = &ClapPluginHost::hostGetExtension;
  m_clapHost.request_restart = &ClapPluginHost::hostRequestRestart;
  m_clapHost.request_process = &ClapPluginHost::hostRequestProcess;
  m_clapHost.request_callback = &ClapPluginHost::hostRequestCallback;

  m_clapHostGui.resize_hints_changed =
      &ClapPluginHost::hostGuiResizeHintsChanged;
  m_clapHostGui.request_resize = &ClapPluginHost::hostGuiRequestResize;
  m_clapHostGui.request_show = &ClapPluginHost::hostGuiRequestShow;
  m_clapHostGui.request_hide = &ClapPluginHost::hostGuiRequestHide;
  m_clapHostGui.closed = &ClapPluginHost::hostGuiClosed;

  m_clapHostThreadCheck.is_main_thread = &ClapPluginHost::hostIsMainThread;
  m_clapHostThreadCheck.is_audio_thread = &ClapPluginHost::hostIsAudioThread;

  m_clapHostTimerSupport.register_timer = &ClapPluginHost::hostRegisterTimer;
  m_clapHostTimerSupport.unregister_timer =
      &ClapPluginHost::hostUnregisterTimer;
}

ClapPluginHost::~ClapPluginHost() { unload(); }

std::vector<ClapPluginMetadata>
ClapPluginHost::enumeratePlugins(const std::string &libraryPath) {
  std::vector<ClapPluginMetadata> result;
  void *lib = openSharedLibrary(libraryPath);
  if (!lib) {
    return result;
  }

  auto *entry = static_cast<const clap_plugin_entry_t *>(
      resolveSymbol(lib, "clap_entry"));
  if (!entry || !entry->init) {
    closeSharedLibrary(lib);
    return result;
  }

  if (entry->init(libraryPath.c_str())) {
    auto *factory = static_cast<const clap_plugin_factory_t *>(
        entry->get_factory(CLAP_PLUGIN_FACTORY_ID));
    if (factory && factory->get_plugin_count &&
        factory->get_plugin_descriptor) {
      uint32_t count = factory->get_plugin_count(factory);
      for (uint32_t i = 0; i < count; ++i) {
        const clap_plugin_descriptor_t *desc =
            factory->get_plugin_descriptor(factory, i);
        if (desc) {
          ClapPluginMetadata meta;
          meta.id = desc->id ? desc->id : "";
          meta.name = desc->name ? desc->name : "Unknown CLAP Effect";
          meta.vendor = desc->vendor ? desc->vendor : "Unknown Vendor";
          meta.version = desc->version ? desc->version : "1.0.0";
          meta.description = desc->description ? desc->description : "";
          meta.libraryPath = libraryPath;
          meta.enabled = false;
          meta.isLoaded = false;
          meta.hasGui = true;
          result.push_back(meta);
        }
      }
    }
    if (entry->deinit) {
      entry->deinit();
    }
  }

  closeSharedLibrary(lib);
  return result;
}

bool ClapPluginHost::load(const std::string &libraryPath,
                          const std::string &pluginId, uint32_t sampleRate,
                          uint32_t maxFrames) {
  unload();

  std::lock_guard<std::mutex> audioLock(m_audioMutex);
  std::lock_guard<std::mutex> guiLock(m_guiMutex);

  m_sampleRate = (sampleRate > 0) ? sampleRate : 44100;
  m_maxFrames = (maxFrames > 0) ? maxFrames : 2048;

  m_libHandle = openSharedLibrary(libraryPath);
  if (!m_libHandle) {
    std::cerr << "ClapPluginHost: Failed to open shared library '"
              << libraryPath << "'" << std::endl;
    return false;
  }

  m_entry = static_cast<const clap_plugin_entry_t *>(
      resolveSymbol(m_libHandle, "clap_entry"));
  if (!m_entry || !m_entry->init) {
    std::cerr << "ClapPluginHost: Symbol 'clap_entry' missing in '"
              << libraryPath << "'" << std::endl;
    unload();
    return false;
  }

  if (!m_entry->init(libraryPath.c_str())) {
    std::cerr << "ClapPluginHost: clap_entry->init failed for '" << libraryPath
              << "'" << std::endl;
    unload();
    return false;
  }

  auto *factory = static_cast<const clap_plugin_factory_t *>(
      m_entry->get_factory(CLAP_PLUGIN_FACTORY_ID));
  if (!factory || !factory->create_plugin) {
    std::cerr << "ClapPluginHost: Failed to acquire CLAP_PLUGIN_FACTORY"
              << std::endl;
    unload();
    return false;
  }

  std::string targetId = pluginId;
  if (targetId.empty()) {
    if (factory->get_plugin_count && factory->get_plugin_count(factory) > 0 &&
        factory->get_plugin_descriptor) {
      const clap_plugin_descriptor_t *firstDesc =
          factory->get_plugin_descriptor(factory, 0);
      if (firstDesc && firstDesc->id) {
        targetId = firstDesc->id;
      }
    }
  }

  if (targetId.empty()) {
    std::cerr << "ClapPluginHost: No valid CLAP plugin descriptor found"
              << std::endl;
    unload();
    return false;
  }

  m_plugin = factory->create_plugin(factory, &m_clapHost, targetId.c_str());
  if (!m_plugin || !m_plugin->init) {
    std::cerr << "ClapPluginHost: Failed to instantiate CLAP plugin '"
              << targetId << "'" << std::endl;
    unload();
    return false;
  }

  if (!m_plugin->init(m_plugin)) {
    std::cerr << "ClapPluginHost: plugin->init failed for '" << targetId << "'"
              << std::endl;
    unload();
    return false;
  }

  {
    std::lock_guard<std::mutex> metaLock(m_metaMutex);
    if (m_plugin->desc) {
      m_meta.id = m_plugin->desc->id ? m_plugin->desc->id : targetId;
      m_meta.name = m_plugin->desc->name ? m_plugin->desc->name : targetId;
      m_meta.vendor =
          m_plugin->desc->vendor ? m_plugin->desc->vendor : "Unknown";
      m_meta.version =
          m_plugin->desc->version ? m_plugin->desc->version : "1.0.0";
      m_meta.description =
          m_plugin->desc->description ? m_plugin->desc->description : "";
    } else {
      m_meta.id = targetId;
      m_meta.name = targetId;
    }
    m_meta.libraryPath = libraryPath;
    m_meta.enabled = false;
    m_meta.isLoaded = true;

    if (m_plugin->get_extension) {
      m_gui = static_cast<const clap_plugin_gui_t *>(
          m_plugin->get_extension(m_plugin, CLAP_EXT_GUI));
      m_timerSupport = static_cast<const clap_plugin_timer_support_t *>(
          m_plugin->get_extension(m_plugin, CLAP_EXT_TIMER_SUPPORT));
      m_meta.hasGui = (m_gui != nullptr);
    }
  }

  if (m_plugin->activate) {
    m_plugin->activate(m_plugin, static_cast<double>(m_sampleRate), 64,
                       m_maxFrames);
  }
  if (m_plugin->start_processing) {
    m_plugin->start_processing(m_plugin);
  }

  // Pre-allocate de-interleaving planar buffers for up to 8 channels
  m_channelInBuffers.resize(8);
  m_channelOutBuffers.resize(8);
  m_channelInPtrs.resize(8);
  m_channelOutPtrs.resize(8);
  for (size_t c = 0; c < 8; ++c) {
    m_channelInBuffers[c].resize(m_maxFrames, 0.0f);
    m_channelOutBuffers[c].resize(m_maxFrames, 0.0f);
    m_channelInPtrs[c] = m_channelInBuffers[c].data();
    m_channelOutPtrs[c] = m_channelOutBuffers[c].data();
  }

  m_isLoaded.store(true, std::memory_order_release);
  return true;
}

void ClapPluginHost::unload() {
  m_isLoaded.store(false, std::memory_order_release);
  m_enabled.store(false, std::memory_order_release);

  {
    std::lock_guard<std::mutex> guiLock(m_guiMutex);
    for (auto &t : m_timers) {
      if (t.second) {
        t.second->stop();
      }
    }
    m_timers.clear();

    if (m_gui && m_guiCreated.load(std::memory_order_relaxed)) {
      if (m_guiVisible.load(std::memory_order_relaxed) && m_gui->hide) {
        m_gui->hide(m_plugin);
      }
      if (m_gui->destroy) {
        m_gui->destroy(m_plugin);
      }
      m_guiCreated.store(false, std::memory_order_release);
      m_guiVisible.store(false, std::memory_order_release);
    }
  }

  {
    std::lock_guard<std::mutex> audioLock(m_audioMutex);
    if (m_plugin) {
      if (m_plugin->stop_processing) {
        m_plugin->stop_processing(m_plugin);
      }
      if (m_plugin->deactivate) {
        m_plugin->deactivate(m_plugin);
      }
      if (m_plugin->destroy) {
        m_plugin->destroy(m_plugin);
      }
      m_plugin = nullptr;
      m_gui = nullptr;
      m_timerSupport = nullptr;
    }

    if (m_entry && m_entry->deinit) {
      m_entry->deinit();
      m_entry = nullptr;
    }

    if (m_libHandle) {
      closeSharedLibrary(m_libHandle);
      m_libHandle = nullptr;
    }
  }

  {
    std::lock_guard<std::mutex> metaLock(m_metaMutex);
    m_meta.isLoaded = false;
  }
}

bool ClapPluginHost::isLoaded() const {
  return m_isLoaded.load(std::memory_order_acquire);
}

ClapPluginMetadata ClapPluginHost::getMetadata() const {
  std::lock_guard<std::mutex> lock(m_metaMutex);
  ClapPluginMetadata copy = m_meta;
  copy.enabled = m_enabled.load(std::memory_order_relaxed);
  copy.isLoaded = m_isLoaded.load(std::memory_order_relaxed);
  return copy;
}

void ClapPluginHost::setEnabled(bool enabled) {
  m_enabled.store(enabled, std::memory_order_release);
}

bool ClapPluginHost::isEnabled() const {
  return m_enabled.load(std::memory_order_acquire);
}

bool ClapPluginHost::showWindow() {
  std::lock_guard<std::mutex> lock(m_guiMutex);
  if (!m_plugin || !m_gui) {
    return false;
  }

  if (!m_guiCreated.load(std::memory_order_acquire)) {
    const char *api = nullptr;
#if defined(__APPLE__)
    api = CLAP_WINDOW_API_COCOA;
#elif defined(_WIN32)
    api = CLAP_WINDOW_API_WIN32;
#else
    api = CLAP_WINDOW_API_X11;
#endif

    bool isFloating = true;
    if (m_gui->get_preferred_api) {
      m_gui->get_preferred_api(m_plugin, &api, &isFloating);
    }

    if (m_gui->create) {
      bool created = m_gui->create(m_plugin, api, true);
      m_guiCreated.store(created, std::memory_order_release);
    }

    if (m_guiCreated.load(std::memory_order_acquire)) {
      if (m_gui->suggest_title) {
        std::string title;
        {
          std::lock_guard<std::mutex> metaLock(m_metaMutex);
          title = m_meta.name;
        }
        m_gui->suggest_title(m_plugin, title.c_str());
      }
      // Floating top-level windows do not set transient parent.
      // This ensures window managers provide independent minimize/close
      // controls without locking or capturing focus from the main player
      // window.
    }
  }

  if (m_guiCreated.load(std::memory_order_acquire) && m_gui->show) {
    bool ok = m_gui->show(m_plugin);
    m_guiVisible.store(ok, std::memory_order_release);
    if (ok) {
      notifyWindowVisibility(true);
    }
    return ok;
  }

  return false;
}

bool ClapPluginHost::hideWindow() {
  std::lock_guard<std::mutex> lock(m_guiMutex);
  if (!m_plugin || !m_gui || !m_guiCreated.load(std::memory_order_relaxed)) {
    return false;
  }
  if (m_gui->hide) {
    m_gui->hide(m_plugin);
    m_guiVisible.store(false, std::memory_order_release);
    notifyWindowVisibility(false);
    return true;
  }
  return false;
}

void ClapPluginHost::destroyGui() {
  std::lock_guard<std::mutex> lock(m_guiMutex);
  if (m_plugin && m_gui && m_guiCreated.load(std::memory_order_relaxed)) {
    if (m_gui->destroy) {
      m_gui->destroy(m_plugin);
    }
    m_guiCreated.store(false, std::memory_order_release);
    m_guiVisible.store(false, std::memory_order_release);
    notifyWindowVisibility(false);
  }
}

void ClapPluginHost::notifyWindowVisibility(bool visible) {
  std::string id;
  {
    std::lock_guard<std::mutex> metaLock(m_metaMutex);
    id = m_meta.id;
  }
  if (m_windowVisibilityCallback) {
    m_windowVisibilityCallback(id, visible);
  }
}

bool ClapPluginHost::isWindowVisible() const {
  return m_guiVisible.load(std::memory_order_acquire);
}

void ClapPluginHost::process(float *buffer, size_t frames, uint32_t sampleRate,
                             uint8_t channels) {
  if (!buffer || frames == 0)
    return;

  if (!m_enabled.load(std::memory_order_relaxed) ||
      !m_isLoaded.load(std::memory_order_relaxed) || !m_plugin) {
    return;
  }

  if (sampleRate != m_sampleRate && sampleRate > 0) {
    std::lock_guard<std::mutex> lock(m_audioMutex);
    m_sampleRate = sampleRate;
    if (m_plugin->stop_processing)
      m_plugin->stop_processing(m_plugin);
    if (m_plugin->deactivate)
      m_plugin->deactivate(m_plugin);
    if (m_plugin->activate)
      m_plugin->activate(m_plugin, static_cast<double>(m_sampleRate), 64,
                         m_maxFrames);
    if (m_plugin->start_processing)
      m_plugin->start_processing(m_plugin);
  }

  const uint8_t activeChannels = std::clamp<uint8_t>(channels, 1, 8);
  if (frames > m_maxFrames) {
    std::lock_guard<std::mutex> lock(m_audioMutex);
    for (size_t c = 0; c < 8; ++c) {
      m_channelInBuffers[c].resize(frames);
      m_channelOutBuffers[c].resize(frames);
      m_channelInPtrs[c] = m_channelInBuffers[c].data();
      m_channelOutPtrs[c] = m_channelOutBuffers[c].data();
    }
    m_maxFrames = static_cast<uint32_t>(frames);
  }

  // De-interleave interleaved PCM into planar channel buffers
  for (size_t f = 0; f < frames; ++f) {
    for (size_t c = 0; c < activeChannels; ++c) {
      m_channelInBuffers[c][f] = buffer[f * channels + c];
    }
  }

  clap_audio_buffer_t inBuf{};
  inBuf.data32 = m_channelInPtrs.data();
  inBuf.channel_count = activeChannels;
  inBuf.constant_mask = 0;
  inBuf.latency = 0;

  clap_audio_buffer_t outBuf{};
  outBuf.data32 = m_channelOutPtrs.data();
  outBuf.channel_count = activeChannels;
  outBuf.constant_mask = 0;
  outBuf.latency = 0;

  clap_input_events_t inEvents{};
  inEvents.ctx = nullptr;
  inEvents.size = &emptyEventsSize;
  inEvents.get = &emptyEventsGet;

  clap_output_events_t outEvents{};
  outEvents.ctx = nullptr;
  outEvents.try_push = &emptyEventsPush;

  clap_process_t processData{};
  processData.steady_time = -1;
  processData.frames_count = static_cast<uint32_t>(frames);
  processData.transport = nullptr;
  processData.audio_inputs = &inBuf;
  processData.audio_outputs = &outBuf;
  processData.audio_inputs_count = 1;
  processData.audio_outputs_count = 1;
  processData.in_events = &inEvents;
  processData.out_events = &outEvents;

  if (m_plugin->process) {
    clap_process_status status = m_plugin->process(m_plugin, &processData);
    if (status != CLAP_PROCESS_ERROR) {
      // Interleave processed planar samples back into buffer
      for (size_t f = 0; f < frames; ++f) {
        for (size_t c = 0; c < activeChannels; ++c) {
          buffer[f * channels + c] = m_channelOutBuffers[c][f];
        }
      }
    }
  }
}

const void *ClapPluginHost::hostGetExtension(const clap_host_t *host,
                                             const char *extensionId) {
  if (!host || !host->host_data || !extensionId)
    return nullptr;
  auto *self = static_cast<ClapPluginHost *>(host->host_data);

  if (std::strcmp(extensionId, CLAP_EXT_GUI) == 0) {
    return &self->m_clapHostGui;
  }
  if (std::strcmp(extensionId, CLAP_EXT_THREAD_CHECK) == 0) {
    return &self->m_clapHostThreadCheck;
  }
  if (std::strcmp(extensionId, CLAP_EXT_TIMER_SUPPORT) == 0) {
    return &self->m_clapHostTimerSupport;
  }
  return nullptr;
}

void ClapPluginHost::hostRequestRestart(const clap_host_t *) {}
void ClapPluginHost::hostRequestProcess(const clap_host_t *) {}

void ClapPluginHost::hostRequestCallback(const clap_host_t *host) {
  if (!host || !host->host_data)
    return;
  auto *self = static_cast<ClapPluginHost *>(host->host_data);
  if (qApp) {
    QMetaObject::invokeMethod(
        qApp,
        [self]() {
          if (self->m_isLoaded.load(std::memory_order_acquire)) {
            std::lock_guard<std::mutex> lock(self->m_guiMutex);
            if (self->m_plugin && self->m_plugin->on_main_thread) {
              self->m_plugin->on_main_thread(self->m_plugin);
            }
          }
        },
        Qt::QueuedConnection);
  }
}

void ClapPluginHost::hostGuiResizeHintsChanged(const clap_host_t *) {}
bool ClapPluginHost::hostGuiRequestResize(const clap_host_t *, uint32_t,
                                          uint32_t) {
  return true;
}
bool ClapPluginHost::hostGuiRequestShow(const clap_host_t *host) {
  if (!host || !host->host_data)
    return false;
  auto *self = static_cast<ClapPluginHost *>(host->host_data);
  if (qApp) {
    QMetaObject::invokeMethod(
        qApp, [self]() { self->showWindow(); }, Qt::QueuedConnection);
  }
  return true;
}

bool ClapPluginHost::hostGuiRequestHide(const clap_host_t *host) {
  if (!host || !host->host_data)
    return false;
  auto *self = static_cast<ClapPluginHost *>(host->host_data);
  self->m_guiVisible.store(false, std::memory_order_release);
  if (qApp) {
    QMetaObject::invokeMethod(
        qApp, [self]() { self->hideWindow(); }, Qt::QueuedConnection);
  }
  return true;
}

void ClapPluginHost::hostGuiClosed(const clap_host_t *host, bool wasDestroyed) {
  if (!host || !host->host_data)
    return;
  auto *self = static_cast<ClapPluginHost *>(host->host_data);
  self->m_guiVisible.store(false, std::memory_order_release);
  if (qApp) {
    QMetaObject::invokeMethod(
        qApp,
        [self, wasDestroyed]() {
          if (wasDestroyed) {
            self->destroyGui();
          } else {
            self->hideWindow();
          }
        },
        Qt::QueuedConnection);
  }
}

bool ClapPluginHost::hostIsMainThread(const clap_host_t *) {
  return QThread::currentThread() == QCoreApplication::instance()->thread();
}

bool ClapPluginHost::hostIsAudioThread(const clap_host_t *) {
  return QThread::currentThread() != QCoreApplication::instance()->thread();
}

bool ClapPluginHost::hostRegisterTimer(const clap_host_t *host,
                                       uint32_t period_ms, clap_id *timer_id) {
  if (!host || !host->host_data || !timer_id)
    return false;
  auto *self = static_cast<ClapPluginHost *>(host->host_data);
  clap_id id = self->m_nextTimerId++;
  *timer_id = id;

  if (qApp) {
    QMetaObject::invokeMethod(
        qApp,
        [self, id, period_ms]() {
          auto timer = std::make_unique<QTimer>();
          timer->setInterval(std::max<uint32_t>(16, period_ms));
          QObject::connect(timer.get(), &QTimer::timeout, [self, id]() {
            if (self->m_timerSupport && self->m_timerSupport->on_timer &&
                self->m_isLoaded.load(std::memory_order_relaxed)) {
              self->m_timerSupport->on_timer(self->m_plugin, id);
            }
          });
          timer->start();
          self->m_timers[id] = std::move(timer);
        },
        Qt::QueuedConnection);
  }
  return true;
}

bool ClapPluginHost::hostUnregisterTimer(const clap_host_t *host,
                                         clap_id timer_id) {
  if (!host || !host->host_data)
    return false;
  auto *self = static_cast<ClapPluginHost *>(host->host_data);
  if (qApp) {
    QMetaObject::invokeMethod(
        qApp,
        [self, timer_id]() {
          auto it = self->m_timers.find(timer_id);
          if (it != self->m_timers.end()) {
            it->second->stop();
            self->m_timers.erase(it);
          }
        },
        Qt::QueuedConnection);
  }
  return true;
}

} // namespace adapters
