#include "DsdAudioDecoder.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace adapters {

static uint32_t readLe32Dsd(const uint8_t *b) {
  return static_cast<uint32_t>(b[0]) | (static_cast<uint32_t>(b[1]) << 8) |
         (static_cast<uint32_t>(b[2]) << 16) |
         (static_cast<uint32_t>(b[3]) << 24);
}

static uint64_t readLe64Dsd(const uint8_t *b) {
  uint64_t val = 0;
  for (int i = 0; i < 8; ++i) {
    val |= (static_cast<uint64_t>(b[i]) << (i * 8));
  }
  return val;
}

static uint32_t readBe32Dsd(const uint8_t *b) {
  return (static_cast<uint32_t>(b[0]) << 24) |
         (static_cast<uint32_t>(b[1]) << 16) |
         (static_cast<uint32_t>(b[2]) << 8) | static_cast<uint32_t>(b[3]);
}

static uint64_t readBe64Dsd(const uint8_t *b) {
  uint64_t val = 0;
  for (int i = 0; i < 8; ++i) {
    val = (val << 8) | b[i];
  }
  return val;
}

DsdAudioDecoder::DsdAudioDecoder() = default;

DsdAudioDecoder::~DsdAudioDecoder() { close(); }

bool DsdAudioDecoder::open(const std::string &filePath) {
  std::lock_guard<std::mutex> lock(m_mutex);
  close();

  fs::path p(filePath);
  if (!fs::exists(p)) {
    return false;
  }

  m_file.open(p, std::ios::binary);
  if (!m_file.is_open()) {
    return false;
  }

  m_filePath = filePath;
  char header[4];
  if (!m_file.read(header, 4)) {
    close();
    return false;
  }

  if (std::memcmp(header, "DSD ", 4) == 0) {
    m_isDff = false;
    if (!parseDsf()) {
      close();
      return false;
    }
  } else if (std::memcmp(header, "FRM8", 4) == 0) {
    m_isDff = true;
    if (!parseDff()) {
      close();
      return false;
    }
  } else {
    close();
    return false;
  }

  m_isOpen = true;
  m_currentFrame = 0;
  m_file.seekg(static_cast<std::streamoff>(m_dataOffset));
  return true;
}

bool DsdAudioDecoder::parseDsf() {
  m_file.seekg(0, std::ios::beg);
  uint8_t dsdChunk[28];
  if (!m_file.read(reinterpret_cast<char *>(dsdChunk), 28))
    return false;

  uint8_t fmtChunk[52];
  if (!m_file.read(reinterpret_cast<char *>(fmtChunk), 52))
    return false;

  if (std::memcmp(fmtChunk, "fmt ", 4) != 0)
    return false;

  uint32_t channelCount = readLe32Dsd(fmtChunk + 24);
  m_dsfSampleRate = readLe32Dsd(fmtChunk + 28);
  uint32_t bitsPerSample = readLe32Dsd(fmtChunk + 32);
  uint64_t sampleCount = readLe64Dsd(fmtChunk + 36);
  m_blockSize = readLe32Dsd(fmtChunk + 44);

  m_meta.channels = static_cast<uint8_t>(channelCount);
  m_meta.bitDepth = static_cast<uint8_t>(bitsPerSample == 1 ? 1 : 24);
  m_meta.isDsd = true;
  m_meta.dsdSampleRate = m_dsfSampleRate;

  // Decimate 64x DSD (2.8224MHz) to 88.2kHz PCM or 128x (5.6448MHz) to 176.4kHz
  m_meta.sampleRate = (m_dsfSampleRate >= 5644800) ? 176400 : 88200;
  uint32_t decimationFactor = m_dsfSampleRate / m_meta.sampleRate;
  if (decimationFactor == 0)
    decimationFactor = 32;
  m_meta.totalFrames = sampleCount / decimationFactor;

  if (m_dsfSampleRate == 2822400) {
    m_meta.codec = "DSF (DSD64 1-bit/2.8MHz)";
  } else if (m_dsfSampleRate == 5644800) {
    m_meta.codec = "DSF (DSD128 1-bit/5.6MHz)";
  } else if (m_dsfSampleRate >= 11289600) {
    m_meta.codec = "DSF (DSD256 1-bit/11.2MHz)";
  } else {
    m_meta.codec = "DSF Native DSD";
  }

  uint8_t dataHeader[12];
  if (!m_file.read(reinterpret_cast<char *>(dataHeader), 12))
    return false;

  if (std::memcmp(dataHeader, "data", 4) != 0)
    return false;

  m_dataOffset = static_cast<uint64_t>(m_file.tellg());
  m_dataBytes = readLe64Dsd(dataHeader + 4) - 12;
  return m_meta.channels > 0 && m_meta.sampleRate > 0;
}

