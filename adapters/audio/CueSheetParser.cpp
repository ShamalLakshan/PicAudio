#include "CueSheetParser.hpp"
#include "metadata/TagLibMetadataAdapter.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unordered_set>

namespace fs = std::filesystem;

namespace adapters {

static std::string trimString(const std::string &str) {
  size_t start = str.find_first_not_of(" \t\r\n\"");
  if (start == std::string::npos)
    return "";
  size_t end = str.find_last_not_of(" \t\r\n\"");
  return str.substr(start, end - start + 1);
}

static bool startsWithCi(const std::string &str, const std::string &prefix) {
  if (str.size() < prefix.size())
    return false;
  return std::equal(prefix.begin(), prefix.end(), str.begin(),
                    [](char a, char b) {
                      return std::tolower(static_cast<unsigned char>(a)) ==
                             std::tolower(static_cast<unsigned char>(b));
                    });
}

static std::string extractValue(const std::string &line, size_t prefixLen) {
  if (line.size() <= prefixLen)
    return "";
  std::string rest = line.substr(prefixLen);
  size_t start = rest.find_first_not_of(" \t\r\n");
  if (start == std::string::npos)
    return "";
  rest = rest.substr(start);

  if (!rest.empty() && rest.front() == '"') {
    size_t endQuote = rest.find('"', 1);
    if (endQuote != std::string::npos) {
      return rest.substr(1, endQuote - 1);
    }
    return rest.substr(1);
  }

  size_t end = rest.find_last_not_of(" \t\r\n");
  return (end == std::string::npos) ? "" : rest.substr(0, end + 1);
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
  if (clean.empty())
    return 0;

  std::vector<std::string> parts;
  size_t start = 0;
  while (true) {
    size_t colon = clean.find(':', start);
    if (colon == std::string::npos) {
      parts.push_back(clean.substr(start));
      break;
    }
    parts.push_back(clean.substr(start, colon - start));
    start = colon + 1;
  }

  try {
    if (parts.size() == 3) {
      // Standard CUE: MM:SS:FF (FF = 75 frames per second)
      uint64_t m = std::stoull(parts[0]);
      uint64_t s = std::stoull(parts[1]);
      uint64_t f = std::stoull(parts[2]);
      return (m * 60 + s) * 1000 + (f * 1000) / 75;
    } else if (parts.size() == 4) {
      // HH:MM:SS:FF
      uint64_t h = std::stoull(parts[0]);
      uint64_t m = std::stoull(parts[1]);
      uint64_t s = std::stoull(parts[2]);
      uint64_t f = std::stoull(parts[3]);
      return (h * 3600 + m * 60 + s) * 1000 + (f * 1000) / 75;
    } else if (parts.size() == 2) {
      // MM:SS or MM:SS.mmm
      uint64_t m = std::stoull(parts[0]);
      double s = std::stod(parts[1]);
      return static_cast<uint64_t>(m * 60000 + s * 1000);
    } else if (parts.size() == 1) {
      double s = std::stod(parts[0]);
      return static_cast<uint64_t>(s * 1000);
    }
  } catch (...) {
    return 0;
  }
  return 0;
}

std::string
CueSheetParser::resolveAudioFile(const std::string &cueFilePath,
                                 const std::string &referencedFilename) {
  fs::path cuePath(cueFilePath);
  fs::path dir = cuePath.parent_path();
  std::string cleanRef = trimString(referencedFilename);

  static const std::vector<std::string> audioExts = {
      ".flac", ".wav", ".m4a", ".alac", ".mp3", ".ape",
      ".wv",   ".ogg", ".oga", ".opus", ".aac", ".aiff",
      ".aif",  ".wma", ".mp4", ".m4b",  ".dsf", ".dff"};

  // 1. Direct path check
  if (!cleanRef.empty()) {
    fs::path direct = dir / fs::path(cleanRef);
    if (fs::exists(direct) && fs::is_regular_file(direct)) {
      return direct.string();
    }

    // 2. Case-insensitive search for referenced filename in dir
    try {
      std::string refLower = cleanRef;
      std::transform(refLower.begin(), refLower.end(), refLower.begin(),
                     ::tolower);
      for (const auto &entry : fs::directory_iterator(dir)) {
        if (entry.is_regular_file()) {
          std::string nameLower = entry.path().filename().string();
          std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(),
                         ::tolower);
          if (nameLower == refLower) {
            return entry.path().string();
          }
        }
      }
    } catch (...) {
    }

    // 3. Same stem as referenced filename with common audio extensions
    std::string refStem = fs::path(cleanRef).stem().string();
    for (const auto &ext : audioExts) {
      fs::path cand = dir / fs::path(refStem + ext);
      if (fs::exists(cand) && fs::is_regular_file(cand)) {
        return cand.string();
      }
      std::string extUpper = ext;
      std::transform(extUpper.begin(), extUpper.end(), extUpper.begin(),
                     ::toupper);
      cand = dir / fs::path(refStem + extUpper);
      if (fs::exists(cand) && fs::is_regular_file(cand)) {
        return cand.string();
      }
    }
  }

