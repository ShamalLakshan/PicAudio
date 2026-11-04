#include "WavAudioDecoder.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace adapters {

static uint16_t readLe16(const uint8_t *b) {
  return static_cast<uint16_t>(b[0]) | (static_cast<uint16_t>(b[1]) << 8);
}

static uint32_t readLe32(const uint8_t *b) {
  return static_cast<uint32_t>(b[0]) | (static_cast<uint32_t>(b[1]) << 8) |
         (static_cast<uint32_t>(b[2]) << 16) |
         (static_cast<uint32_t>(b[3]) << 24);
}

static uint16_t readBe16(const uint8_t *b) {
  return (static_cast<uint16_t>(b[0]) << 8) | static_cast<uint16_t>(b[1]);
}

static uint32_t readBe32(const uint8_t *b) {
  return (static_cast<uint32_t>(b[0]) << 24) |
         (static_cast<uint32_t>(b[1]) << 16) |
         (static_cast<uint32_t>(b[2]) << 8) | static_cast<uint32_t>(b[3]);
}

static double readExtended80(const uint8_t *b) {
  int exponent = ((b[0] & 0x7F) << 8) | b[1];
  uint64_t mantissa = 0;
  for (int i = 2; i < 10; ++i) {
    mantissa = (mantissa << 8) | b[i];
  }
  if (exponent == 0 && mantissa == 0)
    return 0.0;
  double val = std::ldexp(static_cast<double>(mantissa), exponent - 16383 - 63);
  return (b[0] & 0x80) ? -val : val;
}

WavAudioDecoder::WavAudioDecoder() = default;

WavAudioDecoder::~WavAudioDecoder() { close(); }

bool WavAudioDecoder::open(const std::string &filePath) {
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
  uint8_t header[12];
  if (!m_file.read(reinterpret_cast<char *>(header), 12)) {
    close();
    return false;
  }

  if (std::memcmp(header, "RIFF", 4) == 0 &&
      std::memcmp(header + 8, "WAVE", 4) == 0) {
    m_isAiff = false;
    if (!parseWav()) {
      close();
      return false;
    }
  } else if (std::memcmp(header, "FORM", 4) == 0 &&
             (std::memcmp(header + 8, "AIFF", 4) == 0 ||
              std::memcmp(header + 8, "AIFC", 4) == 0)) {
    m_isAiff = true;
    if (!parseAiff()) {
      close();
      return false;
    }
  } else {
    close();
    return false;
  }

  m_bytesPerFrame = m_meta.channels * m_bytesPerSample;
  if (m_bytesPerFrame == 0) {
    close();
    return false;
  }

  m_meta.totalFrames = m_dataBytes / m_bytesPerFrame;
  if (m_meta.sampleRate > 0) {
    m_meta.bitrate =
        static_cast<uint32_t>((m_bytesPerFrame * m_meta.sampleRate * 8) / 1000);
  }

  m_isOpen = true;
  m_currentFrame = 0;
  m_file.seekg(static_cast<std::streamoff>(m_dataOffset));
  return true;
}

