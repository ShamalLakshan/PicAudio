#include "EqFileParser.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <regex>
#include <sstream>

namespace adapters {

namespace {

std::string trim(const std::string &str) {
  size_t first = str.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return "";
  }
  size_t last = str.find_last_not_of(" \t\r\n");
  return str.substr(first, (last - first + 1));
}

std::string toUpper(std::string str) {
  std::transform(str.begin(), str.end(), str.begin(),
                 [](unsigned char c) { return std::toupper(c); });
  return str;
}

static const std::array<float, 10> kIsoFreqs10 = {
    31.0f,   63.0f,   125.0f,  250.0f,  500.0f,
    1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f};

static const std::array<float, 15> kIsoFreqs15 = {
    25.0f,   40.0f,   63.0f,   100.0f,  160.0f,  250.0f,   400.0f,  630.0f,
    1000.0f, 1600.0f, 2500.0f, 4000.0f, 6300.0f, 10000.0f, 16000.0f};

static const std::array<float, 31> kIsoFreqs31 = {
    20.0f,   25.0f,   31.5f,   40.0f,    50.0f,    63.0f,    80.0f,   100.0f,
    125.0f,  160.0f,  200.0f,  250.0f,   315.0f,   400.0f,   500.0f,  630.0f,
    800.0f,  1000.0f, 1250.0f, 1600.0f,  2000.0f,  2500.0f,  3150.0f, 4000.0f,
    5000.0f, 6300.0f, 8000.0f, 10000.0f, 12500.0f, 16000.0f, 20000.0f};

core::EqFilterType parseFilterType(const std::string &typeStr) {
  std::string upper = toUpper(trim(typeStr));
  if (upper == "PK" || upper == "PEAK" || upper == "PEAKING") {
    return core::EqFilterType::Peaking;
  }
  if (upper == "LS" || upper == "LSC" || upper == "LOW_SHELF" ||
      upper == "LOWSHELF") {
    return core::EqFilterType::LowShelf;
  }
  if (upper == "HS" || upper == "HSC" || upper == "HIGH_SHELF" ||
      upper == "HIGHSHELF") {
    return core::EqFilterType::HighShelf;
  }
  if (upper == "LP" || upper == "LPF" || upper == "LOW_PASS" ||
      upper == "LOWPASS") {
    return core::EqFilterType::LowPass;
  }
  if (upper == "HP" || upper == "HPF" || upper == "HIGH_PASS" ||
      upper == "HIGHPASS") {
    return core::EqFilterType::HighPass;
  }
  if (upper == "BP" || upper == "BPF" || upper == "BAND_PASS" ||
      upper == "BANDPASS") {
    return core::EqFilterType::BandPass;
  }
  if (upper == "NO" || upper == "NOTCH") {
    return core::EqFilterType::Notch;
  }
  return core::EqFilterType::Peaking;
}

std::string filterTypeToString(core::EqFilterType type) {
  switch (type) {
  case core::EqFilterType::Peaking:
    return "PK";
  case core::EqFilterType::LowShelf:
    return "LSC";
  case core::EqFilterType::HighShelf:
    return "HSC";
  case core::EqFilterType::LowPass:
    return "LPF";
  case core::EqFilterType::HighPass:
    return "HPF";
  case core::EqFilterType::BandPass:
    return "BP";
  case core::EqFilterType::Notch:
    return "NOTCH";
  }
  return "PK";
}

} // namespace

std::vector<float> EqFileParser::interpolateGraphicEq(
    const std::vector<std::pair<float, float>> &curvePoints,
    size_t targetCount) {
  if (curvePoints.empty()) {
    return std::vector<float>(targetCount, 0.0f);
  }

  const float *targets = nullptr;
  size_t count = 10;
  if (targetCount == 31) {
    targets = kIsoFreqs31.data();
    count = 31;
  } else if (targetCount == 15) {
    targets = kIsoFreqs15.data();
    count = 15;
  } else {
    targets = kIsoFreqs10.data();
    count = 10;
  }

  std::vector<float> gains(count, 0.0f);

  for (size_t i = 0; i < count; ++i) {
    float f = targets[i];
    if (f <= curvePoints.front().first) {
      gains[i] = curvePoints.front().second;
    } else if (f >= curvePoints.back().first) {
      gains[i] = curvePoints.back().second;
    } else {
      for (size_t j = 0; j < curvePoints.size() - 1; ++j) {
        float f0 = curvePoints[j].first;
        float f1 = curvePoints[j + 1].first;
        if (f >= f0 && f <= f1) {
          float logF = std::log2(f);
          float log0 = std::log2(f0);
          float log1 = std::log2(f1);
          float t = (log1 > log0) ? (logF - log0) / (log1 - log0) : 0.0f;
          gains[i] = curvePoints[j].second * (1.0f - t) +
                     curvePoints[j + 1].second * t;
          break;
        }
      }
    }
  }

  return gains;
}