  // 4. Same stem as CUE file with common audio extensions
  std::string cueStem = cuePath.stem().string();
  for (const auto &ext : audioExts) {
    fs::path cand = dir / fs::path(cueStem + ext);
    if (fs::exists(cand) && fs::is_regular_file(cand)) {
      return cand.string();
    }
    std::string extUpper = ext;
    std::transform(extUpper.begin(), extUpper.end(), extUpper.begin(),
                   ::toupper);
    cand = dir / fs::path(cueStem + extUpper);
    if (fs::exists(cand) && fs::is_regular_file(cand)) {
      return cand.string();
    }
  }

  // 5. Look for any audio file in dir: if there is only 1 audio file, use it!
  std::vector<fs::path> dirAudioFiles;
  try {
    for (const auto &entry : fs::directory_iterator(dir)) {
      if (entry.is_regular_file()) {
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (std::find(audioExts.begin(), audioExts.end(), ext) !=
            audioExts.end()) {
          dirAudioFiles.push_back(entry.path());
        }
      }
    }
  } catch (...) {
  }

  if (dirAudioFiles.size() == 1) {
    return dirAudioFiles[0].string();
  }

  // 6. If multiple audio files in dir, pick the one matching cueStem or largest
  if (!dirAudioFiles.empty()) {
    std::string cueStemLower = cueStem;
    std::transform(cueStemLower.begin(), cueStemLower.end(),
                   cueStemLower.begin(), ::tolower);
    for (const auto &p : dirAudioFiles) {
      std::string fnameLower = p.stem().string();
      std::transform(fnameLower.begin(), fnameLower.end(), fnameLower.begin(),
                     ::tolower);
      if (fnameLower.find(cueStemLower) != std::string::npos ||
          cueStemLower.find(fnameLower) != std::string::npos) {
        return p.string();
      }
    }
    auto largest =
        std::max_element(dirAudioFiles.begin(), dirAudioFiles.end(),
                         [](const fs::path &a, const fs::path &b) {
                           try {
                             return fs::file_size(a) < fs::file_size(b);
                           } catch (...) {
                             return false;
                           }
                         });
    return largest->string();
  }

  return "";
}