bool WavAudioDecoder::parseWav() {
  bool foundFmt = false;
  bool foundData = false;

  m_file.seekg(12, std::ios::beg);

  while (m_file && (!foundFmt || !foundData)) {
    uint8_t chunkHeader[8];
    if (!m_file.read(reinterpret_cast<char *>(chunkHeader), 8))
      break;

    uint32_t chunkSize = readLe32(chunkHeader + 4);
    std::streampos chunkPos = m_file.tellg();

    if (std::memcmp(chunkHeader, "fmt ", 4) == 0 && chunkSize >= 16) {
      std::vector<uint8_t> fmtData(chunkSize);
      if (!m_file.read(reinterpret_cast<char *>(fmtData.data()), chunkSize))
        return false;

      uint16_t formatTag = readLe16(fmtData.data());
      m_meta.channels = static_cast<uint8_t>(readLe16(fmtData.data() + 2));
      m_meta.sampleRate = readLe32(fmtData.data() + 4);
      m_meta.bitDepth = static_cast<uint8_t>(readLe16(fmtData.data() + 14));
      m_bytesPerSample = (m_meta.bitDepth + 7) / 8;

      if (formatTag == 3) { // IEEE Float
        m_isFloat = true;
        m_meta.codec = "WAV Float";
      } else if (formatTag == 0xFFFE &&
                 chunkSize >= 40) { // WAVE_FORMAT_EXTENSIBLE
        uint16_t subFormat = readLe16(fmtData.data() + 24);
        m_isFloat = (subFormat == 3);
        m_meta.codec =
            m_isFloat ? "WAV Extensible Float" : "WAV Extensible PCM";
      } else {
        m_isFloat = false;
        m_meta.codec = "WAV PCM";
      }
      foundFmt = true;
    } else if (std::memcmp(chunkHeader, "data", 4) == 0) {
      m_dataOffset = static_cast<uint64_t>(chunkPos);
      m_dataBytes = chunkSize;
      foundData = true;
      break;
    } else {
      m_file.seekg(chunkPos +
                   static_cast<std::streamoff>(chunkSize + (chunkSize & 1)));
    }
  }

  return foundFmt && foundData;
}

bool WavAudioDecoder::parseAiff() {
  bool foundComm = false;
  bool foundSsnd = false;

  m_file.seekg(12, std::ios::beg);

  while (m_file && (!foundComm || !foundSsnd)) {
    uint8_t chunkHeader[8];
    if (!m_file.read(reinterpret_cast<char *>(chunkHeader), 8))
      break;

    uint32_t chunkSize = readBe32(chunkHeader + 4);
    std::streampos chunkPos = m_file.tellg();

    if (std::memcmp(chunkHeader, "COMM", 4) == 0 && chunkSize >= 18) {
      std::vector<uint8_t> commData(chunkSize);
      if (!m_file.read(reinterpret_cast<char *>(commData.data()), chunkSize))
        return false;

      m_meta.channels = static_cast<uint8_t>(readBe16(commData.data()));
      uint32_t numSampleFrames = readBe32(commData.data() + 2);
      m_meta.bitDepth = static_cast<uint8_t>(readBe16(commData.data() + 6));
      m_bytesPerSample = (m_meta.bitDepth + 7) / 8;
      double srate = readExtended80(commData.data() + 8);
      m_meta.sampleRate = static_cast<uint32_t>(std::round(srate));
      m_meta.codec = "AIFF PCM";
      m_meta.totalFrames = numSampleFrames;
      foundComm = true;
    } else if (std::memcmp(chunkHeader, "SSND", 4) == 0 && chunkSize >= 8) {
      uint8_t ssndHeader[8];
      if (!m_file.read(reinterpret_cast<char *>(ssndHeader), 8))
        return false;
      uint32_t offset = readBe32(ssndHeader);
      m_dataOffset = static_cast<uint64_t>(chunkPos) + 8 + offset;
      m_dataBytes = chunkSize - 8 - offset;
      foundSsnd = true;
      break;
    } else {
      m_file.seekg(chunkPos +
                   static_cast<std::streamoff>(chunkSize + (chunkSize & 1)));
    }
  }

  return foundComm && foundSsnd;
}

void WavAudioDecoder::close() {
  if (m_file.is_open()) {
    m_file.close();
  }
  m_isOpen = false;
  m_currentFrame = 0;
  m_dataOffset = 0;
  m_dataBytes = 0;
}

bool WavAudioDecoder::isOpen() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_isOpen;
}

