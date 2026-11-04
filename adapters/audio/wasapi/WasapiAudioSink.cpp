#include "WasapiAudioSink.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>

#if defined(_WIN32)
#include <functiondiscoverykeys_devpkey.h>
#include <ksmedia.h>
#endif

namespace adapters {

WasapiAudioSink::WasapiAudioSink() {
#if defined(_WIN32)
  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  m_comInitialized = SUCCEEDED(hr);
#endif
}

WasapiAudioSink::~WasapiAudioSink() {
  close();
#if defined(_WIN32)
  if (m_comInitialized) {
    CoUninitialize();
  }
#endif
}

bool WasapiAudioSink::open(const AudioSinkConfig &config) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  close();

  m_activeConfig = config;
  const uint8_t ch = config.channels > 0 ? config.channels : 2;
  const size_t bytesPerSample =
      config.isFloat ? sizeof(float) : (config.bitDepth / 8);
  m_frameBytes = ch * bytesPerSample;
  m_latencyMs = config.bufferLatencyMs > 0 ? config.bufferLatencyMs : 10;

#if defined(_WIN32)
  HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                CLSCTX_ALL, IID_PPV_ARGS(&m_enumerator));
  if (FAILED(hr) || !m_enumerator) {
    std::cerr << "WasapiAudioSink: Failed to create device enumerator"
              << std::endl;
    return false;
  }

  if (config.deviceId.empty() || config.deviceId == "default") {
    hr = m_enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &m_device);
  } else {
    std::wstring wid(config.deviceId.begin(), config.deviceId.end());
    hr = m_enumerator->GetDevice(wid.c_str(), &m_device);
    if (FAILED(hr)) {
      hr = m_enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia,
                                                 &m_device);
    }
  }

  if (FAILED(hr) || !m_device) {
    std::cerr << "WasapiAudioSink: Failed to activate audio endpoint"
              << std::endl;
    return false;
  }

  hr = m_device->Activate(
      __uuidof(IAudioClient), CLSCTX_ALL, nullptr,
      reinterpret_cast<void **>(m_audioClient.GetAddressOf()));
  if (FAILED(hr) || !m_audioClient) {
    std::cerr << "WasapiAudioSink: Failed to activate IAudioClient"
              << std::endl;
    return false;
  }

  // Setup WAVEFORMATEXTENSIBLE matching requested hardware specs
  WAVEFORMATEXTENSIBLE wfe{};
  wfe.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
  wfe.Format.nChannels = ch;
  wfe.Format.nSamplesPerSec = config.sampleRate > 0 ? config.sampleRate : 44100;
  wfe.Format.wBitsPerSample = static_cast<WORD>(bytesPerSample * 8);
  wfe.Format.nBlockAlign = static_cast<WORD>(m_frameBytes);
  wfe.Format.nAvgBytesPerSec =
      wfe.Format.nSamplesPerSec * wfe.Format.nBlockAlign;
  wfe.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
  wfe.Samples.wValidBitsPerSample = wfe.Format.wBitsPerSample;
  wfe.SubFormat = config.isFloat ? KSDATAFORMAT_SUBTYPE_IEEE_FLOAT
                                 : KSDATAFORMAT_SUBTYPE_PCM;

  if (ch == 1) {
    wfe.dwChannelMask = SPEAKER_FRONT_CENTER;
  } else if (ch == 2) {
    wfe.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
  } else if (ch == 6) {
    wfe.dwChannelMask = SPEAKER_5POINT1;
  } else if (ch == 8) {
    wfe.dwChannelMask = SPEAKER_7POINT1_SURROUND;
  } else {
    wfe.dwChannelMask = 0;
  }

  REFERENCE_TIME bufferDuration =
      static_cast<REFERENCE_TIME>(m_latencyMs) * 10000;
  REFERENCE_TIME periodicity = bufferDuration;

  // Try Exclusive Mode with Event-driven callback first
  hr = m_audioClient->Initialize(
      AUDCLNT_SHAREMODE_EXCLUSIVE, AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
      bufferDuration, periodicity, reinterpret_cast<WAVEFORMATEX *>(&wfe),
      nullptr);
  if (SUCCEEDED(hr)) {
    m_isExclusive.store(true);
  } else {
    // Graceful fallback to Shared Mode if exclusive access is denied
    m_isExclusive.store(false);
    WAVEFORMATEX *closestMatch = nullptr;
    hr = m_audioClient->IsFormatSupported(
        AUDCLNT_SHAREMODE_SHARED, reinterpret_cast<WAVEFORMATEX *>(&wfe),
        &closestMatch);
    WAVEFORMATEX *finalFmt =
        (hr == S_OK) ? reinterpret_cast<WAVEFORMATEX *>(&wfe) : closestMatch;

    hr = m_audioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
        bufferDuration, 0,
        finalFmt ? finalFmt : reinterpret_cast<WAVEFORMATEX *>(&wfe), nullptr);
    if (closestMatch) {
      CoTaskMemFree(closestMatch);
    }
  }

  if (FAILED(hr)) {
    std::cerr << "WasapiAudioSink: Audio client initialization failed: 0x"
              << std::hex << hr << std::dec << std::endl;
    return false;
  }

  m_audioEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  hr = m_audioClient->SetEventHandle(m_audioEvent);
  if (FAILED(hr)) {
    std::cerr << "WasapiAudioSink: Failed to set event handle" << std::endl;
    return false;
  }

  hr = m_audioClient->GetBufferSize(&m_bufferFrameCount);
  if (FAILED(hr)) {
    m_bufferFrameCount = (wfe.Format.nSamplesPerSec * m_latencyMs) / 1000;
  }

  hr = m_audioClient->GetService(
      __uuidof(IAudioRenderClient),
      reinterpret_cast<void **>(m_renderClient.GetAddressOf()));
  if (FAILED(hr) || !m_renderClient) {
    std::cerr << "WasapiAudioSink: Failed to get IAudioRenderClient"
              << std::endl;
    return false;
  }

  m_audioClient->Start();
