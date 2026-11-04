#include "CueSheetParser.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

namespace adapters {

static std::string trimString(const std::string &str) {
  size_t start = str.find_first_not_of(" \t\r\n\"");
  if (start == std::string::npos)
    return "";
  size_t end = str.find_last_not_of(" \t\r\n\"");
  return str.substr(start, end - start + 1);
}

static std::string simpleHash(const std::string &input) {
  uint64_t hash = 14695981039346656037ULL;
  for (unsigned char c : input) {
    hash ^= c;
    hash *= 1099511628211ULL;
  }
  std::stringstream ss;
  ss << std::hex << std::setw(16) << std::setfill('0') << hash;
  return ss.str();
}

uint64_t CueSheetParser::parseIndexTimestampMs(const std::string &timestamp) {
  std::string clean = trimString(timestamp);
  size_t colon1 = clean.find(':');
  if (colon1 == std::string::npos)
    return 0;
  size_t colon2 = clean.find(':', colon1 + 1);
  if (colon2 == std::string::npos)
    return 0;

  try {
    uint64_t minutes = std::stoull(clean.substr(0, colon1));
    uint64_t seconds =
        std::stoull(clean.substr(colon1 + 1, colon2 - colon1 - 1));
    uint64_t frames = std::stoull(clean.substr(colon2 + 1));

    // CUE standard: 75 frames per second
    return (minutes * 60 + seconds) * 1000 + (frames * 1000) / 75;
  } catch (...) {
    return 0;
  }
}

std::vector<core::Track>
CueSheetParser::parseFile(const std::string &cueFilePath,
                          uint64_t audioFileDurationMs) {
  fs::path p = fs::path(cueFilePath);
  if (!fs::exists(p)) {
    return {};
  }

  std::ifstream file(p);
  if (!file.is_open()) {
    return {};
  }

  std::stringstream ss;
  ss << file.rdbuf();
  std::string content = ss.str();

  fs::path dir = p.parent_path();
  std::string baseAudioPath;

  // Find FILE tag to identify companion audio file
  std::istringstream stream(content);
  std::string line;
  while (std::getline(stream, line)) {
    size_t filePos = line.find("FILE ");
    if (filePos != std::string::npos) {
      size_t firstQuote = line.find('"', filePos);
      size_t lastQuote = line.rfind('"');
      if (firstQuote != std::string::npos && lastQuote > firstQuote) {
        std::string filename =
            line.substr(firstQuote + 1, lastQuote - firstQuote - 1);
        fs::path audioPath = dir / fs::path(filename);
        if (fs::exists(audioPath)) {
          baseAudioPath = audioPath.string();
        } else {
          // Try file stem match with common audio extensions
          std::string stem = p.stem().string();
          for (const auto &ext :
               {".flac", ".wav", ".ape", ".wv", ".aiff", ".mp3", ".m4a"}) {
            fs::path candidate = dir / fs::path(stem + ext);
            if (fs::exists(candidate)) {
              baseAudioPath = candidate.string();
              break;
            }
          }
        }
        break;
      }
    }
  }

  if (baseAudioPath.empty()) {
    // Fallback: look for audio file with same stem
    std::string stem = p.stem().string();
    for (const auto &ext :
         {".flac", ".wav", ".ape", ".wv", ".aiff", ".mp3", ".m4a"}) {
      fs::path candidate = dir / fs::path(stem + ext);
      if (fs::exists(candidate)) {
        baseAudioPath = candidate.string();
        break;
      }
    }
  }

  return parseString(content, baseAudioPath, audioFileDurationMs);
}

std::vector<core::Track>
CueSheetParser::parseString(const std::string &cueContent,
                            const std::string &baseAudioPath,
                            uint64_t audioFileDurationMs) {
  std::vector<core::Track> tracks;
  std::istringstream stream(cueContent);
  std::string line;

  std::string albumArtist = "Various Artists";
  std::string albumTitle = "Album";
  std::string songwriter = "";
  std::string currentAudioFile = baseAudioPath;

  core::Track currentTrack;
  bool inTrack = false;

  while (std::getline(stream, line)) {
    std::string trimmed = trimString(line);
    if (trimmed.empty())
      continue;

    if (trimmed.rfind("PERFORMER ", 0) == 0 && !inTrack) {
      albumArtist = trimString(trimmed.substr(10));
    } else if (trimmed.rfind("TITLE ", 0) == 0 && !inTrack) {
      albumTitle = trimString(trimmed.substr(6));
    } else if (trimmed.rfind("SONGWRITER ", 0) == 0 && !inTrack) {
      songwriter = trimString(trimmed.substr(11));
    } else if (trimmed.rfind("FILE ", 0) == 0) {
      size_t firstQuote = trimmed.find('"');
      size_t lastQuote = trimmed.rfind('"');
      if (firstQuote != std::string::npos && lastQuote > firstQuote) {
        std::string fname =
            trimmed.substr(firstQuote + 1, lastQuote - firstQuote - 1);
        if (baseAudioPath.empty()) {
          currentAudioFile = fname;
        }
      }
    } else if (trimmed.rfind("TRACK ", 0) == 0) {
      if (inTrack) {
        tracks.push_back(currentTrack);
      }

      inTrack = true;
      currentTrack = core::Track{};
      currentTrack.album = albumTitle;
      currentTrack.albumArtist = albumArtist;
      currentTrack.artist = albumArtist;
      currentTrack.isVirtualTrack = true;

      std::string trackNumStr = trimString(trimmed.substr(6));
      size_t spacePos = trackNumStr.find(' ');
      if (spacePos != std::string::npos) {
        trackNumStr = trackNumStr.substr(0, spacePos);
      }
      try {
        currentTrack.trackNumber =
            static_cast<uint32_t>(std::stoul(trackNumStr));
      } catch (...) {
        currentTrack.trackNumber = static_cast<uint32_t>(tracks.size() + 1);
      }

      currentTrack.title = "Track " + std::to_string(currentTrack.trackNumber);
      currentTrack.filePath =
          currentAudioFile + "#cue:" + std::to_string(currentTrack.trackNumber);
      currentTrack.id = "cue_" + simpleHash(currentTrack.filePath);
      currentTrack.codec = "FLAC/CUE";
    } else if (inTrack && trimmed.rfind("TITLE ", 0) == 0) {
      currentTrack.title = trimString(trimmed.substr(6));
    } else if (inTrack && trimmed.rfind("PERFORMER ", 0) == 0) {
      currentTrack.artist = trimString(trimmed.substr(10));
    } else if (inTrack && trimmed.rfind("SONGWRITER ", 0) == 0) {
      // Songwriter parsed into metadata
      if (currentTrack.artist.empty() || currentTrack.artist == albumArtist) {
        currentTrack.artist = trimString(trimmed.substr(11));
      }
    } else if (inTrack && trimmed.rfind("INDEX 01 ", 0) == 0) {
      currentTrack.startTimeMs = parseIndexTimestampMs(trimmed.substr(9));
    }
  }

  if (inTrack) {
    tracks.push_back(currentTrack);
  }

  // Calculate accurate durations across virtual track boundaries
  for (size_t i = 0; i < tracks.size(); ++i) {
    if (i + 1 < tracks.size()) {
      tracks[i].endTimeMs = tracks[i + 1].startTimeMs;
      tracks[i].durationMs = (tracks[i].endTimeMs > tracks[i].startTimeMs)
                                 ? (tracks[i].endTimeMs - tracks[i].startTimeMs)
                                 : 0;
    } else {
      // Last track
      if (audioFileDurationMs > tracks[i].startTimeMs) {
        tracks[i].endTimeMs = audioFileDurationMs;
        tracks[i].durationMs = audioFileDurationMs - tracks[i].startTimeMs;
      } else {
        tracks[i].endTimeMs = 0;
        tracks[i].durationMs = 0;
      }
    }
  }

  return tracks;
}

} // namespace adapters