ParsedEqData EqFileParser::parse(const std::string &content) {
  ParsedEqData result;
  if (content.empty()) {
    result.errorMessage = "Empty configuration content";
    return result;
  }

  // Check if content contains GraphicEQ
  std::string upperContent = toUpper(content);
  size_t geqPos = upperContent.find("GRAPHICEQ:");

  if (geqPos != std::string::npos) {
    // Wavelet / Squiglink GraphicEQ format
    result.mode = 0; // Graphic mode
    std::string dataPart = content.substr(geqPos + 10);
    // Remove newlines and process pairs "freq gain;"
    std::vector<std::pair<float, float>> points;
    std::istringstream stream(dataPart);
    std::string token;

    while (std::getline(stream, token, ';')) {
      std::istringstream pairStream(token);
      float freq = 0.0f, gain = 0.0f;
      if (pairStream >> freq >> gain) {
        if (freq > 0.0f) {
          points.push_back({freq, gain});
        }
      }
    }

    if (!points.empty()) {
      std::sort(points.begin(), points.end(),
                [](const auto &a, const auto &b) { return a.first < b.first; });

      float maxBoost = 0.0f;
      for (const auto &p : points) {
        if (p.second > maxBoost) {
          maxBoost = p.second;
        }
      }
      if (maxBoost > 0.0f) {
        result.preampDb = -std::round((maxBoost + 0.2f) * 10.0f) / 10.0f;
      }

      result.graphicGains = interpolateGraphicEq(points, 31);
      result.graphicBandCount = 31;

      // Also generate equivalent PEQ peaking bands
      for (size_t i = 0; i < kIsoFreqs10.size(); ++i) {
        core::PeqFilterBand band;
        band.type = core::EqFilterType::Peaking;
        band.frequency = kIsoFreqs10[i];
        band.q = 1.414f;
        band.gainDb = 0.0f;
        band.enabled = true;
        result.peqBands.push_back(band);
      }
      auto peq10Gains = interpolateGraphicEq(points, 10);
      for (size_t i = 0; i < peq10Gains.size() && i < result.peqBands.size();
           ++i) {
        result.peqBands[i].gainDb = peq10Gains[i];
      }

      result.success = true;
      return result;
    }
  }

  // Parse Equalizer APO / Peace GUI Parametric EQ lines
  // Example lines:
  // Preamp: -4.5 dB
  // Filter 1: ON PK Fc 105 Hz Gain 3.0 dB Q 0.70
  // Filter: ON LSC Fc 80 Hz Gain 4.0 dB Q 0.71
  std::regex preampRegex(
      R"((?:preamp|gain)\s*:\s*([+-]?[0-9]+(?:\.[0-9]+)?)\s*(?:db)?)",
      std::regex_constants::icase);

  std::regex filterRegex(
      R"((?:filter\s*[0-9]*\s*:\s*)?(ON|OFF)?\s*(PK|PEAK|PEAKING|LS|LSC|LOW_SHELF|LOWSHELF|HS|HSC|HIGH_SHELF|HIGHSHELF|LP|LPF|LOW_PASS|LOWPASS|HP|HPF|HIGH_PASS|HIGHPASS|BP|BPF|BAND_PASS|BANDPASS|NO|NOTCH)\s*(?:fc)?\s*([0-9]+(?:\.[0-9]+)?)\s*(?:hz)?\s*(?:gain)?\s*([+-]?[0-9]+(?:\.[0-9]+)?)\s*(?:db)?\s*(?:q)?\s*([0-9]+(?:\.[0-9]+)?))",
      std::regex_constants::icase);

  std::istringstream ss(content);
  std::string line;
  int parsedFilters = 0;

  while (std::getline(ss, line)) {
    line = trim(line);
    if (line.empty() || line[0] == '#') {
      continue;
    }

    // Check Preamp
    std::smatch match;
    if (std::regex_search(line, match, preampRegex)) {
      try {
        result.preampDb = std::stof(match[1].str());
      } catch (...) {
      }
      continue;
    }

    // Check Filter
    if (std::regex_search(line, match, filterRegex)) {
      std::string onOff = toUpper(match[1].str());
      std::string typeStr = match[2].str();
      std::string freqStr = match[3].str();
      std::string gainStr = match[4].str();
      std::string qStr = match[5].str();

      core::PeqFilterBand band;
      band.enabled = (onOff != "OFF");
      band.type = parseFilterType(typeStr);
      try {
        band.frequency = std::stof(freqStr);
        band.gainDb = std::stof(gainStr);
        band.q = qStr.empty() ? 1.414f : std::stof(qStr);
      } catch (...) {
        continue;
      }

      result.peqBands.push_back(band);
      parsedFilters++;
    }
  }

  if (parsedFilters > 0) {
    result.success = true;
    result.mode = 1; // Parametric mode
    return result;
  }

  result.errorMessage = "No valid Equalizer APO or GraphicEQ filters found";
  return result;
}

ParsedEqData EqFileParser::parseFile(const std::string &filePath) {
  ParsedEqData result;
  std::filesystem::path path(filePath);
  if (!std::filesystem::exists(path) ||
      !std::filesystem::is_regular_file(path)) {
    result.errorMessage = "File not found or inaccessible";
    return result;
  }

  std::ifstream file(path);
  if (!file.is_open()) {
    result.errorMessage = "Unable to open file for reading";
    return result;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  return parse(buffer.str());
}

std::string
EqFileParser::exportEqualizerApo(const std::vector<core::PeqFilterBand> &bands,
                                 float preampDb, const std::string &title) {
  std::ostringstream out;
  out << "# PicAudio Equalizer Configuration\n";
  if (!title.empty()) {
    out << "# Profile: " << title << "\n";
  }
  out << "# Equalizer APO & Peace GUI format compatible\n";
  out << std::fixed << std::setprecision(1);
  out << "Preamp: " << preampDb << " dB\n";

  for (size_t i = 0; i < bands.size(); ++i) {
    const auto &b = bands[i];
    out << "Filter " << (i + 1) << ": " << (b.enabled ? "ON" : "OFF") << " "
        << filterTypeToString(b.type) << " Fc " << std::setprecision(0)
        << b.frequency << " Hz Gain " << std::setprecision(1) << b.gainDb
        << " dB Q " << std::setprecision(2) << b.q << "\n";
  }

  return out.str();
}

} // namespace adapters
