#include "TagLibMetadataAdapter.hpp"

#include <taglib/aifffile.h>
#include <taglib/apefile.h>
#include <taglib/apetag.h>
#include <taglib/asffile.h>
#include <taglib/asfpicture.h>
#include <taglib/asftag.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/audioproperties.h>
#if __has_include(<taglib/dsdifffile.h>)
#include <taglib/dsdifffile.h>
#define PIC_AUDIO_HAS_TAGLIB_DSDIFF 1
#endif
#if __has_include(<taglib/dsffile.h>)
#include <taglib/dsffile.h>
#define PIC_AUDIO_HAS_TAGLIB_DSF 1
#endif
#include <taglib/fileref.h>
#include <taglib/flacfile.h>
#include <taglib/flacpicture.h>
#include <taglib/id3v2tag.h>
#include <taglib/mp4coverart.h>
#include <taglib/mp4file.h>
#include <taglib/mp4tag.h>
#include <taglib/mpegfile.h>
#include <taglib/opusfile.h>
#include <taglib/tag.h>
#include <taglib/tbytevector.h>
#include <taglib/tfile.h>
#include <taglib/tpropertymap.h>
#include <taglib/unsynchronizedlyricsframe.h>
#include <taglib/vorbisfile.h>
#include <taglib/wavfile.h>
#include <taglib/wavpackfile.h>
#include <taglib/xiphcomment.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

namespace adapters {

TagLibMetadataAdapter::TagLibMetadataAdapter() = default;

void TagLibMetadataAdapter::setArtworkPriority(bool preferEmbedded) {
  m_preferEmbedded = preferEmbedded;
}

std::string TagLibMetadataAdapter::toLower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return s;
}

std::string
TagLibMetadataAdapter::getFileExtension(const std::string &filePath) const {
  fs::path p(filePath);
  return toLower(p.extension().string());
}

bool TagLibMetadataAdapter::supportsFormat(const std::string &extension) const {
  std::string ext = toLower(extension);
  if (!ext.empty() && ext[0] != '.') {
    ext = "." + ext;
  }
  return (ext == ".flac" || ext == ".wav" || ext == ".aiff" || ext == ".aif" ||
          ext == ".mp3" || ext == ".m4a" || ext == ".alac" || ext == ".mp4" ||
          ext == ".m4b" || ext == ".aac" || ext == ".ogg" || ext == ".oga" ||
          ext == ".opus" || ext == ".spx" || ext == ".dsf" || ext == ".dff" ||
          ext == ".ape" || ext == ".wv" || ext == ".wma");
}

std::string TagLibMetadataAdapter::computeHash(const uint8_t *data,
                                               size_t size) {
  uint64_t hash = 14695981039346656037ULL;
  for (size_t i = 0; i < size; ++i) {
    hash ^= data[i];
    hash *= 1099511628211ULL;
  }
  std::stringstream ss;
  ss << std::hex << std::setw(16) << std::setfill('0') << hash;
  return ss.str();
}

std::string TagLibMetadataAdapter::detectMimeType(const uint8_t *data,
                                                  size_t size) {
  if (size >= 3 && data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF) {
    return "image/jpeg";
  }
  if (size >= 8 && data[0] == 0x89 && data[1] == 0x50 && data[2] == 0x4E &&
      data[3] == 0x47 && data[4] == 0x0D && data[5] == 0x0A &&
      data[6] == 0x1A && data[7] == 0x0A) {
    return "image/png";
  }
  if (size >= 12 && data[0] == 'R' && data[1] == 'I' && data[2] == 'F' &&
      data[3] == 'F' && data[8] == 'W' && data[9] == 'E' && data[10] == 'B' &&
      data[11] == 'P') {
    return "image/webp";
  }
  if (size >= 6 && data[0] == 'G' && data[1] == 'I' && data[2] == 'F' &&
      data[3] == '8' && (data[4] == '7' || data[4] == '9') && data[5] == 'a') {
    return "image/gif";
  }
  if (size >= 2 && data[0] == 'B' && data[1] == 'M') {
    return "image/bmp";
  }
  return "image/jpeg";
}

