#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace adapters {

/**
 * @brief Stream properties reported by audio decoders.
 */
struct AudioDecoderMetadata {
  uint32_t sampleRate{44100};
  uint8_t channels{2};
  uint8_t bitDepth{16};
  uint64_t totalFrames{0};
  uint32_t bitrate{0};
  std::string codec{"PCM"};
  bool isDsd{false};
  uint32_t dsdSampleRate{0};
};

/**
 * @brief Interface for in-process bit-perfect audio decoders.
 */
class IAudioDecoder {
public:
  virtual ~IAudioDecoder() = default;

  /**
   * @brief Opens and parses the target audio file headers.
   */
  virtual bool open(const std::string &filePath) = 0;

  /**
   * @brief Closes the audio file and releases resources.
   */
  virtual void close() = 0;

  /**
   * @brief Queries whether the decoder is open and ready to read frames.
   */
  [[nodiscard]] virtual bool isOpen() const = 0;

  /**
   * @brief Reads interleaved normalized 32-bit float PCM audio frames [-1.0f,
   * +1.0f].
   * @param buffer Output buffer of at least frames * channels floats.
   * @param frames Number of frames to read.
   * @return Number of frames actually read.
   */
  virtual size_t readFrames(float *buffer, size_t frames) = 0;

  /**
   * @brief Seeks to a specific frame position.
   */
  virtual bool seek(uint64_t frameIndex) = 0;

  /**
   * @brief Current read position in frames.
   */
  [[nodiscard]] virtual uint64_t getPositionFrames() const = 0;

  /**
   * @brief Stream metadata including sample rate, channels, bit depth, and
   * duration.
   */
  [[nodiscard]] virtual AudioDecoderMetadata getMetadata() const = 0;
};

} // namespace adapters
