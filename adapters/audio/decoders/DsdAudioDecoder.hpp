#pragma once

#include "IAudioDecoder.hpp"
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

namespace adapters {

/**
 * @brief In-process decoder for Sony DSF and Philips DSDIFF (DFF)
 * high-resolution 1-bit audio formats.
 *
 * Supports DSD64 (2.8224 MHz), DSD128 (5.6448 MHz), and DSD256 (11.2896 MHz),
 * offering both multi-tap FIR decimation to 32-bit float PCM and DoP (DSD over
 * PCM) marker packing.
 */
class DsdAudioDecoder : public IAudioDecoder {
public:
  enum class OutputMode {
    PcmDecimated, ///< Multi-tap FIR decimation to 88.2kHz / 176.4kHz float PCM
    DsdOverPcm    ///< DoP 16-bit DSD payload with 0x05/0xFA marker bytes
  };

  DsdAudioDecoder();
  ~DsdAudioDecoder() override;

  bool open(const std::string &filePath) override;
  void close() override;
  [[nodiscard]] bool isOpen() const override;

  size_t readFrames(float *buffer, size_t frames) override;
  bool seek(uint64_t frameIndex) override;
  [[nodiscard]] uint64_t getPositionFrames() const override;
  [[nodiscard]] AudioDecoderMetadata getMetadata() const override;

  void setOutputMode(OutputMode mode);
  [[nodiscard]] OutputMode getOutputMode() const;

private:
  bool parseDsf();
  bool parseDff();

  mutable std::mutex m_mutex;
  std::ifstream m_file;
  std::string m_filePath;
  bool m_isOpen{false};
  bool m_isDff{false};
  OutputMode m_outputMode{OutputMode::PcmDecimated};

  AudioDecoderMetadata m_meta;
  uint64_t m_dataOffset{0};
  uint64_t m_dataBytes{0};
  uint64_t m_currentFrame{0};
  uint32_t m_blockSize{4096};
  uint32_t m_dsfSampleRate{2822400};
  uint8_t m_dopMarker{0x05};
};

} // namespace adapters
