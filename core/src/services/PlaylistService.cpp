#include "core/services/PlaylistService.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace core {

namespace fs = std::filesystem;

namespace {

std::string trim(const std::string &str) {
  size_t first = str.find_first_not_of(" \t\r\n");
  if (first == std::string::npos)
    return "";
  size_t last = str.find_last_not_of(" \t\r\n");
  return str.substr(first, (last - first + 1));
}

std::string toLower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return s;
}

std::string escapeXml(const std::string &data) {
  std::string buffer;
  buffer.reserve(data.size());
  for (char c : data) {
    switch (c) {
    case '&':
      buffer.append("&amp;");
      break;
    case '\"':
      buffer.append("&quot;");
      break;
    case '\'':
      buffer.append("&apos;");
      break;
    case '<':
      buffer.append("&lt;");
      break;
    case '>':
      buffer.append("&gt;");
      break;
    default:
      buffer.push_back(c);
      break;
    }
  }
  return buffer;
}

std::string unescapeXml(const std::string &data) {
  std::string result = data;
  auto replaceAll = [](std::string &str, const std::string &from,
                       const std::string &to) {
    size_t startPos = 0;
    while ((startPos = str.find(from, startPos)) != std::string::npos) {
      str.replace(startPos, from.length(), to);
      startPos += to.length();
    }
  };
  replaceAll(result, "&quot;", "\"");
  replaceAll(result, "&apos;", "\'");
  replaceAll(result, "&lt;", "<");
  replaceAll(result, "&gt;", ">");
  replaceAll(result, "&amp;", "&");
  return result;
}

std::string extractXmlTag(const std::string &xml, const std::string &tag) {
  std::string openTag = "<" + tag + ">";
  std::string closeTag = "</" + tag + ">";
  size_t start = xml.find(openTag);
  if (start == std::string::npos)
    return "";
  start += openTag.length();
  size_t end = xml.find(closeTag, start);
  if (end == std::string::npos)
    return "";
  return unescapeXml(trim(xml.substr(start, end - start)));
}

} // namespace

PlaylistFormat PlaylistService::detectFormat(const std::string &filePath) {
  std::string ext = toLower(fs::path(filePath).extension().string());
  if (ext == ".m3u")
    return PlaylistFormat::M3U;
  if (ext == ".m3u8")
    return PlaylistFormat::M3U8;
  if (ext == ".pls")
    return PlaylistFormat::PLS;
  if (ext == ".xspf")
    return PlaylistFormat::XSPF;
  return PlaylistFormat::Unknown;
}

std::string PlaylistService::makeRelativePath(const std::string &targetFilePath,
                                              const std::string &baseDir) {
  if (baseDir.empty() || targetFilePath.empty()) {
    return targetFilePath;
  }
  try {
    fs::path target(targetFilePath);
    fs::path base(baseDir);
    std::error_code ec;
    fs::path rel = fs::relative(target, base, ec);
    if (!ec && !rel.empty()) {
      return rel.generic_string();
    }
  } catch (...) {
  }
  return targetFilePath;
}

std::string PlaylistService::resolvePath(const std::string &path,
                                         const std::string &baseDir) {
  if (path.empty())
    return "";
  std::string clean = path;
  if (clean.rfind("file://", 0) == 0) {
    clean = clean.substr(7);
#if defined(_WIN32)
    if (!clean.empty() && clean[0] == '/') {
      clean = clean.substr(1);
    }
#endif
  }
  try {
    fs::path p(clean);
    if (p.is_absolute()) {
      return p.lexically_normal().string();
    }
    if (!baseDir.empty()) {
      fs::path base(baseDir);
      return (base / p).lexically_normal().string();
    }
  } catch (...) {
  }
  return clean;
}

bool PlaylistService::moveItem(StaticPlaylist &playlist, size_t fromIndex,
                               size_t toIndex) {
  if (fromIndex >= playlist.items.size() || toIndex >= playlist.items.size() ||
      fromIndex == toIndex) {
    return false;
  }
  auto item = playlist.items[fromIndex];
  playlist.items.erase(playlist.items.begin() + fromIndex);
  playlist.items.insert(playlist.items.begin() + toIndex, item);
  return true;
}

std::optional<StaticPlaylist>
PlaylistService::importPlaylist(const std::string &filePath) {
  fs::path p(filePath);
  if (!fs::exists(p)) {
    return std::nullopt;
  }
  std::ifstream file(p, std::ios::binary);
  if (!file.is_open()) {
    return std::nullopt;
  }

  std::string baseDir = p.parent_path().string();
  PlaylistFormat fmt = detectFormat(filePath);
  StaticPlaylist pl;
  pl.name = p.stem().string();

  bool ok = false;
  if (fmt == PlaylistFormat::M3U || fmt == PlaylistFormat::M3U8) {
    ok = parseM3U(file, baseDir, pl);
  } else if (fmt == PlaylistFormat::PLS) {
    ok = parsePLS(file, baseDir, pl);
  } else if (fmt == PlaylistFormat::XSPF) {
    ok = parseXSPF(file, baseDir, pl);
  } else {
    // Try M3U parser by default
    ok = parseM3U(file, baseDir, pl);
    if (!ok || pl.items.empty()) {
      file.clear();
      file.seekg(0);
      ok = parsePLS(file, baseDir, pl);
    }
  }

  if (ok) {
    return pl;
  }
  return std::nullopt;
}