std::vector<core::Track>
CueSheetParser::parseFile(const std::string &cueFilePath,
                          uint64_t audioFileDurationMs) {
  fs::path p = fs::path(cueFilePath);
  if (!fs::exists(p)) {
    return {};
  }

  std::ifstream file(p, std::ios::binary);
  if (!file.is_open()) {
    return {};
  }

  std::stringstream ss;
  ss << file.rdbuf();
  std::string content = ss.str();

  // Strip UTF-8 BOM if present
  if (content.size() >= 3 && static_cast<unsigned char>(content[0]) == 0xEF &&
      static_cast<unsigned char>(content[1]) == 0xBB &&
      static_cast<unsigned char>(content[2]) == 0xBF) {
    content = content.substr(3);
  }

  // Find referenced filename in CUE sheet content
  std::string referencedName;
  std::istringstream stream(content);
  std::string line;
  while (std::getline(stream, line)) {
    std::string trimmed = trimString(line);
    if (startsWithCi(trimmed, "FILE ")) {
      referencedName = extractValue(trimmed, 4);
      break;
    }
  }

  std::string baseAudioPath = resolveAudioFile(cueFilePath, referencedName);

  // Probe companion audio file for true audio duration, specs, and cover art
  uint64_t probedDurationMs = 0;
  uint32_t sampleRate = 44100;
  uint8_t bitDepth = 16;
  uint8_t channels = 2;
  uint32_t bitrate = 0;
  std::string codec = "FLAC";
  std::string artHash;
  uint64_t fileSize = 0;
  uint32_t probedYear = 0;
  std::string probedGenre;
  std::string probedAlbumArtist;

  if (!baseAudioPath.empty() && fs::exists(baseAudioPath)) {
    try {
      TagLibMetadataAdapter extractor;
      auto probed = extractor.extract(baseAudioPath);
      if (probed.has_value()) {
        probedDurationMs = probed->durationMs;
        sampleRate = probed->sampleRate;
        bitDepth = probed->bitDepth;
        channels = probed->channels;
        bitrate = probed->bitrate;
        codec = probed->codec;
        artHash = probed->artHash;
        fileSize = probed->fileSizeBytes;
        probedYear = probed->year;
        probedGenre = probed->genre;
        probedAlbumArtist = probed->albumArtist;
      }
    } catch (...) {
    }
  }

  // Fallback artwork discovery from directory images if embedded tag art is
  // missing
  if (artHash.empty()) {
    fs::path dir = p.parent_path();
    static const std::vector<std::string> artNames = {
        "Cover.jpg",  "cover.jpg", "Folder.jpg",   "folder.jpg",
        "front.jpg",  "front.png", "Cover.png",    "cover.png",
        "inside.png", "CD.png",    "front 1-2.png"};
    for (const auto &name : artNames) {
      fs::path cand = dir / name;
      if (fs::exists(cand) && fs::is_regular_file(cand)) {
        artHash = "art_" + simpleHash(cand.string());
        break;
      }
    }
  }

  uint64_t effectiveDurationMs =
      (audioFileDurationMs > 0) ? audioFileDurationMs : probedDurationMs;

  auto tracks = parseString(content, baseAudioPath, effectiveDurationMs);

  // Apply probed audio specs, cover art, and metadata
  for (auto &t : tracks) {
    if (sampleRate > 0)
      t.sampleRate = sampleRate;
    if (bitDepth > 0)
      t.bitDepth = bitDepth;
    if (channels > 0)
      t.channels = channels;
    if (bitrate > 0)
      t.bitrate = bitrate;
    if (!codec.empty())
      t.codec = codec;
    if (!artHash.empty())
      t.artHash = artHash;
    if (fileSize > 0)
      t.fileSizeBytes = fileSize;
    if (t.year == 0 && probedYear > 0)
      t.year = probedYear;
    if (t.genre.empty() && !probedGenre.empty())
      t.genre = probedGenre;
    if ((t.albumArtist.empty() || t.albumArtist == "Various Artists") &&
        !probedAlbumArtist.empty()) {
      t.albumArtist = probedAlbumArtist;
    }
  }

  return tracks;
}