std::optional<core::ExtractedArtwork>
TagLibMetadataAdapter::extractFromId3v2(TagLib::ID3v2::Tag *id3v2) {
  if (!id3v2) {
    return std::nullopt;
  }

  const TagLib::ID3v2::AttachedPictureFrame *selected = nullptr;

  const auto &apicList = id3v2->frameList("APIC");
  for (auto *f : apicList) {
    auto *apic = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame *>(f);
    if (apic && apic->picture().size() > 0) {
      if (apic->type() == TagLib::ID3v2::AttachedPictureFrame::FrontCover) {
        selected = apic;
        break;
      }
      if (!selected) {
        selected = apic;
      }
    }
  }

  if (!selected) {
    const auto &picList = id3v2->frameList("PIC");
    for (auto *f : picList) {
      auto *pic = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame *>(f);
      if (pic && pic->picture().size() > 0) {
        if (pic->type() == TagLib::ID3v2::AttachedPictureFrame::FrontCover) {
          selected = pic;
          break;
        }
        if (!selected) {
          selected = pic;
        }
      }
    }
  }

  if (selected) {
    TagLib::ByteVector picData = selected->picture();
    if (!picData.isEmpty()) {
      core::ExtractedArtwork art;
      art.data.assign(picData.data(), picData.data() + picData.size());
      std::string mime = selected->mimeType().to8Bit(true);
      art.mimeType = mime.empty()
                         ? detectMimeType(art.data.data(), art.data.size())
                         : mime;
      art.artHash = computeHash(art.data.data(), art.data.size());
      return art;
    }
  }

  return std::nullopt;
}

std::optional<core::ExtractedArtwork>
TagLibMetadataAdapter::extractFromXiphComment(
    TagLib::Ogg::XiphComment *comment) {
  if (!comment) {
    return std::nullopt;
  }

  // TagLib 2.x parses METADATA_BLOCK_PICTURE directly into pictureList()
  const auto &pics = comment->pictureList();
  if (!pics.isEmpty()) {
    const TagLib::FLAC::Picture *selectedPic = nullptr;
    for (const auto *pic : pics) {
      if (pic) {
        TagLib::ByteVector picData = pic->data();
        if (!picData.isEmpty()) {
          if (pic->type() == TagLib::FLAC::Picture::FrontCover) {
            selectedPic = pic;
            break;
          }
          if (!selectedPic) {
            selectedPic = pic;
          }
        }
      }
    }
    if (selectedPic) {
      TagLib::ByteVector picData = selectedPic->data();
      if (!picData.isEmpty()) {
        core::ExtractedArtwork art;
        art.data.assign(picData.data(), picData.data() + picData.size());
        art.mimeType = detectMimeType(art.data.data(), art.data.size());
        art.artHash = computeHash(art.data.data(), art.data.size());
        return art;
      }
    }
  }

  // Fallback for raw base64 Vorbis comment fields (case-insensitive)
  const auto &fieldMap = comment->fieldListMap();
  for (const auto &[key, list] : fieldMap) {
    std::string k = toLower(key.to8Bit(true));
    if (k == "metadata_block_picture") {
      for (const auto &str : list) {
        TagLib::ByteVector b64(str.toCString(true));
        TagLib::ByteVector decoded = TagLib::ByteVector::fromBase64(b64);
        if (decoded.size() > 32) {
          TagLib::FLAC::Picture pic;
          if (pic.parse(decoded)) {
            TagLib::ByteVector picData = pic.data();
            if (!picData.isEmpty()) {
              core::ExtractedArtwork art;
              art.data.assign(picData.data(), picData.data() + picData.size());
              art.mimeType = detectMimeType(art.data.data(), art.data.size());
              art.artHash = computeHash(art.data.data(), art.data.size());
              return art;
            }
          }
        }
      }
    } else if (k == "coverart") {
      for (const auto &str : list) {
        TagLib::ByteVector b64(str.toCString(true));
        TagLib::ByteVector decoded = TagLib::ByteVector::fromBase64(b64);
        if (decoded.size() > 16) {
          core::ExtractedArtwork art;
          art.data.assign(decoded.data(), decoded.data() + decoded.size());
          art.mimeType = detectMimeType(art.data.data(), art.data.size());
          art.artHash = computeHash(art.data.data(), art.data.size());
          return art;
        }
      }
    }
  }

  return std::nullopt;
}