bool PlaylistService::exportPlaylist(const StaticPlaylist &playlist,
                                     const std::string &targetFilePath,
                                     PlaylistFormat format,
                                     bool useRelativePaths) {
  fs::path outPath(targetFilePath);
  if (outPath.has_parent_path()) {
    std::error_code ec;
    fs::create_directories(outPath.parent_path(), ec);
  }

  std::ofstream out(outPath, std::ios::binary | std::ios::trunc);
  if (!out.is_open()) {
    return false;
  }

  std::string baseDir = outPath.parent_path().string();
  if (format == PlaylistFormat::M3U || format == PlaylistFormat::M3U8) {
    return writeM3U(out, playlist, baseDir, useRelativePaths);
  } else if (format == PlaylistFormat::PLS) {
    return writePLS(out, playlist, baseDir, useRelativePaths);
  } else if (format == PlaylistFormat::XSPF) {
    return writeXSPF(out, playlist, baseDir, useRelativePaths);
  }

  return writeM3U(out, playlist, baseDir, useRelativePaths);
}

bool PlaylistService::parseM3U(std::istream &in, const std::string &baseDir,
                               StaticPlaylist &outPlaylist) {
  std::string line;
  std::string pendingTitle;
  std::string pendingArtist;
  uint64_t pendingDurationMs = 0;

  while (std::getline(in, line)) {
    line = trim(line);
    if (line.empty())
      continue;

    if (line.rfind("#PLAYLIST:", 0) == 0) {
      outPlaylist.name = trim(line.substr(10));
      continue;
    }

    if (line.rfind("#EXTINF:", 0) == 0) {
      std::string inf = line.substr(8);
      size_t comma = inf.find(',');
      if (comma != std::string::npos) {
        std::string durStr = trim(inf.substr(0, comma));
        try {
          int sec = std::stoi(durStr);
          if (sec > 0) {
            pendingDurationMs = static_cast<uint64_t>(sec) * 1000;
          }
        } catch (...) {
        }
        std::string infoStr = trim(inf.substr(comma + 1));
        size_t dash = infoStr.find(" - ");
        if (dash != std::string::npos) {
          pendingArtist = trim(infoStr.substr(0, dash));
          pendingTitle = trim(infoStr.substr(dash + 3));
        } else {
          pendingTitle = infoStr;
        }
      }
      continue;
    }

    if (line[0] == '#') {
      continue;
    }

    PlaylistItem item;
    item.filePath = resolvePath(line, baseDir);
    item.title = pendingTitle.empty() ? fs::path(item.filePath).stem().string()
                                      : pendingTitle;
    item.artist = pendingArtist;
    item.durationMs = pendingDurationMs;

    outPlaylist.items.push_back(std::move(item));
    pendingTitle.clear();
    pendingArtist.clear();
    pendingDurationMs = 0;
  }

  return true;
}

bool PlaylistService::writeM3U(std::ostream &out,
                               const StaticPlaylist &playlist,
                               const std::string &baseDir, bool useRelative) {
  out << "#EXTM3U\n";
  if (!playlist.name.empty()) {
    out << "#PLAYLIST:" << playlist.name << "\n";
  }

  for (const auto &item : playlist.items) {
    uint64_t sec = item.durationMs / 1000;
    out << "#EXTINF:" << sec << ",";
    if (!item.artist.empty()) {
      out << item.artist << " - ";
    }
    out << (item.title.empty() ? fs::path(item.filePath).stem().string()
                               : item.title)
        << "\n";

    std::string pathOut = item.filePath;
    if (useRelative) {
      pathOut = makeRelativePath(item.filePath, baseDir);
    }
    out << pathOut << "\n";
  }

  return true;
}