std::vector<core::Track>
CueSheetParser::parseString(const std::string &cueContent,
                            const std::string &baseAudioPath,
                            uint64_t audioFileDurationMs) {
  std::vector<core::Track> tracks;

  std::string cleanContent = cueContent;
  if (cleanContent.size() >= 3 &&
      static_cast<unsigned char>(cleanContent[0]) == 0xEF &&
      static_cast<unsigned char>(cleanContent[1]) == 0xBB &&
      static_cast<unsigned char>(cleanContent[2]) == 0xBF) {
    cleanContent = cleanContent.substr(3);
  }

  std::istringstream stream(cleanContent);
  std::string line;

  std::string albumArtist = "Various Artists";
  std::string albumTitle = "Album";
  std::string albumGenre = "";
  uint32_t albumYear = 0;
  uint32_t albumDiscNumber = 1;
  float replayGainAlbumGain = 0.0f;
  float replayGainAlbumPeak = 1.0f;
  bool hasAlbumReplayGain = false;

  std::string currentAudioFile = baseAudioPath;

  struct TrackEntry {
    core::Track track;
    uint64_t index00Ms{0};
    bool hasIndex00{false};
  };

  std::vector<TrackEntry> rawEntries;
  TrackEntry currentEntry;
  bool inTrack = false;

  uint64_t accumulatedHourOffsetMs = 0;
  uint64_t lastTrackStartMs = 0;

  while (std::getline(stream, line)) {
    // Strip carriage return and leading/trailing whitespace
    while (!line.empty() &&
           (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
      line.pop_back();
    }
    size_t firstNonSpace = line.find_first_not_of(" \t");
    if (firstNonSpace == std::string::npos)
      continue;
    std::string trimmed = line.substr(firstNonSpace);

    if (startsWithCi(trimmed, "REM ")) {
      std::string remContent = trimmed.substr(4);
      size_t remStart = remContent.find_first_not_of(" \t");
      if (remStart != std::string::npos) {
        remContent = remContent.substr(remStart);
      }
      if (startsWithCi(remContent, "GENRE ")) {
        std::string g = extractValue(remContent, 5);
        if (inTrack) {
          currentEntry.track.genre = g;
        } else {
          albumGenre = g;
        }
      } else if (startsWithCi(remContent, "DATE ") ||
                 startsWithCi(remContent, "YEAR ")) {
        std::string d = extractValue(remContent, 4);
        try {
          uint32_t y = static_cast<uint32_t>(std::stoul(d));
          if (inTrack) {
            currentEntry.track.year = y;
          } else {
            albumYear = y;
          }
        } catch (...) {
        }
      } else if (startsWithCi(remContent, "DISCNUMBER ") ||
                 startsWithCi(remContent, "DISC ")) {
        std::string d = extractValue(remContent, 4);
        try {
          albumDiscNumber = static_cast<uint32_t>(std::stoul(d));
        } catch (...) {
        }
      } else if (startsWithCi(remContent, "REPLAYGAIN_ALBUM_GAIN ")) {
        try {
          replayGainAlbumGain = std::stof(extractValue(remContent, 21));
          hasAlbumReplayGain = true;
        } catch (...) {
        }
      } else if (startsWithCi(remContent, "REPLAYGAIN_ALBUM_PEAK ")) {
        try {
          replayGainAlbumPeak = std::stof(extractValue(remContent, 21));
          hasAlbumReplayGain = true;
        } catch (...) {
        }
      } else if (inTrack &&
                 startsWithCi(remContent, "REPLAYGAIN_TRACK_GAIN ")) {
        try {
          currentEntry.track.replayGainTrackGain =
              std::stof(extractValue(remContent, 21));
          currentEntry.track.hasReplayGain = true;
        } catch (...) {
        }
      } else if (inTrack &&
                 startsWithCi(remContent, "REPLAYGAIN_TRACK_PEAK ")) {
        try {
          currentEntry.track.replayGainTrackPeak =
              std::stof(extractValue(remContent, 21));
          currentEntry.track.hasReplayGain = true;
        } catch (...) {
        }
      }
    } else if (startsWithCi(trimmed, "PERFORMER ") && !inTrack) {
      albumArtist = extractValue(trimmed, 9);
    } else if (startsWithCi(trimmed, "TITLE ") && !inTrack) {
      albumTitle = extractValue(trimmed, 5);
    } else if (startsWithCi(trimmed, "DISC ") && !inTrack) {
      try {
        albumDiscNumber =
            static_cast<uint32_t>(std::stoul(extractValue(trimmed, 4)));
      } catch (...) {
      }
    } else if (startsWithCi(trimmed, "FILE ")) {
      std::string fname = extractValue(trimmed, 4);
      if (!fname.empty()) {
        if (!baseAudioPath.empty()) {
          // If baseAudioPath was specified and matches or is adjacent
          fs::path dir = fs::path(baseAudioPath).parent_path();
          fs::path direct = dir / fs::path(fname);
          if (fs::exists(direct)) {
            currentAudioFile = direct.string();
          } else {
            currentAudioFile = baseAudioPath;
          }
        } else {
          currentAudioFile = fname;
        }
      }
    } else if (startsWithCi(trimmed, "TRACK ")) {
      if (inTrack) {
        rawEntries.push_back(currentEntry);
      }

      inTrack = true;
      currentEntry = TrackEntry{};
      currentEntry.track = core::Track{};
      currentEntry.track.album = albumTitle;
      currentEntry.track.albumArtist = albumArtist;
      currentEntry.track.artist = albumArtist;
      currentEntry.track.genre = albumGenre;
      currentEntry.track.year = albumYear;
      currentEntry.track.discNumber = albumDiscNumber;
      currentEntry.track.isVirtualTrack = true;

      std::string rest = extractValue(trimmed, 5);
      size_t spacePos = rest.find(' ');
      std::string numStr =
          (spacePos != std::string::npos) ? rest.substr(0, spacePos) : rest;
      try {
        currentEntry.track.trackNumber =
            static_cast<uint32_t>(std::stoul(numStr));
      } catch (...) {
        currentEntry.track.trackNumber =
            static_cast<uint32_t>(rawEntries.size() + 1);
      }

      currentEntry.track.title =
          "Track " + std::to_string(currentEntry.track.trackNumber);
      currentEntry.track.filePath =
          currentAudioFile +
          "#cue:" + std::to_string(currentEntry.track.trackNumber);
      currentEntry.track.id = "cue_" + simpleHash(currentEntry.track.filePath);
      currentEntry.track.codec = "FLAC";
    } else if (inTrack && startsWithCi(trimmed, "TITLE ")) {
      currentEntry.track.title = extractValue(trimmed, 5);
    } else if (inTrack && startsWithCi(trimmed, "PERFORMER ")) {
      currentEntry.track.artist = extractValue(trimmed, 9);
    } else if (inTrack && startsWithCi(trimmed, "SONGWRITER ")) {
      if (currentEntry.track.artist.empty() ||
          currentEntry.track.artist == albumArtist) {
        currentEntry.track.artist = extractValue(trimmed, 10);
      }
    } else if (inTrack && startsWithCi(trimmed, "INDEX 00 ")) {
      std::string ts = extractValue(trimmed, 8);
      uint64_t rawMs = parseIndexTimestampMs(ts);
      currentEntry.index00Ms = rawMs + accumulatedHourOffsetMs;
      currentEntry.hasIndex00 = true;
    } else if (inTrack && startsWithCi(trimmed, "INDEX 01 ")) {
      std::string ts = extractValue(trimmed, 8);
      uint64_t rawMs = parseIndexTimestampMs(ts);

      // Handle 60-minute rollover / modulo 60 timestamps in live and long
      // concert files
      if (rawMs + accumulatedHourOffsetMs < lastTrackStartMs) {
        while (rawMs + accumulatedHourOffsetMs < lastTrackStartMs) {
          accumulatedHourOffsetMs += 3600000ULL;
        }
      }

      uint64_t trackStart = rawMs + accumulatedHourOffsetMs;
      lastTrackStartMs = trackStart;
      currentEntry.track.startTimeMs = trackStart;
    }
  }

  if (inTrack) {
    rawEntries.push_back(currentEntry);
  }

  // Calculate accurate durations across virtual track boundaries
  tracks.reserve(rawEntries.size());
  for (size_t i = 0; i < rawEntries.size(); ++i) {
    auto t = rawEntries[i].track;
    if (i + 1 < rawEntries.size()) {
      // If next track has INDEX 00 (pre-gap), end at next track's INDEX 00
      if (rawEntries[i + 1].hasIndex00 &&
          rawEntries[i + 1].index00Ms > t.startTimeMs) {
        t.endTimeMs = rawEntries[i + 1].index00Ms;
      } else {
        t.endTimeMs = rawEntries[i + 1].track.startTimeMs;
      }
      t.durationMs =
          (t.endTimeMs > t.startTimeMs) ? (t.endTimeMs - t.startTimeMs) : 0;
    } else {
      // Last track
      if (audioFileDurationMs > t.startTimeMs) {
        t.endTimeMs = audioFileDurationMs;
        t.durationMs = audioFileDurationMs - t.startTimeMs;
      } else {
        t.endTimeMs = t.startTimeMs;
        t.durationMs = 0;
      }
    }

    if (hasAlbumReplayGain) {
      t.replayGainAlbumGain = replayGainAlbumGain;
      t.replayGainAlbumPeak = replayGainAlbumPeak;
      t.hasReplayGain = true;
    }

    tracks.push_back(std::move(t));
  }

  return tracks;
}

} // namespace adapters