std::optional<core::ExtractedArtwork>
TagLibMetadataAdapter::extractFromApe(TagLib::APE::Tag *apeTag) {
  if (!apeTag) {
    return std::nullopt;
  }
  const auto &items = apeTag->itemListMap();
  const std::vector<std::string> keys = {"Cover Art (Front)", "Cover Art",
                                         "Cover Art (Back)"};
  for (const auto &k : keys) {
    if (items.contains(k.c_str())) {
      const auto &item = items[k.c_str()];
      TagLib::ByteVector val = item.binaryData();
      int nullPos = val.find('\0');
      if (nullPos >= 0 && nullPos + 1 < static_cast<int>(val.size())) {
        core::ExtractedArtwork art;
        const char *dataPtr = val.data() + nullPos + 1;
        size_t dataSize = val.size() - nullPos - 1;
        art.data.assign(dataPtr, dataPtr + dataSize);
        art.mimeType = detectMimeType(art.data.data(), art.data.size());
        art.artHash = computeHash(art.data.data(), art.data.size());
        return art;
      }
    }
  }
  return std::nullopt;
}

std::optional<core::ExtractedArtwork>
TagLibMetadataAdapter::extractEmbeddedArtwork(const std::string &filePath) {
  std::string ext = getFileExtension(filePath);

  if (ext == ".flac") {
    TagLib::FLAC::File flacFile(filePath.c_str());
    if (flacFile.isValid()) {
      const auto &pictures = flacFile.pictureList();
      const TagLib::FLAC::Picture *selectedPic = nullptr;
      for (const auto *pic : pictures) {
        if (pic) {
          TagLib::ByteVector picData = pic->data();
          if (!picData.isEmpty()) {
            if (pic->type() == TagLib::FLAC::Picture::FrontCover) {
              selectedPic = pic;
              break;
            }
            if (!selectedPic) {
              selectedPic = pic;
            }
          }
        }
      }
      if (selectedPic) {
        TagLib::ByteVector picData = selectedPic->data();
        if (!picData.isEmpty()) {
          core::ExtractedArtwork art;
          art.data.assign(picData.data(), picData.data() + picData.size());
          std::string mime = selectedPic->mimeType().to8Bit(true);
          art.mimeType = mime.empty()
                             ? detectMimeType(art.data.data(), art.data.size())
                             : mime;
          art.artHash = computeHash(art.data.data(), art.data.size());
          return art;
        }
      }

      if (flacFile.xiphComment()) {
        auto art = extractFromXiphComment(flacFile.xiphComment());
        if (art)
          return art;
      }

      if (flacFile.ID3v2Tag()) {
        auto art = extractFromId3v2(flacFile.ID3v2Tag());
        if (art)
          return art;
      }
    }
  }

  if (ext == ".mp3") {
    TagLib::MPEG::File mpegFile(filePath.c_str());
    if (mpegFile.isValid()) {
      if (mpegFile.ID3v2Tag()) {
        auto art = extractFromId3v2(mpegFile.ID3v2Tag());
        if (art)
          return art;
      }
      if (mpegFile.APETag()) {
        auto art = extractFromApe(mpegFile.APETag());
        if (art)
          return art;
      }
    }
  }

  if (ext == ".m4a" || ext == ".alac" || ext == ".mp4" || ext == ".m4b" ||
      ext == ".aac") {
    TagLib::MP4::File mp4File(filePath.c_str());
    if (mp4File.isValid() && mp4File.tag()) {
      auto *mp4Tag = mp4File.tag();
      if (mp4Tag->itemMap().contains("covr")) {
        TagLib::MP4::CoverArtList coverList =
            mp4Tag->itemMap()["covr"].toCoverArtList();
        if (!coverList.isEmpty()) {
          const auto &cover = coverList.front();
          TagLib::ByteVector picData = cover.data();
          if (!picData.isEmpty()) {
            core::ExtractedArtwork art;
            art.data.assign(picData.data(), picData.data() + picData.size());
            art.mimeType = detectMimeType(art.data.data(), art.data.size());
            art.artHash = computeHash(art.data.data(), art.data.size());
            return art;
          }
        }
      }
    }
    if (ext == ".aac") {
      TagLib::MPEG::File mpegFile(filePath.c_str());
      if (mpegFile.isValid() && mpegFile.ID3v2Tag()) {
        auto art = extractFromId3v2(mpegFile.ID3v2Tag());
        if (art)
          return art;
      }
    }
  }

  if (ext == ".wav") {
    TagLib::RIFF::WAV::File wavFile(filePath.c_str());
    if (wavFile.isValid() && wavFile.ID3v2Tag()) {
      auto art = extractFromId3v2(wavFile.ID3v2Tag());
      if (art)
        return art;
    }
  }

  if (ext == ".aiff" || ext == ".aif") {
    TagLib::RIFF::AIFF::File aiffFile(filePath.c_str());
    if (aiffFile.isValid() && aiffFile.tag()) {
      auto *id3v2 = dynamic_cast<TagLib::ID3v2::Tag *>(aiffFile.tag());
      if (id3v2) {
        auto art = extractFromId3v2(id3v2);
        if (art)
          return art;
      }
    }
  }

  if (ext == ".ogg" || ext == ".oga") {
    TagLib::Ogg::Vorbis::File vorbisFile(filePath.c_str());
    if (vorbisFile.isValid() && vorbisFile.tag()) {
      auto art = extractFromXiphComment(vorbisFile.tag());
      if (art)
        return art;
    }
    TagLib::Ogg::Opus::File opusFile(filePath.c_str());
    if (opusFile.isValid() && opusFile.tag()) {
      auto art = extractFromXiphComment(opusFile.tag());
      if (art)
        return art;
    }
  }

  if (ext == ".opus" || ext == ".spx") {
    TagLib::Ogg::Opus::File opusFile(filePath.c_str());
    if (opusFile.isValid() && opusFile.tag()) {
      auto art = extractFromXiphComment(opusFile.tag());
      if (art)
        return art;
    }
    TagLib::Ogg::Vorbis::File vorbisFile(filePath.c_str());
    if (vorbisFile.isValid() && vorbisFile.tag()) {
      auto art = extractFromXiphComment(vorbisFile.tag());
      if (art)
        return art;
    }
  }

  if (ext == ".dsf") {
#if defined(PIC_AUDIO_HAS_TAGLIB_DSF)
    TagLib::DSF::File dsfFile(filePath.c_str());
    if (dsfFile.isValid() && dsfFile.tag()) {
      auto *id3v2 = dynamic_cast<TagLib::ID3v2::Tag *>(dsfFile.tag());
      if (id3v2) {
        auto art = extractFromId3v2(id3v2);
        if (art)
          return art;
      }
    }
#endif
  }

  if (ext == ".dff") {
#if defined(PIC_AUDIO_HAS_TAGLIB_DSDIFF)
    TagLib::DSDIFF::File dffFile(filePath.c_str());
    if (dffFile.isValid() && dffFile.ID3v2Tag()) {
      auto art = extractFromId3v2(dffFile.ID3v2Tag());
      if (art)
        return art;
    }
#endif
  }

  if (ext == ".ape") {
    TagLib::APE::File apeFile(filePath.c_str());
    if (apeFile.isValid() && apeFile.APETag()) {
      auto art = extractFromApe(apeFile.APETag());
      if (art)
        return art;
    }
  }

  if (ext == ".wv") {
    TagLib::WavPack::File wvFile(filePath.c_str());
    if (wvFile.isValid()) {
      if (wvFile.APETag()) {
        auto art = extractFromApe(wvFile.APETag());
        if (art)
          return art;
      }
    }
  }

  if (ext == ".wma") {
    TagLib::ASF::File asfFile(filePath.c_str());
    if (asfFile.isValid() && asfFile.tag()) {
      auto *asfTag = asfFile.tag();
      const auto &attrMap = asfTag->attributeListMap();
      if (attrMap.contains("WM/Picture")) {
        const auto &attrs = attrMap["WM/Picture"];
        for (const auto &attr : attrs) {
          TagLib::ASF::Picture pic = attr.toPicture();
          TagLib::ByteVector picData = pic.picture();
          if (pic.isValid() && !picData.isEmpty()) {
            core::ExtractedArtwork art;
            art.data.assign(picData.data(), picData.data() + picData.size());
            art.mimeType = detectMimeType(art.data.data(), art.data.size());
            art.artHash = computeHash(art.data.data(), art.data.size());
            return art;
          }
        }
      }
    }
  }

  // Generic FileRef fallback
  TagLib::FileRef fileRef(filePath.c_str());
  if (!fileRef.isNull() && fileRef.file()) {
    auto *id3v2 = dynamic_cast<TagLib::ID3v2::Tag *>(fileRef.file()->tag());
    if (id3v2) {
      auto art = extractFromId3v2(id3v2);
      if (art)
        return art;
    }
  }

  return std::nullopt;
}

