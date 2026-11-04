#include "PipeWireAudioSink.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>

namespace adapters {

#if defined(PICAUDIO_ENABLE_PIPEWIRE)
static void onProcessCallback(void *userdata) {
  auto *self = static_cast<PipeWireAudioSink *>(userdata);
  if (self) {
    self->processAudio();
  }
}

static const pw_stream_events kStreamEvents = {
    .version = PW_VERSION_STREAM_EVENTS,
    .destroy = nullptr,
    .state_changed = nullptr,
    .control_info = nullptr,
    .io_changed = nullptr,
    .param_changed = nullptr,
    .add_buffer = nullptr,
    .remove_buffer = nullptr,
    .process = onProcessCallback,
    .drained = nullptr,
    .command = nullptr,
    .trigger_done = nullptr,
};
#endif

PipeWireAudioSink::PipeWireAudioSink() {
#if defined(PICAUDIO_ENABLE_PIPEWIRE)
  static bool pwInitialized = false;
  if (!pwInitialized) {
    pw_init(nullptr, nullptr);
    pwInitialized = true;
  }
#endif
}

PipeWireAudioSink::~PipeWireAudioSink() { close(); }

bool PipeWireAudioSink::open(const AudioSinkConfig &config) {
  std::lock_guard<std::mutex> lock(m_mutex);
  close();

#if defined(PICAUDIO_ENABLE_PIPEWIRE)
  m_config = config;
  m_loop = pw_thread_loop_new("picaudio-pw", nullptr);
  if (!m_loop) {
    std::cerr << "PipeWireAudioSink: Failed to create pw_thread_loop"
              << std::endl;
    return false;
  }

  pw_loop *underlyingLoop = pw_thread_loop_get_loop(m_loop);
  m_context = pw_context_new(underlyingLoop, nullptr, 0);
  if (!m_context) {
    std::cerr << "PipeWireAudioSink: Failed to create pw_context" << std::endl;
    pw_thread_loop_destroy(m_loop);
    m_loop = nullptr;
    return false;
  }

  if (pw_thread_loop_start(m_loop) < 0) {
    std::cerr << "PipeWireAudioSink: Failed to start pw_thread_loop"
              << std::endl;
    pw_context_destroy(m_context);
    pw_thread_loop_destroy(m_loop);
    m_context = nullptr;
    m_loop = nullptr;
    return false;
  }

  pw_thread_loop_lock(m_loop);

  pw_properties *props = pw_properties_new(
      PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY, "Playback",
      PW_KEY_MEDIA_ROLE, "Music", PW_KEY_APP_NAME, "PicAudio", PW_KEY_NODE_NAME,
      "PicAudio Playback", PW_KEY_NODE_DESCRIPTION,
      "PicAudio Bit-Perfect Audio Pipeline", nullptr);

  if (config.bufferLatencyMs > 0) {
    uint32_t quantum = (config.sampleRate * config.bufferLatencyMs) / 1000;
    std::string quantumStr = std::to_string(std::max<uint32_t>(32, quantum));
    pw_properties_set(props, PW_KEY_NODE_LATENCY, quantumStr.c_str());
  }

  m_stream = pw_stream_new_simple(underlyingLoop, "PicAudio Native Stream",
                                  props, &kStreamEvents, this);
  if (!m_stream) {
    std::cerr << "PipeWireAudioSink: Failed to create pw_stream" << std::endl;
    pw_thread_loop_unlock(m_loop);
    close();
    return false;
  }

  uint8_t buffer[1024];
  spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
  spa_audio_info_raw info{};

  if (config.isFloat) {
    info.format = SPA_AUDIO_FORMAT_F32;
    m_frameBytes = sizeof(float) * config.channels;
  } else if (config.bitDepth == 32) {
    info.format = SPA_AUDIO_FORMAT_S32;
    m_frameBytes = 4 * config.channels;
  } else if (config.bitDepth == 24) {
    info.format = SPA_AUDIO_FORMAT_S24_32;
    m_frameBytes = 4 * config.channels;
  } else {
    info.format = SPA_AUDIO_FORMAT_S16;
    m_frameBytes = 2 * config.channels;
  }

  info.rate = config.sampleRate;
  info.channels = config.channels;
  if (config.channels == 1) {
    info.position[0] = SPA_AUDIO_CHANNEL_MONO;
  } else if (config.channels == 2) {
    info.position[0] = SPA_AUDIO_CHANNEL_FL;
    info.position[1] = SPA_AUDIO_CHANNEL_FR;
  } else if (config.channels == 3) {
    info.position[0] = SPA_AUDIO_CHANNEL_FL;
    info.position[1] = SPA_AUDIO_CHANNEL_FR;
    info.position[2] = SPA_AUDIO_CHANNEL_LFE;
  } else if (config.channels == 4) {
    info.position[0] = SPA_AUDIO_CHANNEL_FL;
    info.position[1] = SPA_AUDIO_CHANNEL_FR;
    info.position[2] = SPA_AUDIO_CHANNEL_RL;
    info.position[3] = SPA_AUDIO_CHANNEL_RR;
  } else if (config.channels == 5) {
    info.position[0] = SPA_AUDIO_CHANNEL_FL;
    info.position[1] = SPA_AUDIO_CHANNEL_FR;
    info.position[2] = SPA_AUDIO_CHANNEL_FC;
    info.position[3] = SPA_AUDIO_CHANNEL_RL;
    info.position[4] = SPA_AUDIO_CHANNEL_RR;
  } else if (config.channels == 6) {
    // 5.1 Surround
    info.position[0] = SPA_AUDIO_CHANNEL_FL;
    info.position[1] = SPA_AUDIO_CHANNEL_FR;
    info.position[2] = SPA_AUDIO_CHANNEL_FC;
    info.position[3] = SPA_AUDIO_CHANNEL_LFE;
    info.position[4] = SPA_AUDIO_CHANNEL_RL;
    info.position[5] = SPA_AUDIO_CHANNEL_RR;
  } else if (config.channels >= 8) {
    // 7.1 Surround
    info.position[0] = SPA_AUDIO_CHANNEL_FL;
    info.position[1] = SPA_AUDIO_CHANNEL_FR;
    info.position[2] = SPA_AUDIO_CHANNEL_FC;
    info.position[3] = SPA_AUDIO_CHANNEL_LFE;
    info.position[4] = SPA_AUDIO_CHANNEL_RL;
    info.position[5] = SPA_AUDIO_CHANNEL_RR;
    info.position[6] = SPA_AUDIO_CHANNEL_SL;
    info.position[7] = SPA_AUDIO_CHANNEL_SR;
  }

  const spa_pod *params[1];
  params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &info);

