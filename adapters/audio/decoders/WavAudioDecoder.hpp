#pragma once

#include "IAudioDecoder.hpp"
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

namespace adapters {

/**
 * @brief In-process bit-perfect decoder for WAV, WAVE, and AIFF audio formats.
 *
 * Supports PCM 8/16/24/32-bit integers, 32/64-bit IEEE float,
 * WAVE_FORMAT_EXTENSIBLE, and AIFF big-endian audio streams with
 * sample-accurate seeking.
 */
class WavAudioDecoder : public IAudioDecoder {
public:
  WavAudioDecoder();
  ~WavAudioDecoder() override;

  bool open(const std::string &filePath) override;
  void close() override;
  [[nodiscard]] bool isOpen() const override;

  size_t readFrames(float *buffer, size_t frames) override;
  bool seek(uint64_t frameIndex) override;
  [[nodiscard]] uint64_t getPositionFrames() const override;
  [[nodiscard]] AudioDecoderMetadata getMetadata() const override;

private:
  bool parseWav();
  bool parseAiff();

  mutable std::mutex m_mutex;
  std::ifstream m_file;
  std::string m_filePath;
  bool m_isOpen{false};
  bool m_isAiff{false};
  bool m_isFloat{false};

  AudioDecoderMetadata m_meta;
  uint64_t m_dataOffset{0};
  uint64_t m_dataBytes{0};
  uint64_t m_currentFrame{0};
  uint32_t m_bytesPerFrame{4};
  uint32_t m_bytesPerSample{2};
};

} // namespace adapters