std::optional<core::ExtractedArtwork>
TagLibMetadataAdapter::findFolderArtwork(const std::string &filePath) {
  fs::path p(filePath);
  fs::path dir = p.parent_path();
  if (!fs::exists(dir) || !fs::is_directory(dir)) {
    return std::nullopt;
  }

  std::string dirKey = dir.lexically_normal().string();
  {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    auto it = m_folderArtCache.find(dirKey);
    if (it != m_folderArtCache.end()) {
      return it->second;
    }
  }

  std::vector<fs::path> dirsToCheck = {dir};
  std::string dirName = toLower(dir.filename().string());
  if (dirName.find("cd") == 0 || dirName.find("disc") == 0 ||
      dirName.find("disk") == 0) {
    fs::path parent = dir.parent_path();
    if (fs::exists(parent) && fs::is_directory(parent)) {
      dirsToCheck.push_back(parent);
    }
  }

  auto isImageExt = [](const std::string &ext) {
    return ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".webp" ||
           ext == ".bmp" || ext == ".gif" || ext == ".jpe";
  };

  fs::path bestCandidate;
  int bestPriority = 999;

  for (const auto &d : dirsToCheck) {
    try {
      int imageCount = 0;
      fs::path singleImage;

      for (const auto &entry : fs::directory_iterator(
               d, fs::directory_options::skip_permission_denied)) {
        if (!entry.is_regular_file())
          continue;

        std::string ext = toLower(entry.path().extension().string());
        if (!isImageExt(ext))
          continue;

        imageCount++;
        singleImage = entry.path();

        std::string stem = toLower(entry.path().stem().string());
        int priority = 100;

        if (stem == "cover" || stem == "folder" || stem == "front") {
          priority = 1;
        } else if (stem == "album" || stem == "artwork") {
          priority = 2;
        } else if (stem.rfind("cover", 0) == 0 ||
                   stem.rfind("folder", 0) == 0 ||
                   stem.rfind("front", 0) == 0) {
          priority = 3;
        } else if (stem == "albumart" || stem == "albumartsmall" ||
                   stem == "disc" || stem == "cd" || stem == "sleeve") {
          priority = 4;
        } else {
          priority = 50;
        }

        if (priority < bestPriority) {
          bestPriority = priority;
          bestCandidate = entry.path();
        }
      }

      if (bestPriority > 50 && imageCount == 1) {
        bestCandidate = singleImage;
        bestPriority = 10;
      }
    } catch (...) {
    }

    if (bestPriority <= 2) {
      break;
    }
  }

  std::optional<core::ExtractedArtwork> result = std::nullopt;
  if (!bestCandidate.empty() && fs::exists(bestCandidate)) {
    try {
      size_t size = fs::file_size(bestCandidate);
      if (size > 0 && size < 50 * 1024 * 1024) {
        core::ExtractedArtwork art;
        art.data.resize(size);
        std::ifstream ifs(bestCandidate, std::ios::binary);
        if (ifs) {
          ifs.read(reinterpret_cast<char *>(art.data.data()), size);
          if (ifs.gcount() == static_cast<std::streamsize>(size)) {
            art.mimeType = detectMimeType(art.data.data(), art.data.size());
            art.artHash = computeHash(art.data.data(), art.data.size());
            result = art;
          }
        }
      }
    } catch (...) {
    }
  }

  {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_folderArtCache[dirKey] = result;
  }

  return result;
}