bool PlaylistService::parsePLS(std::istream &in, const std::string &baseDir,
                               StaticPlaylist &outPlaylist) {
  std::string line;
  std::vector<std::string> files;
  std::vector<std::string> titles;
  std::vector<int> lengths;

  while (std::getline(in, line)) {
    line = trim(line);
    if (line.empty() || line[0] == ';' || line[0] == '#')
      continue;

    if (line == "[playlist]")
      continue;

    size_t eq = line.find('=');
    if (eq == std::string::npos)
      continue;

    std::string key = toLower(trim(line.substr(0, eq)));
    std::string val = trim(line.substr(eq + 1));

    if (key.rfind("file", 0) == 0) {
      try {
        int idx = std::stoi(key.substr(4));
        if (idx > 0) {
          if (static_cast<size_t>(idx) > files.size()) {
            files.resize(idx);
          }
          files[idx - 1] = val;
        }
      } catch (...) {
      }
    } else if (key.rfind("title", 0) == 0) {
      try {
        int idx = std::stoi(key.substr(5));
        if (idx > 0) {
          if (static_cast<size_t>(idx) > titles.size()) {
            titles.resize(idx);
          }
          titles[idx - 1] = val;
        }
      } catch (...) {
      }
    } else if (key.rfind("length", 0) == 0) {
      try {
        int idx = std::stoi(key.substr(6));
        if (idx > 0) {
          if (static_cast<size_t>(idx) > lengths.size()) {
            lengths.resize(idx);
          }
          lengths[idx - 1] = std::stoi(val);
        }
      } catch (...) {
      }
    }
  }

  for (size_t i = 0; i < files.size(); ++i) {
    if (files[i].empty())
      continue;
    PlaylistItem item;
    item.filePath = resolvePath(files[i], baseDir);
    if (i < titles.size() && !titles[i].empty()) {
      std::string fullTitle = titles[i];
      size_t dash = fullTitle.find(" - ");
      if (dash != std::string::npos) {
        item.artist = trim(fullTitle.substr(0, dash));
        item.title = trim(fullTitle.substr(dash + 3));
      } else {
        item.title = fullTitle;
      }
    } else {
      item.title = fs::path(item.filePath).stem().string();
    }
    if (i < lengths.size() && lengths[i] > 0) {
      item.durationMs = static_cast<uint64_t>(lengths[i]) * 1000;
    }
    outPlaylist.items.push_back(std::move(item));
  }

  return true;
}

bool PlaylistService::writePLS(std::ostream &out,
                               const StaticPlaylist &playlist,
                               const std::string &baseDir, bool useRelative) {
  out << "[playlist]\n";
  out << "NumberOfEntries=" << playlist.items.size() << "\n\n";

  for (size_t i = 0; i < playlist.items.size(); ++i) {
    const auto &item = playlist.items[i];
    std::string pathOut = item.filePath;
    if (useRelative) {
      pathOut = makeRelativePath(item.filePath, baseDir);
    }
    out << "File" << (i + 1) << "=" << pathOut << "\n";
    std::string titleStr;
    if (!item.artist.empty()) {
      titleStr += item.artist + " - ";
    }
    titleStr += item.title.empty() ? fs::path(item.filePath).stem().string()
                                   : item.title;
    out << "Title" << (i + 1) << "=" << titleStr << "\n";
    out << "Length" << (i + 1) << "=" << (item.durationMs / 1000) << "\n\n";
  }

  out << "Version=2\n";
  return true;
}

bool PlaylistService::parseXSPF(std::istream &in, const std::string &baseDir,
                                StaticPlaylist &outPlaylist) {
  std::stringstream ss;
  ss << in.rdbuf();
  std::string xml = ss.str();

  std::string plTitle = extractXmlTag(xml, "title");
  if (!plTitle.empty()) {
    outPlaylist.name = plTitle;
  }

  size_t cur = 0;
  while ((cur = xml.find("<track>", cur)) != std::string::npos) {
    size_t endTrack = xml.find("</track>", cur);
    if (endTrack == std::string::npos)
      break;

    std::string trackBlock = xml.substr(cur, (endTrack - cur) + 8);
    std::string location = extractXmlTag(trackBlock, "location");
    std::string title = extractXmlTag(trackBlock, "title");
    std::string creator = extractXmlTag(trackBlock, "creator");
    std::string album = extractXmlTag(trackBlock, "album");
    std::string durationStr = extractXmlTag(trackBlock, "duration");

    if (!location.empty()) {
      PlaylistItem item;
      item.filePath = resolvePath(location, baseDir);
      item.title =
          title.empty() ? fs::path(item.filePath).stem().string() : title;
      item.artist = creator;
      item.album = album;
      try {
        if (!durationStr.empty()) {
          item.durationMs = std::stoull(durationStr);
        }
      } catch (...) {
      }
      outPlaylist.items.push_back(std::move(item));
    }

    cur = endTrack + 8;
  }

  return true;
}

bool PlaylistService::writeXSPF(std::ostream &out,
                                const StaticPlaylist &playlist,
                                const std::string &baseDir, bool useRelative) {
  out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  out << "<playlist version=\"1\" xmlns=\"http://xspf.org/ns/0/\">\n";
  out << "  <title>" << escapeXml(playlist.name) << "</title>\n";
  out << "  <trackList>\n";

  for (const auto &item : playlist.items) {
    out << "    <track>\n";
    std::string pathOut = item.filePath;
    if (useRelative) {
      pathOut = makeRelativePath(item.filePath, baseDir);
    }
    out << "      <location>" << escapeXml(pathOut) << "</location>\n";
    if (!item.title.empty()) {
      out << "      <title>" << escapeXml(item.title) << "</title>\n";
    }
    if (!item.artist.empty()) {
      out << "      <creator>" << escapeXml(item.artist) << "</creator>\n";
    }
    if (!item.album.empty()) {
      out << "      <album>" << escapeXml(item.album) << "</album>\n";
    }
    if (item.durationMs > 0) {
      out << "      <duration>" << item.durationMs << "</duration>\n";
    }
    out << "    </track>\n";
  }

  out << "  </trackList>\n";
  out << "</playlist>\n";
  return true;
}

} // namespace core
