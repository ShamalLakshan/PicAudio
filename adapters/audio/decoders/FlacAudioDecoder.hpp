#pragma once

#include "IAudioDecoder.hpp"
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

namespace adapters {

/**
 * @brief In-process bit-perfect FLAC audio decoder supporting up to 32-bit /
 * 384kHz.
 *
 * Implements native STREAMINFO metadata extraction, seektable parsing,
 * and high-resolution frame streaming.
 */
class FlacAudioDecoder : public IAudioDecoder {
public:
  FlacAudioDecoder();
  ~FlacAudioDecoder() override;

  bool open(const std::string &filePath) override;
  void close() override;
  [[nodiscard]] bool isOpen() const override;

  size_t readFrames(float *buffer, size_t frames) override;
  bool seek(uint64_t frameIndex) override;
  [[nodiscard]] uint64_t getPositionFrames() const override;
  [[nodiscard]] AudioDecoderMetadata getMetadata() const override;

private:
  bool parseStreamInfo();

  mutable std::mutex m_mutex;
  std::ifstream m_file;
  std::string m_filePath;
  bool m_isOpen{false};
  AudioDecoderMetadata m_meta;

  uint64_t m_firstFrameOffset{0};
  uint64_t m_currentFrame{0};
  uint16_t m_minBlockSize{0};
  uint16_t m_maxBlockSize{0};
};

} // namespace adapters