std::optional<core::ExtractedArtwork>
TagLibMetadataAdapter::extractArtwork(const std::string &filePath) {
  if (!fs::exists(filePath)) {
    return std::nullopt;
  }

  if (m_preferEmbedded) {
    auto embeddedArt = extractEmbeddedArtwork(filePath);
    if (embeddedArt.has_value() && !embeddedArt->data.empty()) {
      return embeddedArt;
    }
    return findFolderArtwork(filePath);
  }

  auto folderArt = findFolderArtwork(filePath);
  if (folderArt.has_value() && !folderArt->data.empty()) {
    return folderArt;
  }
  return extractEmbeddedArtwork(filePath);
}

std::optional<core::Track>
TagLibMetadataAdapter::extract(const std::string &filePath) {
  if (!fs::exists(filePath)) {
    return std::nullopt;
  }

  core::Track track;
  track.filePath = filePath;
  fs::path p(filePath);

  track.title = p.stem().string();
  track.artist = "Unknown Artist";
  track.album = "Unknown Album";
  track.albumArtist = "";
  track.genre = "Unknown Genre";
  track.fileSizeBytes = fs::file_size(filePath);

  auto lastWriteTime = fs::last_write_time(filePath);
  auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
      lastWriteTime - fs::file_time_type::clock::now() +
      std::chrono::system_clock::now());
  track.dateAdded =
      std::chrono::duration_cast<std::chrono::seconds>(sctp.time_since_epoch())
          .count();

  std::string ext = getFileExtension(filePath);
  if (ext == ".flac") {
    track.codec = "FLAC";
  } else if (ext == ".wav") {
    track.codec = "WAV";
  } else if (ext == ".aiff" || ext == ".aif") {
    track.codec = "AIFF";
  } else if (ext == ".dsf" || ext == ".dff") {
    track.codec = "DSD";
  } else if (ext == ".mp3") {
    track.codec = "MP3";
  } else if (ext == ".m4a" || ext == ".alac" || ext == ".mp4" ||
             ext == ".m4b") {
    track.codec = "ALAC";
  } else if (ext == ".aac") {
    track.codec = "AAC";
  } else if (ext == ".opus") {
    track.codec = "OPUS";
  } else if (ext == ".ogg") {
    track.codec = "VORBIS";
  } else if (ext == ".ape") {
    track.codec = "APE";
  } else if (ext == ".wv") {
    track.codec = "WAVPACK";
  } else {
    track.codec = "PCM";
  }

  TagLib::FileRef fileRef(filePath.c_str());
  if (!fileRef.isNull() && fileRef.tag()) {
    auto *tag = fileRef.tag();
    if (!tag->title().isEmpty())
      track.title = tag->title().to8Bit(true);
    if (!tag->artist().isEmpty())
      track.artist = tag->artist().to8Bit(true);
    if (!tag->album().isEmpty())
      track.album = tag->album().to8Bit(true);
    if (!tag->genre().isEmpty())
      track.genre = tag->genre().to8Bit(true);
    track.year = tag->year();
    track.trackNumber = tag->track();

    if (fileRef.audioProperties()) {
      auto *props = fileRef.audioProperties();
      track.sampleRate = props->sampleRate();
      track.channels = props->channels();
      track.bitrate = props->bitrate();
      track.durationMs = props->lengthInMilliseconds();
      track.bitDepth = 16;
    }
  }

  // Specialized bit depth and container inspection
  if (ext == ".flac") {
    TagLib::FLAC::File flacFile(filePath.c_str());
    if (flacFile.isValid()) {
      if (flacFile.audioProperties()) {
        auto *props = flacFile.audioProperties();
        track.sampleRate = props->sampleRate();
        track.bitDepth = props->bitsPerSample();
        track.channels = props->channels();
        track.bitrate = props->bitrate();
        track.durationMs = props->lengthInMilliseconds();
      }
    }
  } else if (ext == ".wav") {
    TagLib::RIFF::WAV::File wavFile(filePath.c_str());
    if (wavFile.isValid()) {
      if (wavFile.audioProperties()) {
        auto *props = wavFile.audioProperties();
        track.sampleRate = props->sampleRate();
        track.bitDepth = props->bitsPerSample();
        track.channels = props->channels();
        track.bitrate = props->bitrate();
        track.durationMs = props->lengthInMilliseconds();
      }
    }
  } else if (ext == ".aiff" || ext == ".aif") {
    TagLib::RIFF::AIFF::File aiffFile(filePath.c_str());
    if (aiffFile.isValid() && aiffFile.audioProperties()) {
      auto *props = aiffFile.audioProperties();
      track.sampleRate = props->sampleRate();
      track.bitDepth = props->bitsPerSample();
      track.channels = props->channels();
      track.bitrate = props->bitrate();
      track.durationMs = props->lengthInMilliseconds();
    }
  }

  // Extract album artist and disc number from TagLib PropertyMap
  if (!fileRef.isNull() && fileRef.file()) {
    auto props = fileRef.file()->properties();

    if (props.contains("ALBUMARTIST") && !props["ALBUMARTIST"].isEmpty()) {
      track.albumArtist = props["ALBUMARTIST"].front().to8Bit(true);
    } else if (props.contains("ALBUM ARTIST") &&
               !props["ALBUM ARTIST"].isEmpty()) {
      track.albumArtist = props["ALBUM ARTIST"].front().to8Bit(true);
    } else if (props.contains("ALBUM_ARTIST") &&
               !props["ALBUM_ARTIST"].isEmpty()) {
      track.albumArtist = props["ALBUM_ARTIST"].front().to8Bit(true);
    } else if (props.contains("ENSEMBLE") && !props["ENSEMBLE"].isEmpty()) {
      track.albumArtist = props["ENSEMBLE"].front().to8Bit(true);
    }

    if (track.albumArtist.empty() && props.contains("COMPILATION") &&
        !props["COMPILATION"].isEmpty()) {
      std::string comp = props["COMPILATION"].front().to8Bit(true);
      if (comp == "1" || comp == "true") {
        track.albumArtist = "Various Artists";
      }
    }

    if (props.contains("DISCNUMBER") && !props["DISCNUMBER"].isEmpty()) {
      try {
        std::string dStr = props["DISCNUMBER"].front().to8Bit(true);
        size_t slashPos = dStr.find('/');
        if (slashPos != std::string::npos) {
          dStr = dStr.substr(0, slashPos);
        }
        track.discNumber = static_cast<uint32_t>(std::stoul(dStr));
      } catch (...) {
      }
    }
  }

  // Fallback to container-specific album artist if PropertyMap did not have it
  if (track.albumArtist.empty()) {
    if (ext == ".mp3") {
      TagLib::MPEG::File mpegFile(filePath.c_str());
      if (mpegFile.isValid() && mpegFile.ID3v2Tag()) {
        auto *id3v2 = mpegFile.ID3v2Tag();
        const auto &frames = id3v2->frameList("TPE2");
        if (!frames.isEmpty()) {
          track.albumArtist = frames.front()->toString().to8Bit(true);
        }
      }
    } else if (ext == ".m4a" || ext == ".alac" || ext == ".mp4" ||
               ext == ".m4b") {
      TagLib::MP4::File mp4File(filePath.c_str());
      if (mp4File.isValid() && mp4File.tag()) {
        auto *mp4Tag = mp4File.tag();
        if (mp4Tag->itemMap().contains("aART")) {
          auto strList = mp4Tag->itemMap()["aART"].toStringList();
          if (!strList.isEmpty()) {
            track.albumArtist = strList.front().to8Bit(true);
          }
        }
      }
    }
  }

  // Artwork extraction
  auto artOpt = extractArtwork(filePath);
  if (artOpt.has_value() && !artOpt->artHash.empty()) {
    track.artHash = artOpt->artHash;
  }

  track.id =
      "trk_" + computeHash(reinterpret_cast<const uint8_t *>(filePath.data()),
                           filePath.size());
  return track;
}

