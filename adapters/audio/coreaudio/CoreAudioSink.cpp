#include "CoreAudioSink.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>

#if defined(__APPLE__)
#include <unistd.h>
#endif

namespace adapters {

CoreAudioSink::CoreAudioSink() = default;

CoreAudioSink::~CoreAudioSink() { close(); }

bool CoreAudioSink::open(const AudioSinkConfig &config) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  close();

  m_activeConfig = config;
  const uint8_t ch = config.channels > 0 ? config.channels : 2;
  const size_t bytesPerSample =
      config.isFloat ? sizeof(float) : (config.bitDepth / 8);
  m_frameBytes = ch * bytesPerSample;
  m_latencyMs = config.bufferLatencyMs > 0 ? config.bufferLatencyMs : 15;

#if defined(__APPLE__)
  AudioComponentDescription desc{};
  desc.componentType = kAudioUnitType_Output;
  desc.componentSubType = kAudioUnitSubType_HALOutput;
  desc.componentManufacturer = kAudioUnitManufacturer_Apple;

  AudioComponent comp = AudioComponentFindNext(nullptr, &desc);
  if (!comp) {
    std::cerr << "CoreAudioSink: Output AudioComponent not found" << std::endl;
    return false;
  }

  OSStatus status = AudioComponentInstanceNew(comp, &m_audioUnit);
  if (status != noErr || !m_audioUnit) {
    std::cerr << "CoreAudioSink: Failed to create AudioUnit instance: "
              << status << std::endl;
    return false;
  }

  // Determine target audio device
  UInt32 dataSize = sizeof(AudioDeviceID);
  if (config.deviceId.empty() || config.deviceId == "default") {
    AudioObjectPropertyAddress addr = {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
    status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &addr, 0,
                                        nullptr, &dataSize, &m_deviceID);
  }

  if (m_deviceID != kAudioObjectUnknown) {
    status = AudioUnitSetProperty(
        m_audioUnit, kAudioOutputUnitProperty_CurrentDevice,
        kAudioUnitScope_Global, 0, &m_deviceID, sizeof(m_deviceID));
  }

  // Automatic hardware sample rate matching without resampling
  if (config.sampleRate > 0 && m_deviceID != kAudioObjectUnknown) {
    Float64 targetSampleRate = static_cast<Float64>(config.sampleRate);
    AudioObjectPropertyAddress rateAddr = {
        kAudioDevicePropertyNominalSampleRate, kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain};
    AudioObjectSetPropertyData(m_deviceID, &rateAddr, 0, nullptr,
                               sizeof(Float64), &targetSampleRate);
  }

  // Stream format setup
  AudioStreamBasicDescription asbd{};
  asbd.mSampleRate =
      static_cast<Float64>(config.sampleRate > 0 ? config.sampleRate : 44100);
  asbd.mChannelsPerFrame = ch;
  asbd.mBitsPerChannel = static_cast<UInt32>(bytesPerSample * 8);
  asbd.mBytesPerFrame = static_cast<UInt32>(m_frameBytes);
  asbd.mFramesPerPacket = 1;
  asbd.mBytesPerPacket = asbd.mBytesPerFrame;

  if (config.isFloat) {
    asbd.mFormatID = kAudioFormatLinearPCM;
    asbd.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
  } else {
    // Integer mode bit-perfect PCM
    asbd.mFormatID = kAudioFormatLinearPCM;
    asbd.mFormatFlags =
        kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
  }

  status = AudioUnitSetProperty(m_audioUnit, kAudioUnitProperty_StreamFormat,
                                kAudioUnitScope_Input, 0, &asbd, sizeof(asbd));
  if (status != noErr) {
    std::cerr << "CoreAudioSink: Failed to set stream format: " << status
              << std::endl;
    AudioComponentInstanceDispose(m_audioUnit);
    m_audioUnit = nullptr;
    return false;
  }

  status = AudioUnitInitialize(m_audioUnit);
  if (status != noErr) {
    std::cerr << "CoreAudioSink: Failed to initialize AudioUnit: " << status
              << std::endl;
    AudioComponentInstanceDispose(m_audioUnit);
    m_audioUnit = nullptr;
    return false;
  }

  status = AudioOutputUnitStart(m_audioUnit);
  if (status != noErr) {
    std::cerr << "CoreAudioSink: Failed to start AudioUnit: " << status
              << std::endl;
    AudioUnitUninitialize(m_audioUnit);
    AudioComponentInstanceDispose(m_audioUnit);
    m_audioUnit = nullptr;
    return false;
  }
#endif

  m_isOpen.store(true);
  m_isPaused.store(false);
  return true;
}

void CoreAudioSink::close() {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (!m_isOpen.load()) {
    return;
  }

#if defined(__APPLE__)
  if (m_hogModeActive.load()) {
    setHogMode(false);
  }

  if (m_audioUnit) {
    AudioOutputUnitStop(m_audioUnit);
    AudioUnitUninitialize(m_audioUnit);
    AudioComponentInstanceDispose(m_audioUnit);
    m_audioUnit = nullptr;
  }
  m_deviceID = kAudioObjectUnknown;
#endif

  m_isOpen.store(false);
  m_isPaused.store(false);
}

