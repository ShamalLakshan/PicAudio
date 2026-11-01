#include "AlsaAudioSink.hpp"
#include "audio/AudioThreadPriority.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>

namespace adapters {

AlsaAudioSink::AlsaAudioSink() = default;

AlsaAudioSink::~AlsaAudioSink() { close(); }

bool AlsaAudioSink::open(const AudioSinkConfig &config) {
  std::lock_guard<std::mutex> lock(m_mutex);
  close();

#if defined(PICAUDIO_ENABLE_ALSA)
  std::string deviceName =
      config.deviceId.empty() ? "default" : config.deviceId;

  int err =
      snd_pcm_open(&m_pcm, deviceName.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
  if (err < 0) {
    // If opening direct hw: failed (e.g. busy), try plughw: fallback
    if (deviceName.rfind("hw:", 0) == 0) {
      std::string plugDevice = "plug" + deviceName;
      err =
          snd_pcm_open(&m_pcm, plugDevice.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
      if (err >= 0) {
        deviceName = plugDevice;
      }
    }
  }

  if (err < 0 || !m_pcm) {
    std::cerr << "AlsaAudioSink: Failed to open device '" << deviceName
              << "': " << snd_strerror(err) << std::endl;
    return false;
  }

  snd_pcm_hw_params_t *params = nullptr;
  snd_pcm_hw_params_alloca(&params);
  snd_pcm_hw_params_any(m_pcm, params);

  err = snd_pcm_hw_params_set_access(m_pcm, params,
                                     SND_PCM_ACCESS_RW_INTERLEAVED);
  if (err < 0) {
    std::cerr << "AlsaAudioSink: Interleaved access not supported: "
              << snd_strerror(err) << std::endl;
    snd_pcm_close(m_pcm);
    m_pcm = nullptr;
    return false;
  }

  // Select PCM format based on bit depth and float flag
  m_format = SND_PCM_FORMAT_S16_LE;
  if (config.isFloat) {
    m_format = SND_PCM_FORMAT_FLOAT_LE;
    m_frameBytes = sizeof(float) * config.channels;
  } else if (config.bitDepth == 32) {
    m_format = SND_PCM_FORMAT_S32_LE;
    m_frameBytes = 4 * config.channels;
  } else if (config.bitDepth == 24) {
    // Test 24-bit formats
    if (snd_pcm_hw_params_test_format(m_pcm, params, SND_PCM_FORMAT_S24_LE) ==
        0) {
      m_format = SND_PCM_FORMAT_S24_LE;
      m_frameBytes = 4 * config.channels;
    } else if (snd_pcm_hw_params_test_format(m_pcm, params,
                                             SND_PCM_FORMAT_S24_3LE) == 0) {
      m_format = SND_PCM_FORMAT_S24_3LE;
      m_frameBytes = 3 * config.channels;
    } else {
      m_format = SND_PCM_FORMAT_S32_LE;
      m_frameBytes = 4 * config.channels;
    }
  } else {
    m_format = SND_PCM_FORMAT_S16_LE;
    m_frameBytes = 2 * config.channels;
  }

  err = snd_pcm_hw_params_set_format(m_pcm, params, m_format);
  if (err < 0) {
    // Fallback to S16_LE if higher depth is unsupported
    m_format = SND_PCM_FORMAT_S16_LE;
    m_frameBytes = 2 * config.channels;
    snd_pcm_hw_params_set_format(m_pcm, params, m_format);
  }

  // Set channel count (exact or nearest hardware supported)
  unsigned int targetChannels = config.channels;
  err = snd_pcm_hw_params_set_channels_near(m_pcm, params, &targetChannels);
  if (err < 0) {
    std::cerr << "AlsaAudioSink: Failed to set channels "
              << static_cast<int>(config.channels) << ": " << snd_strerror(err)
              << std::endl;
    snd_pcm_close(m_pcm);
    m_pcm = nullptr;
    return false;
  }

  // Recalculate frame bytes if channels were constrained by hardware
  size_t bytesPerSample = (m_format == SND_PCM_FORMAT_S16_LE) ? 2 : (m_format == SND_PCM_FORMAT_S24_3LE ? 3 : 4);
  m_frameBytes = bytesPerSample * targetChannels;

  // Hardware sample rate matching
  unsigned int targetRate = config.sampleRate;
  int dir = 0;
  err = snd_pcm_hw_params_set_rate_near(m_pcm, params, &targetRate, &dir);
  if (err < 0) {
    std::cerr << "AlsaAudioSink: Failed to set rate " << config.sampleRate
              << ": " << snd_strerror(err) << std::endl;
    snd_pcm_close(m_pcm);
    m_pcm = nullptr;
    return false;
  }

  m_isBitPerfect = (targetRate == config.sampleRate);

  // Set buffer time and period size
  unsigned int bufferTimeUs =
      std::clamp(config.bufferLatencyMs, 10u, 500u) * 1000;
  err = snd_pcm_hw_params_set_buffer_time_near(m_pcm, params, &bufferTimeUs,
                                               &dir);
  unsigned int periodTimeUs = bufferTimeUs / 4;
  snd_pcm_hw_params_set_period_time_near(m_pcm, params, &periodTimeUs, &dir);

  err = snd_pcm_hw_params(m_pcm, params);
  if (err < 0) {
    std::cerr << "AlsaAudioSink: Failed to apply hw params: "
              << snd_strerror(err) << std::endl;
    snd_pcm_close(m_pcm);
    m_pcm = nullptr;
    return false;
  }

  snd_pcm_uframes_t bufferSize = 0;
  snd_pcm_hw_params_get_buffer_size(params, &bufferSize);
  m_latencyMs = static_cast<uint32_t>((bufferSize * 1000) / targetRate);

  snd_pcm_prepare(m_pcm);
  m_config = config;
  m_isOpen.store(true);
  m_isPaused.store(false);
  return true;

#else
  (void)config;
  return false;
#endif
}

void AlsaAudioSink::close() {
#if defined(PICAUDIO_ENABLE_ALSA)
  if (m_pcm) {
    snd_pcm_drop(m_pcm);
    snd_pcm_close(m_pcm);
    m_pcm = nullptr;
  }
#endif
  m_isOpen.store(false);
  m_isPaused.store(false);
}

bool AlsaAudioSink::isOpen() const { return m_isOpen.load(); }

size_t AlsaAudioSink::write(const void *buffer, size_t frames) {
#if defined(PICAUDIO_ENABLE_ALSA)
  if (!m_pcm || !buffer || frames == 0 || m_isPaused.load()) {
    return 0;
  }

  snd_pcm_sframes_t written = snd_pcm_writei(m_pcm, buffer, frames);
  if (written < 0) {
    if (written == -EPIPE) {
      m_xrunCount.fetch_add(1, std::memory_order_relaxed);
      snd_pcm_prepare(m_pcm);
      written = snd_pcm_writei(m_pcm, buffer, frames);
    } else if (written == -ESTRPIPE) {
      while ((written = snd_pcm_resume(m_pcm)) == -EAGAIN) {
        // Sleep briefly waiting for device to resume
        struct timespec ts{0, 10000000}; // 10ms
        nanosleep(&ts, nullptr);
      }
      if (written < 0) {
        snd_pcm_prepare(m_pcm);
      }
      written = snd_pcm_writei(m_pcm, buffer, frames);
    }
  }

  return (written > 0) ? static_cast<size_t>(written) : 0;
#else
  (void)buffer;
  (void)frames;
  return 0;
#endif
}

void AlsaAudioSink::pause() {
#if defined(PICAUDIO_ENABLE_ALSA)
  if (m_pcm && m_isOpen.load() && !m_isPaused.load()) {
    snd_pcm_pause(m_pcm, 1);
    m_isPaused.store(true);
  }
#endif
}

void AlsaAudioSink::resume() {
#if defined(PICAUDIO_ENABLE_ALSA)
  if (m_pcm && m_isOpen.load() && m_isPaused.load()) {
    snd_pcm_pause(m_pcm, 0);
    m_isPaused.store(false);
  }
#endif
}

void AlsaAudioSink::flush() {
#if defined(PICAUDIO_ENABLE_ALSA)
  if (m_pcm) {
    snd_pcm_drop(m_pcm);
    snd_pcm_prepare(m_pcm);
  }
#endif
}

uint32_t AlsaAudioSink::getLatencyMs() const { return m_latencyMs; }

uint64_t AlsaAudioSink::getXrunCount() const {
  return m_xrunCount.load(std::memory_order_relaxed);
}

bool AlsaAudioSink::isBitPerfect() const { return m_isBitPerfect; }

std::string AlsaAudioSink::getName() const {
  return "ALSA Direct Hardware (hw:)";
}

std::vector<AudioDeviceInfo> AlsaAudioSink::enumerateDevices() {
  std::vector<AudioDeviceInfo> devices;

  AudioDeviceInfo defDevice;
  defDevice.id = "default";
  defDevice.name = "System Default ALSA";
  defDevice.description = "ALSA default routing";
  defDevice.isDefault = true;
  devices.push_back(defDevice);

#if defined(PICAUDIO_ENABLE_ALSA)
  void **hints = nullptr;
  if (snd_device_name_hint(-1, "pcm", &hints) == 0 && hints) {
    for (void **h = hints; *h != nullptr; ++h) {
      char *name = snd_device_name_get_hint(*h, "NAME");
      char *desc = snd_device_name_get_hint(*h, "DESC");
      char *ioid = snd_device_name_get_hint(*h, "IOID");

      // Filter: only playback devices
      if (ioid && std::string(ioid) != "Output") {
        if (name)
          free(name);
        if (desc)
          free(desc);
        if (ioid)
          free(ioid);
        continue;
      }

      if (name) {
        std::string nameStr(name);
        // Expose hw: and plughw: direct devices
        if (nameStr.rfind("hw:", 0) == 0 || nameStr.rfind("plughw:", 0) == 0) {
          AudioDeviceInfo info;
          info.id = nameStr;
          std::string descStr = desc ? desc : nameStr;
          // Clean up multi-line descriptions from ALSA
          std::replace(descStr.begin(), descStr.end(), '\n', ' ');
          info.description = descStr;
          info.name = descStr + " (" + nameStr + ")";
          info.isDefault = false;

          // Avoid duplicates
          if (std::find_if(devices.begin(), devices.end(),
                           [&](const AudioDeviceInfo &d) {
                             return d.id == info.id;
                           }) == devices.end()) {
            devices.push_back(info);
          }
        }
      }

      if (name)
        free(name);
      if (desc)
        free(desc);
      if (ioid)
        free(ioid);
    }
    snd_device_name_free_hint(hints);
  }
#endif

  return devices;
}

} // namespace adapters