#endif

  m_isOpen.store(true);
  m_isPaused.store(false);
  return true;
}

void WasapiAudioSink::close() {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (!m_isOpen.load()) {
    return;
  }

#if defined(_WIN32)
  if (m_audioClient) {
    m_audioClient->Stop();
    m_audioClient.Reset();
  }
  m_renderClient.Reset();
  m_device.Reset();
  m_enumerator.Reset();

  if (m_audioEvent) {
    CloseHandle(m_audioEvent);
    m_audioEvent = nullptr;
  }
#endif

  m_isOpen.store(false);
  m_isPaused.store(false);
}

bool WasapiAudioSink::isOpen() const {
  return m_isOpen.load(std::memory_order_relaxed);
}

size_t WasapiAudioSink::write(const void *buffer, size_t frames) {
  if (!m_isOpen.load(std::memory_order_relaxed) ||
      m_isPaused.load(std::memory_order_relaxed) || !buffer || frames == 0) {
    return 0;
  }

  std::lock_guard<std::recursive_mutex> lock(m_mutex);

#if defined(_WIN32)
  if (!m_renderClient || !m_audioClient) {
    return 0;
  }

  UINT32 padding = 0;
  if (!m_isExclusive.load()) {
    m_audioClient->GetCurrentPadding(&padding);
  }

  UINT32 availableFrames =
      (m_bufferFrameCount > padding) ? (m_bufferFrameCount - padding) : 0;
  if (availableFrames == 0) {
    // Wait briefly for buffer vacancy event
    if (m_audioEvent) {
      WaitForSingleObject(m_audioEvent, m_latencyMs);
    }
    if (!m_isExclusive.load()) {
      m_audioClient->GetCurrentPadding(&padding);
      availableFrames =
          (m_bufferFrameCount > padding) ? (m_bufferFrameCount - padding) : 0;
    } else {
      availableFrames = m_bufferFrameCount;
    }
  }

  size_t framesToWrite = std::min(static_cast<size_t>(availableFrames), frames);
  if (framesToWrite == 0) {
    m_xrunCount.fetch_add(1, std::memory_order_relaxed);
    return 0;
  }

  BYTE *dstData = nullptr;
  HRESULT hr =
      m_renderClient->GetBuffer(static_cast<UINT32>(framesToWrite), &dstData);
  if (SUCCEEDED(hr) && dstData) {
    std::memcpy(dstData, buffer, framesToWrite * m_frameBytes);
    m_renderClient->ReleaseBuffer(static_cast<UINT32>(framesToWrite), 0);
    return framesToWrite;
  } else {
    m_xrunCount.fetch_add(1, std::memory_order_relaxed);
    return 0;
  }
#else
  // Non-Windows simulation: mock successful write
  return frames;
#endif
}