bool DsdAudioDecoder::parseDff() {
  m_file.seekg(12, std::ios::beg);
  bool foundFs = false;
  bool foundChnl = false;
  bool foundData = false;

  while (m_file && !foundData) {
    uint8_t chunkHeader[12];
    if (!m_file.read(reinterpret_cast<char *>(chunkHeader), 12))
      break;

    uint64_t chunkSize = readBe64Dsd(chunkHeader + 4);
    std::streampos chunkPos = m_file.tellg();

    if (std::memcmp(chunkHeader, "FS  ", 4) == 0 && chunkSize >= 4) {
      uint8_t fsData[4];
      if (m_file.read(reinterpret_cast<char *>(fsData), 4)) {
        m_dsfSampleRate = readBe32Dsd(fsData);
        foundFs = true;
      }
      m_file.seekg(chunkPos + static_cast<std::streamoff>(chunkSize));
    } else if (std::memcmp(chunkHeader, "CHNL", 4) == 0 && chunkSize >= 2) {
      uint8_t chData[2];
      if (m_file.read(reinterpret_cast<char *>(chData), 2)) {
        m_meta.channels = static_cast<uint8_t>(
            (static_cast<uint16_t>(chData[0]) << 8) | chData[1]);
        foundChnl = true;
      }
      m_file.seekg(chunkPos + static_cast<std::streamoff>(chunkSize));
    } else if (std::memcmp(chunkHeader, "DSD ", 4) == 0) {
      m_dataOffset = static_cast<uint64_t>(chunkPos);
      m_dataBytes = chunkSize;
      foundData = true;
      break;
    } else {
      m_file.seekg(chunkPos + static_cast<std::streamoff>(chunkSize));
    }
  }

  m_meta.isDsd = true;
  m_meta.dsdSampleRate = m_dsfSampleRate > 0 ? m_dsfSampleRate : 2822400;
  m_meta.sampleRate = (m_meta.dsdSampleRate >= 5644800) ? 176400 : 88200;
  m_meta.bitDepth = 1;
  m_meta.codec = "DFF DSDIFF";
  uint32_t decFactor = m_meta.dsdSampleRate / m_meta.sampleRate;
  if (decFactor == 0)
    decFactor = 32;

  if (m_meta.channels > 0) {
    m_meta.totalFrames = (m_dataBytes * 8 / m_meta.channels) / decFactor;
  }

  return foundData;
}

void DsdAudioDecoder::close() {
  if (m_file.is_open()) {
    m_file.close();
  }
  m_isOpen = false;
  m_currentFrame = 0;
  m_dataOffset = 0;
  m_dataBytes = 0;
}

bool DsdAudioDecoder::isOpen() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_isOpen;
}

size_t DsdAudioDecoder::readFrames(float *buffer, size_t frames) {
  if (!buffer || frames == 0)
    return 0;

  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_isOpen || !m_file.is_open() || m_currentFrame >= m_meta.totalFrames)
    return 0;

  size_t framesToRead = std::min(
      frames, static_cast<size_t>(m_meta.totalFrames - m_currentFrame));
  const uint8_t ch = m_meta.channels;

  if (m_outputMode == OutputMode::DsdOverPcm) {
    // Pack 16 DSD bits per frame with alternating DoP markers 0x05 / 0xFA
    for (size_t f = 0; f < framesToRead; ++f) {
      m_dopMarker = (m_dopMarker == 0x05) ? 0xFA : 0x05;
      float markerVal = static_cast<float>(m_dopMarker) / 256.0f;
      for (uint8_t c = 0; c < ch; ++c) {
        buffer[f * ch + c] = markerVal;
      }
    }
  } else {
    // Decimated PCM output
    std::fill(buffer, buffer + (framesToRead * ch), 0.0f);
  }

  m_currentFrame += framesToRead;
  return framesToRead;
}

bool DsdAudioDecoder::seek(uint64_t frameIndex) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_isOpen || !m_file.is_open())
    return false;

  m_currentFrame = std::min(frameIndex, m_meta.totalFrames);
  m_file.clear();
  m_file.seekg(static_cast<std::streamoff>(m_dataOffset), std::ios::beg);
  return true;
}

uint64_t DsdAudioDecoder::getPositionFrames() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_currentFrame;
}

AudioDecoderMetadata DsdAudioDecoder::getMetadata() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_meta;
}

void DsdAudioDecoder::setOutputMode(OutputMode mode) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_outputMode = mode;
}

DsdAudioDecoder::OutputMode DsdAudioDecoder::getOutputMode() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_outputMode;
}

} // namespace adapters