size_t WavAudioDecoder::readFrames(float *buffer, size_t frames) {
  if (!buffer || frames == 0)
    return 0;

  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_isOpen || !m_file.is_open() || m_currentFrame >= m_meta.totalFrames)
    return 0;

  size_t framesToRead = std::min(
      frames, static_cast<size_t>(m_meta.totalFrames - m_currentFrame));
  size_t bytesToRead = framesToRead * m_bytesPerFrame;
  std::vector<uint8_t> rawBuffer(bytesToRead);

  m_file.read(reinterpret_cast<char *>(rawBuffer.data()), bytesToRead);
  size_t bytesRead = static_cast<size_t>(m_file.gcount());
  size_t framesRead = bytesRead / m_bytesPerFrame;
  if (framesRead == 0)
    return 0;

  const uint8_t *src = rawBuffer.data();
  const uint8_t ch = m_meta.channels;
  size_t outIdx = 0;

  if (m_isFloat && m_bytesPerSample == 4) {
    const float *fsrc = reinterpret_cast<const float *>(src);
    for (size_t i = 0; i < framesRead * ch; ++i) {
      buffer[outIdx++] = fsrc[i];
    }
  } else if (!m_isAiff) {
    // Little-endian PCM
    if (m_bytesPerSample == 2) {
      for (size_t f = 0; f < framesRead; ++f) {
        for (uint8_t c = 0; c < ch; ++c) {
          int16_t s = static_cast<int16_t>(readLe16(src));
          buffer[outIdx++] = static_cast<float>(s) / 32768.0f;
          src += 2;
        }
      }
    } else if (m_bytesPerSample == 3) {
      for (size_t f = 0; f < framesRead; ++f) {
        for (uint8_t c = 0; c < ch; ++c) {
          int32_t s = (static_cast<int32_t>(src[2]) << 24) |
                      (static_cast<int32_t>(src[1]) << 16) |
                      (static_cast<int32_t>(src[0]) << 8);
          buffer[outIdx++] = static_cast<float>(s) / 2147483648.0f;
          src += 3;
        }
      }
    } else if (m_bytesPerSample == 4) {
      for (size_t f = 0; f < framesRead; ++f) {
        for (uint8_t c = 0; c < ch; ++c) {
          int32_t s = static_cast<int32_t>(readLe32(src));
          buffer[outIdx++] = static_cast<float>(s) / 2147483648.0f;
          src += 4;
        }
      }
    } else if (m_bytesPerSample == 1) {
      for (size_t f = 0; f < framesRead; ++f) {
        for (uint8_t c = 0; c < ch; ++c) {
          buffer[outIdx++] = (static_cast<float>(*src++) - 128.0f) / 128.0f;
        }
      }
    }
  } else {
    // Big-endian AIFF
    if (m_bytesPerSample == 2) {
      for (size_t f = 0; f < framesRead; ++f) {
        for (uint8_t c = 0; c < ch; ++c) {
          int16_t s = static_cast<int16_t>(readBe16(src));
          buffer[outIdx++] = static_cast<float>(s) / 32768.0f;
          src += 2;
        }
      }
    } else if (m_bytesPerSample == 3) {
      for (size_t f = 0; f < framesRead; ++f) {
        for (uint8_t c = 0; c < ch; ++c) {
          int32_t s = (static_cast<int32_t>(src[0]) << 24) |
                      (static_cast<int32_t>(src[1]) << 16) |
                      (static_cast<int32_t>(src[2]) << 8);
          buffer[outIdx++] = static_cast<float>(s) / 2147483648.0f;
          src += 3;
        }
      }
    } else if (m_bytesPerSample == 4) {
      for (size_t f = 0; f < framesRead; ++f) {
        for (uint8_t c = 0; c < ch; ++c) {
          int32_t s = static_cast<int32_t>(readBe32(src));
          buffer[outIdx++] = static_cast<float>(s) / 2147483648.0f;
          src += 4;
        }
      }
    }
  }

  m_currentFrame += framesRead;
  return framesRead;
}

bool WavAudioDecoder::seek(uint64_t frameIndex) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_isOpen || !m_file.is_open())
    return false;

  uint64_t clampedFrame = std::min(frameIndex, m_meta.totalFrames);
  std::streamoff targetOffset = static_cast<std::streamoff>(
      m_dataOffset + clampedFrame * m_bytesPerFrame);

  m_file.clear();
  m_file.seekg(targetOffset, std::ios::beg);
  m_currentFrame = clampedFrame;
  return true;
}

uint64_t WavAudioDecoder::getPositionFrames() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_currentFrame;
}

AudioDecoderMetadata WavAudioDecoder::getMetadata() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_meta;
}

} // namespace adapters
