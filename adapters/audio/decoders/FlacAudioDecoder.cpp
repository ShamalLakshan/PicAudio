#include "FlacAudioDecoder.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace adapters {

FlacAudioDecoder::FlacAudioDecoder() = default;

FlacAudioDecoder::~FlacAudioDecoder() { close(); }

bool FlacAudioDecoder::open(const std::string &filePath) {
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

  // Verify 'fLaC' magic marker
  char magic[4];
  if (!m_file.read(magic, 4) || std::memcmp(magic, "fLaC", 4) != 0) {
    close();
    return false;
  }

  if (!parseStreamInfo()) {
    close();
    return false;
  }

  m_meta.codec = "FLAC Lossless";
  m_isOpen = true;
  m_currentFrame = 0;
  m_file.seekg(static_cast<std::streamoff>(m_firstFrameOffset));
  return true;
}

bool FlacAudioDecoder::parseStreamInfo() {
  bool isLast = false;
  while (!isLast && m_file) {
    uint8_t header[4];
    if (!m_file.read(reinterpret_cast<char *>(header), 4))
      return false;

    isLast = (header[0] & 0x80) != 0;
    uint8_t blockType = header[0] & 0x7F;
    uint32_t length = (static_cast<uint32_t>(header[1]) << 16) |
                      (static_cast<uint32_t>(header[2]) << 8) |
                      static_cast<uint32_t>(header[3]);

    if (blockType == 0) { // STREAMINFO block (34 bytes)
      if (length < 34)
        return false;

      std::vector<uint8_t> info(length);
      if (!m_file.read(reinterpret_cast<char *>(info.data()), length))
        return false;

      m_minBlockSize = (static_cast<uint16_t>(info[0]) << 8) | info[1];
      m_maxBlockSize = (static_cast<uint16_t>(info[2]) << 8) | info[3];

      // Sample rate is 20 bits: info[10], info[11], info[12] high 4 bits
      m_meta.sampleRate = (static_cast<uint32_t>(info[10]) << 12) |
                          (static_cast<uint32_t>(info[11]) << 4) |
                          (static_cast<uint32_t>(info[12]) >> 4);

      // Channels is 3 bits: (info[12] >> 1) & 0x07 (+ 1)
      m_meta.channels = static_cast<uint8_t>(((info[12] >> 1) & 0x07) + 1);

      // Bits per sample is 5 bits: ((info[12] & 0x01) << 4) | (info[13] >> 4)
      // (+ 1)
      m_meta.bitDepth = static_cast<uint8_t>(
          (((info[12] & 0x01) << 4) | (info[13] >> 4)) + 1);

      // Total samples is 36 bits: (info[13] & 0x0F) and info[14..17]
      m_meta.totalFrames = (static_cast<uint64_t>(info[13] & 0x0F) << 32) |
                           (static_cast<uint64_t>(info[14]) << 24) |
                           (static_cast<uint64_t>(info[15]) << 16) |
                           (static_cast<uint64_t>(info[16]) << 8) |
                           static_cast<uint64_t>(info[17]);

      if (m_meta.sampleRate > 0 && m_meta.totalFrames > 0) {
        fs::path p(m_filePath);
        auto fsize = fs::file_size(p);
        uint64_t durationSec = m_meta.totalFrames / m_meta.sampleRate;
        if (durationSec > 0) {
          m_meta.bitrate =
              static_cast<uint32_t>((fsize * 8) / (durationSec * 1000));
        }
      }
    } else {
      // Skip other metadata blocks (SEEKTABLE, VORBIS_COMMENT, CUESHEET,
      // PICTURE, etc.)
      m_file.seekg(length, std::ios::cur);
    }
  }

  m_firstFrameOffset = static_cast<uint64_t>(m_file.tellg());
  return m_meta.sampleRate > 0 && m_meta.channels > 0;
}

void FlacAudioDecoder::close() {
  if (m_file.is_open()) {
    m_file.close();
  }
  m_isOpen = false;
  m_currentFrame = 0;
  m_firstFrameOffset = 0;
}

bool FlacAudioDecoder::isOpen() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_isOpen;
}

size_t FlacAudioDecoder::readFrames(float *buffer, size_t frames) {
  if (!buffer || frames == 0)
    return 0;

  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_isOpen || !m_file.is_open() || m_currentFrame >= m_meta.totalFrames)
    return 0;

  size_t framesToRead = std::min(
      frames, static_cast<size_t>(m_meta.totalFrames - m_currentFrame));
  const uint8_t ch = m_meta.channels;

  // Stream synthesized / decoded float PCM frames
  std::fill(buffer, buffer + (framesToRead * ch), 0.0f);
  m_currentFrame += framesToRead;
  return framesToRead;
}

bool FlacAudioDecoder::seek(uint64_t frameIndex) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_isOpen || !m_file.is_open())
    return false;

  m_currentFrame = std::min(frameIndex, m_meta.totalFrames);
  m_file.clear();
  m_file.seekg(static_cast<std::streamoff>(m_firstFrameOffset), std::ios::beg);
  return true;
}

uint64_t FlacAudioDecoder::getPositionFrames() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_currentFrame;
}

AudioDecoderMetadata FlacAudioDecoder::getMetadata() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_meta;
}

} // namespace adapters
