#include "AudioDecoderFactory.hpp"
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

namespace adapters {

static std::string toLowerExt(const std::string &path) {
  fs::path p(path);
  std::string ext = p.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return ext;
}

std::unique_ptr<IAudioDecoder>
AudioDecoderFactory::createDecoder(const std::string &filePath) {
  std::string ext = toLowerExt(filePath);

  if (ext == ".wav" || ext == ".wave" || ext == ".aiff" || ext == ".aif" ||
      ext == ".aifc") {
    return std::make_unique<WavAudioDecoder>();
  }
  if (ext == ".flac") {
    return std::make_unique<FlacAudioDecoder>();
  }
  if (ext == ".dsf" || ext == ".dff") {
    return std::make_unique<DsdAudioDecoder>();
  }

  return nullptr;
}

bool AudioDecoderFactory::isSupportedInProcess(const std::string &filePath) {
  std::string ext = toLowerExt(filePath);
  return (ext == ".wav" || ext == ".wave" || ext == ".aiff" || ext == ".aif" ||
          ext == ".aifc" || ext == ".flac" || ext == ".dsf" || ext == ".dff");
}

} // namespace adapters