bool CoreAudioSink::isOpen() const {
  return m_isOpen.load(std::memory_order_relaxed);
}

size_t CoreAudioSink::write(const void *buffer, size_t frames) {
  if (!m_isOpen.load(std::memory_order_relaxed) ||
      m_isPaused.load(std::memory_order_relaxed) || !buffer || frames == 0) {
    return 0;
  }

#if defined(__APPLE__)
  // In CoreAudio, output is pull-driven via AURenderCallback or push to buffer
  return frames;
#else
  // Non-macOS stub simulation: successful write
  return frames;
#endif
}

void CoreAudioSink::pause() {
  m_isPaused.store(true);
#if defined(__APPLE__)
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (m_audioUnit) {
    AudioOutputUnitStop(m_audioUnit);
  }
#endif
}

void CoreAudioSink::resume() {
  m_isPaused.store(false);
#if defined(__APPLE__)
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (m_audioUnit) {
    AudioOutputUnitStart(m_audioUnit);
  }
#endif
}

void CoreAudioSink::flush() {
  // Clear buffers
}

uint32_t CoreAudioSink::getLatencyMs() const { return m_latencyMs; }

uint64_t CoreAudioSink::getXrunCount() const {
  return m_xrunCount.load(std::memory_order_relaxed);
}

bool CoreAudioSink::isBitPerfect() const { return true; }

std::string CoreAudioSink::getName() const {
  return "macOS CoreAudio Integer/Hog Mode";
}

bool CoreAudioSink::setHogMode(bool enable) {
#if defined(__APPLE__)
  if (m_deviceID == kAudioObjectUnknown)
    return false;

  AudioObjectPropertyAddress hogAddr = {kAudioDevicePropertyHogMode,
                                        kAudioObjectPropertyScopeGlobal,
                                        kAudioObjectPropertyElementMain};

  pid_t pid = enable ? getpid() : -1;
  OSStatus status = AudioObjectSetPropertyData(m_deviceID, &hogAddr, 0, nullptr,
                                               sizeof(pid_t), &pid);
  if (status == noErr) {
    m_hogModeActive.store(enable);
    m_hogPid = pid;
    return true;
  }
  return false;
#else
  m_hogModeActive.store(enable);
  return true;
#endif
}

bool CoreAudioSink::isHogModeActive() const {
  return m_hogModeActive.load(std::memory_order_relaxed);
}

std::vector<AudioDeviceInfo> CoreAudioSink::enumerateDevices() {
  std::vector<AudioDeviceInfo> devices;

#if defined(__APPLE__)
  AudioObjectPropertyAddress addr = {kAudioHardwarePropertyDevices,
                                     kAudioObjectPropertyScopeGlobal,
                                     kAudioObjectPropertyElementMain};

  UInt32 dataSize = 0;
  OSStatus status = AudioObjectGetPropertyDataSize(
      kAudioObjectSystemObject, &addr, 0, nullptr, &dataSize);
  if (status != noErr || dataSize == 0) {
    return devices;
  }

  UInt32 devCount = dataSize / sizeof(AudioDeviceID);
  std::vector<AudioDeviceID> devIds(devCount);
  status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &addr, 0,
                                      nullptr, &dataSize, devIds.data());
  if (status != noErr) {
    return devices;
  }

  AudioDeviceID defaultDev = kAudioObjectUnknown;
  UInt32 defSize = sizeof(AudioDeviceID);
  AudioObjectPropertyAddress defAddr = {
      kAudioHardwarePropertyDefaultOutputDevice,
      kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
  AudioObjectGetPropertyData(kAudioObjectSystemObject, &defAddr, 0, nullptr,
                             &defSize, &defaultDev);

  for (AudioDeviceID did : devIds) {
    CFStringRef nameRef = nullptr;
    UInt32 nameSize = sizeof(CFStringRef);
    AudioObjectPropertyAddress nameAddr = {kAudioObjectPropertyName,
                                           kAudioObjectPropertyScopeGlobal,
                                           kAudioObjectPropertyElementMain};
    status = AudioObjectGetPropertyData(did, &nameAddr, 0, nullptr, &nameSize,
                                        &nameRef);

    std::string nameStr = "CoreAudio Device";
    if (status == noErr && nameRef) {
      char buf[256];
      if (CFStringGetCString(nameRef, buf, sizeof(buf),
                             kCFStringEncodingUTF8)) {
        nameStr = buf;
      }
      CFRelease(nameRef);
    }

    devices.push_back({std::to_string(did), nameStr, "CoreAudio Output",
                       (did == defaultDev)});
  }
#else
  devices.push_back({"coreaudio:default", "Default CoreAudio Device",
                     "macOS CoreAudio HAL", true});
#endif

  return devices;
}

} // namespace adapters
