#pragma once

#include "DsdAudioDecoder.hpp"
#include "FlacAudioDecoder.hpp"
#include "IAudioDecoder.hpp"
#include "WavAudioDecoder.hpp"
#include <memory>
#include <string>

namespace adapters {

/**
 * @brief Factory creating in-process decoders based on file extensions and
 * format inspection.
 */
class AudioDecoderFactory {
public:
  static std::unique_ptr<IAudioDecoder>
  createDecoder(const std::string &filePath);
  static bool isSupportedInProcess(const std::string &filePath);
};

} // namespace adapters