void WasapiAudioSink::pause() {
  m_isPaused.store(true);
#if defined(_WIN32)
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (m_audioClient) {
    m_audioClient->Stop();
  }
#endif
}

void WasapiAudioSink::resume() {
  m_isPaused.store(false);
#if defined(_WIN32)
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (m_audioClient) {
    m_audioClient->Start();
  }
#endif
}

void WasapiAudioSink::flush() {
#if defined(_WIN32)
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (m_audioClient) {
    m_audioClient->Reset();
  }
#endif
}

uint32_t WasapiAudioSink::getLatencyMs() const { return m_latencyMs; }

uint64_t WasapiAudioSink::getXrunCount() const {
  return m_xrunCount.load(std::memory_order_relaxed);
}

bool WasapiAudioSink::isBitPerfect() const {
  return m_isExclusive.load(std::memory_order_relaxed);
}

std::string WasapiAudioSink::getName() const {
  return "Windows WASAPI Exclusive Mode";
}

bool WasapiAudioSink::isExclusiveModeActive() const {
  return m_isExclusive.load(std::memory_order_relaxed);
}

std::vector<AudioDeviceInfo> WasapiAudioSink::enumerateDevices() {
  std::vector<AudioDeviceInfo> devices;

#if defined(_WIN32)
  Microsoft::WRL::ComPtr<IMMDeviceEnumerator> enumerator;
  HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                CLSCTX_ALL, IID_PPV_ARGS(&enumerator));
  if (FAILED(hr) || !enumerator) {
    return devices;
  }

  Microsoft::WRL::ComPtr<IMMDeviceCollection> collection;
  hr =
      enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &collection);
  if (FAILED(hr) || !collection) {
    return devices;
  }

  UINT count = 0;
  collection->GetCount(&count);

  Microsoft::WRL::ComPtr<IMMDevice> defaultDev;
  enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &defaultDev);
  LPWSTR defaultId = nullptr;
  if (defaultDev) {
    defaultDev->GetId(&defaultId);
  }

  for (UINT i = 0; i < count; ++i) {
    Microsoft::WRL::ComPtr<IMMDevice> dev;
    if (FAILED(collection->Item(i, &dev)) || !dev)
      continue;

    LPWSTR strId = nullptr;
    dev->GetId(&strId);
    std::string idStr;
    if (strId) {
      std::wstring wid(strId);
      idStr = std::string(wid.begin(), wid.end());
    }

    std::string nameStr = "Audio Device";
    Microsoft::WRL::ComPtr<IPropertyStore> props;
    if (SUCCEEDED(dev->OpenPropertyStore(STGM_READ, &props)) && props) {
      PROPVARIANT pv;
      PropVariantInit(&pv);
      if (SUCCEEDED(props->GetValue(PKEY_Device_FriendlyName, &pv))) {
        if (pv.vt == VT_LPWSTR && pv.pwszVal) {
          std::wstring wname(pv.pwszVal);
          nameStr = std::string(wname.begin(), wname.end());
        }
      }
      PropVariantClear(&pv);
    }

    bool isDef = (defaultId && strId && wcscmp(defaultId, strId) == 0);
    devices.push_back({idStr, nameStr, "WASAPI Exclusive Output", isDef});

    if (strId) {
      CoTaskMemFree(strId);
    }
  }

  if (defaultId) {
    CoTaskMemFree(defaultId);
  }
#else
  devices.push_back({"wasapi:default", "Default Audio Endpoint (WASAPI)",
                     "Windows Audio Session API", true});
#endif

  return devices;
}

} // namespace adapters
