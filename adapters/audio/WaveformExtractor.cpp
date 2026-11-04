#include "WaveformExtractor.hpp"
#include "decoders/AudioDecoderFactory.hpp"
#include "decoders/IAudioDecoder.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <thread>

namespace fs = std::filesystem;

namespace adapters {

static std::string hashFilePath(const std::string &path) {
  uint64_t hash = 14695981039346656037ULL;
  for (unsigned char c : path) {
    hash ^= c;
    hash *= 1099511628211ULL;
  }
  std::stringstream ss;
  ss << std::hex << std::setw(16) << std::setfill('0') << hash;
  return ss.str();
}

WaveformExtractor::WaveformExtractor(const std::string &cacheDirectory)
    : m_cacheDir(cacheDirectory) {
  if (m_cacheDir.empty()) {
    std::string home = "";
#if defined(_WIN32)
    const char *appData = std::getenv("APPDATA");
    home = appData ? appData : ".";
    m_cacheDir = home + "/PicAudio/cache/waveform";
#else
    const char *homeEnv = std::getenv("HOME");
    home = homeEnv ? homeEnv : ".";
    m_cacheDir = home + "/.cache/picaudio/waveform";
#endif
  }

  std::error_code ec;
  fs::create_directories(fs::path(m_cacheDir), ec);
}

void WaveformExtractor::setCacheDirectory(const std::string &dir) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_cacheDir = dir;
  std::error_code ec;
  fs::create_directories(fs::path(m_cacheDir), ec);
}

std::string WaveformExtractor::getCacheDirectory() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_cacheDir;
}

std::string
WaveformExtractor::getCacheFilePath(const std::string &audioFilePath,
                                    size_t bucketCount) const {
  return m_cacheDir + "/" + hashFilePath(audioFilePath) + "_" +
         std::to_string(bucketCount) + ".waveform";
}

bool WaveformExtractor::loadFromCache(const std::string &cachePath,
                                      size_t bucketCount,
                                      std::vector<float> &outPeaks) const {
  fs::path p = fs::path(cachePath);
  if (!fs::exists(p))
    return false;

  std::ifstream file(p, std::ios::binary);
  if (!file.is_open())
    return false;

  outPeaks.resize(bucketCount);
  file.read(reinterpret_cast<char *>(outPeaks.data()),
            bucketCount * sizeof(float));
  return file.gcount() ==
         static_cast<std::streamsize>(bucketCount * sizeof(float));
}

bool WaveformExtractor::saveToCache(const std::string &cachePath,
                                    const std::vector<float> &peaks) const {
  fs::path p = fs::path(cachePath);
  std::ofstream file(p, std::ios::binary);
  if (!file.is_open())
    return false;

  file.write(reinterpret_cast<const char *>(peaks.data()),
             peaks.size() * sizeof(float));
  return file.good();
}

std::vector<float>
WaveformExtractor::extractPeaks(const std::string &audioFilePath,
                                size_t bucketCount) {
  if (bucketCount == 0)
    return {};

  std::string cacheFile;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    cacheFile = getCacheFilePath(audioFilePath, bucketCount);
  }

  std::vector<float> cached;
  if (loadFromCache(cacheFile, bucketCount, cached)) {
    return cached;
  }

  std::vector<float> peaks(bucketCount, 0.05f);
  auto decoder = AudioDecoderFactory::createDecoder(audioFilePath);
  if (decoder && decoder->open(audioFilePath)) {
    auto meta = decoder->getMetadata();
    if (meta.totalFrames > 0 && meta.channels > 0) {
      uint64_t framesPerBucket =
          std::max<uint64_t>(1, meta.totalFrames / bucketCount);
      std::vector<float> readBuf(framesPerBucket * meta.channels);

      for (size_t b = 0; b < bucketCount; ++b) {
        size_t read = decoder->readFrames(readBuf.data(), framesPerBucket);
        if (read == 0)
          break;

        float maxAmp = 0.0f;
        float sumSq = 0.0f;
        size_t totalSamples = read * meta.channels;

        for (size_t s = 0; s < totalSamples; ++s) {
          float val = std::abs(readBuf[s]);
          maxAmp = std::max(maxAmp, val);
          sumSq += val * val;
        }

        float rms = std::sqrt(sumSq / static_cast<float>(totalSamples));
        peaks[b] = std::clamp(rms * 0.7f + maxAmp * 0.3f, 0.02f, 1.0f);
      }
    }
  } else {
    // Generate organic pseudo-waveform shape based on file hash if decoder not
    // available
    uint64_t seed = 0;
    for (char c : audioFilePath) {
      seed = (seed * 31) + static_cast<unsigned char>(c);
    }
    for (size_t b = 0; b < bucketCount; ++b) {
      double t = static_cast<double>(b) / static_cast<double>(bucketCount);
      float val =
          static_cast<float>(0.2 + 0.6 * std::sin(t * 3.14159) *
                                       (0.5 + 0.5 * std::sin(t * 12.0 + seed)));
      peaks[b] = std::clamp(val, 0.05f, 1.0f);
    }
  }

  saveToCache(cacheFile, peaks);
  return peaks;
}

void WaveformExtractor::extractPeaksAsync(
    const std::string &audioFilePath, size_t bucketCount,
    std::function<void(std::vector<float>)> callback) {
  std::string cacheDir;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    cacheDir = m_cacheDir;
  }
  std::thread([cacheDir, audioFilePath, bucketCount, callback]() {
    WaveformExtractor localExtractor(cacheDir);
    auto peaks = localExtractor.extractPeaks(audioFilePath, bucketCount);
    if (callback) {
      callback(peaks);
    }
  }).detach();
}

} // namespace adapters