std::string TagLibMetadataAdapter::extractLyrics(const std::string &filePath) {
  if (!fs::exists(filePath)) {
    return "";
  }

  TagLib::FileRef fileRef(filePath.c_str());
  if (!fileRef.isNull() && fileRef.file()) {
    auto props = fileRef.file()->properties();
    if (props.contains("LYRICS") && !props["LYRICS"].isEmpty()) {
      return props["LYRICS"].front().to8Bit(true);
    }
    if (props.contains("UNSYNCEDLYRICS") &&
        !props["UNSYNCEDLYRICS"].isEmpty()) {
      return props["UNSYNCEDLYRICS"].front().to8Bit(true);
    }
  }

  std::string ext = getFileExtension(filePath);
  if (ext == ".mp3" || ext == ".aiff" || ext == ".wav") {
    TagLib::MPEG::File mpegFile(filePath.c_str());
    if (mpegFile.isValid() && mpegFile.ID3v2Tag()) {
      const auto &frames = mpegFile.ID3v2Tag()->frameList("USLT");
      if (!frames.isEmpty()) {
        auto *uslt = dynamic_cast<TagLib::ID3v2::UnsynchronizedLyricsFrame *>(
            frames.front());
        if (uslt) {
          return uslt->text().to8Bit(true);
        }
      }
    }
  } else if (ext == ".flac") {
    TagLib::FLAC::File flacFile(filePath.c_str());
    if (flacFile.isValid() && flacFile.xiphComment()) {
      const auto &map = flacFile.xiphComment()->fieldListMap();
      if (map.contains("LYRICS") && !map["LYRICS"].isEmpty()) {
        return map["LYRICS"].front().to8Bit(true);
      }
      if (map.contains("UNSYNCEDLYRICS") && !map["UNSYNCEDLYRICS"].isEmpty()) {
        return map["UNSYNCEDLYRICS"].front().to8Bit(true);
      }
    }
  }

  return "";
}

} // namespace adapters