  int res = pw_stream_connect(
      m_stream, PW_DIRECTION_OUTPUT, PW_ID_ANY,
      static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT |
                                   PW_STREAM_FLAG_MAP_BUFFERS |
                                   PW_STREAM_FLAG_RT_PROCESS),
      params, 1);

  pw_thread_loop_unlock(m_loop);

  if (res < 0) {
    std::cerr << "PipeWireAudioSink: Failed to connect stream: " << res
              << std::endl;
    close();
    return false;
  }

  m_latencyMs = config.bufferLatencyMs;
  m_isOpen.store(true);
  m_isPaused.store(false);
  return true;

#else
  (void)config;
  return false;
#endif
}

void PipeWireAudioSink::close() {
#if defined(PICAUDIO_ENABLE_PIPEWIRE)
  if (m_loop) {
    pw_thread_loop_stop(m_loop);
  }
  if (m_stream) {
    pw_stream_destroy(m_stream);
    m_stream = nullptr;
  }
  if (m_context) {
    pw_context_destroy(m_context);
    m_context = nullptr;
  }
  if (m_loop) {
    pw_thread_loop_destroy(m_loop);
    m_loop = nullptr;
  }
#endif
  m_isOpen.store(false);
  m_isPaused.store(false);
  m_ringBuffer.reset();
}

bool PipeWireAudioSink::isOpen() const { return m_isOpen.load(); }

size_t PipeWireAudioSink::write(const void *buffer, size_t frames) {
  if (!m_isOpen.load() || !buffer || frames == 0) {
    return 0;
  }
  const size_t bytesToWrite = frames * m_frameBytes;
  const size_t written = m_ringBuffer.write(
      reinterpret_cast<const uint8_t *>(buffer), bytesToWrite);
  return written / m_frameBytes;
}

void PipeWireAudioSink::pause() {
#if defined(PICAUDIO_ENABLE_PIPEWIRE)
  if (m_loop && m_stream && m_isOpen.load() && !m_isPaused.load()) {
    pw_thread_loop_lock(m_loop);
    pw_stream_set_active(m_stream, false);
    pw_thread_loop_unlock(m_loop);
    m_isPaused.store(true);
  }
#endif
}

void PipeWireAudioSink::resume() {
#if defined(PICAUDIO_ENABLE_PIPEWIRE)
  if (m_loop && m_stream && m_isOpen.load() && m_isPaused.load()) {
    pw_thread_loop_lock(m_loop);
    pw_stream_set_active(m_stream, true);
    pw_thread_loop_unlock(m_loop);
    m_isPaused.store(false);
  }
#endif
}

void PipeWireAudioSink::flush() { m_ringBuffer.reset(); }

uint32_t PipeWireAudioSink::getLatencyMs() const { return m_latencyMs; }

uint64_t PipeWireAudioSink::getXrunCount() const {
  return m_xrunCount.load(std::memory_order_relaxed);
}

bool PipeWireAudioSink::isBitPerfect() const { return true; }

std::string PipeWireAudioSink::getName() const {
  return "Linux PipeWire Lock-Free Client";
}

std::vector<AudioDeviceInfo> PipeWireAudioSink::enumerateDevices() {
  std::vector<AudioDeviceInfo> devices;
  AudioDeviceInfo def;
  def.id = "pipewire-default";
  def.name = "PipeWire Default Audio Stream";
  def.description = "PipeWire native low-latency routing graph";
  def.isDefault = true;
  devices.push_back(def);
  return devices;
}

void PipeWireAudioSink::processAudio() {
#if defined(PICAUDIO_ENABLE_PIPEWIRE)
  if (!m_stream) {
    return;
  }

  pw_buffer *buf = pw_stream_dequeue_buffer(m_stream);
  if (!buf) {
    return;
  }

  spa_data *d = &buf->buffer->datas[0];
  if (!d || !d->data) {
    pw_stream_queue_buffer(m_stream, buf);
    return;
  }

  const uint32_t maxBytes = d->maxsize;
  uint32_t reqBytes = maxBytes;
  if (buf->requested > 0) {
    reqBytes = std::min(maxBytes,
                        static_cast<uint32_t>(buf->requested * m_frameBytes));
  }

  size_t bytesRead =
      m_ringBuffer.read(reinterpret_cast<uint8_t *>(d->data), reqBytes);

  if (bytesRead < reqBytes) {
    if (!m_isPaused.load()) {
      m_xrunCount.fetch_add(1, std::memory_order_relaxed);
    }
    std::memset(reinterpret_cast<uint8_t *>(d->data) + bytesRead, 0,
                reqBytes - bytesRead);
  }

  d->chunk->offset = 0;
  d->chunk->stride = static_cast<int32_t>(m_frameBytes);
  d->chunk->size = reqBytes;

  pw_stream_queue_buffer(m_stream, buf);
#endif
}

} // namespace adapters
