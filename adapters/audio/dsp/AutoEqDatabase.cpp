#include "AutoEqDatabase.hpp"
#include "EqFileParser.hpp"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QUrl>
#include <algorithm>

static void initAutoEqResources() { Q_INIT_RESOURCE(autoeq); }

namespace adapters {

AutoEqDatabase &AutoEqDatabase::instance() {
  static AutoEqDatabase s_instance;
  return s_instance;
}

AutoEqDatabase::AutoEqDatabase(QObject *parent)
    : QObject(parent), m_networkManager(new QNetworkAccessManager(this)) {}

QString AutoEqDatabase::attributionText() {
  return QStringLiteral(
      "PicAudio PEQ integration powered by AutoEq (Jaakko Pasanen, MIT "
      "License).\n"
      "Measurements by Oratory1990, Crinacle, Rtings, Super* Review.\n"
      "Harman Target Curves (Harman International). Filter formulas: RBJ Audio "
      "EQ Cookbook.");
}

void AutoEqDatabase::initialize() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_initialized) {
    return;
  }

  initAutoEqResources();
  loadBundledProfiles();
  loadCachedProfiles();
  m_initialized = true;
}

void AutoEqDatabase::loadBundledProfiles() {
  QFile file(QStringLiteral(":/autoeq/autoeq_base.json"));
  if (!file.open(QIODevice::ReadOnly)) {
    // Try development/testing path fallbacks
    file.setFileName(QStringLiteral("assets/autoeq/autoeq_base.json"));
    if (!file.open(QIODevice::ReadOnly)) {
      file.setFileName(QStringLiteral("../assets/autoeq/autoeq_base.json"));
      if (!file.open(QIODevice::ReadOnly)) {
        return;
      }
    }
  }

  QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
  if (!doc.isObject()) {
    return;
  }

  QJsonArray profiles =
      doc.object().value(QStringLiteral("profiles")).toArray();
  for (const auto &item : profiles) {
    if (!item.isObject()) {
      continue;
    }
    QJsonObject obj = item.toObject();
    AutoEqProfile profile;
    profile.id = obj.value(QStringLiteral("id")).toString();
    profile.model = obj.value(QStringLiteral("model")).toString();
    profile.source = obj.value(QStringLiteral("source")).toString();
    profile.target = obj.value(QStringLiteral("target")).toString();
    profile.formFactor = obj.value(QStringLiteral("formFactor")).toString();
    profile.preampDb =
        static_cast<float>(obj.value(QStringLiteral("preamp")).toDouble(0.0));

    QJsonArray filters = obj.value(QStringLiteral("filters")).toArray();
    for (const auto &fItem : filters) {
      if (!fItem.isObject()) {
        continue;
      }
      QJsonObject fObj = fItem.toObject();
      core::PeqFilterBand band;
      band.type = static_cast<core::EqFilterType>(
          fObj.value(QStringLiteral("type")).toInt(0));
      band.frequency = static_cast<float>(
          fObj.value(QStringLiteral("freq")).toDouble(1000.0));
      band.gainDb =
          static_cast<float>(fObj.value(QStringLiteral("gain")).toDouble(0.0));
      band.q =
          static_cast<float>(fObj.value(QStringLiteral("q")).toDouble(1.414));
      band.enabled = fObj.value(QStringLiteral("enabled")).toBool(true);
      profile.filters.push_back(band);
    }

    if (!profile.model.isEmpty()) {
      m_profiles.push_back(profile);
    }
  }
}

void AutoEqDatabase::loadCachedProfiles() {
  QString cacheDir =
      QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) +
      QStringLiteral("/autoeq_cache");
  QDir dir(cacheDir);
  if (!dir.exists()) {
    return;
  }

  QStringList files = dir.entryList(QStringList() << "*.json", QDir::Files);
  for (const auto &fName : files) {
    QFile file(dir.filePath(fName));
    if (!file.open(QIODevice::ReadOnly)) {
      continue;
    }
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
      continue;
    }

    QJsonObject obj = doc.object();
    AutoEqProfile profile;
    profile.id = obj.value(QStringLiteral("id")).toString();
    profile.model = obj.value(QStringLiteral("model")).toString();
    profile.source = obj.value(QStringLiteral("source")).toString();
    profile.target = obj.value(QStringLiteral("target")).toString();
    profile.formFactor = obj.value(QStringLiteral("formFactor")).toString();
    profile.preampDb =
        static_cast<float>(obj.value(QStringLiteral("preamp")).toDouble(0.0));

    QJsonArray filters = obj.value(QStringLiteral("filters")).toArray();
    for (const auto &fItem : filters) {
      if (!fItem.isObject()) {
        continue;
      }
      QJsonObject fObj = fItem.toObject();
      core::PeqFilterBand band;
      band.type = static_cast<core::EqFilterType>(
          fObj.value(QStringLiteral("type")).toInt(0));
      band.frequency = static_cast<float>(
          fObj.value(QStringLiteral("freq")).toDouble(1000.0));
      band.gainDb =
          static_cast<float>(fObj.value(QStringLiteral("gain")).toDouble(0.0));
      band.q =
          static_cast<float>(fObj.value(QStringLiteral("q")).toDouble(1.414));
      band.enabled = fObj.value(QStringLiteral("enabled")).toBool(true);
      profile.filters.push_back(band);
    }

    if (!profile.id.isEmpty()) {
      auto it = std::find_if(
          m_profiles.begin(), m_profiles.end(),
          [&profile](const AutoEqProfile &p) { return p.id == profile.id; });
      if (it != m_profiles.end()) {
        *it = profile;
      } else {
        m_profiles.push_back(profile);
      }
    }
  }
}

void AutoEqDatabase::saveProfileToDisk(const AutoEqProfile &profile) {
  QString cacheDir =
      QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) +
      QStringLiteral("/autoeq_cache");
  QDir().mkpath(cacheDir);

  QString filePath =
      cacheDir + QStringLiteral("/") + profile.id + QStringLiteral(".json");
  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly)) {
    return;
  }

  QJsonObject obj;
  obj[QStringLiteral("id")] = profile.id;
  obj[QStringLiteral("model")] = profile.model;
  obj[QStringLiteral("source")] = profile.source;
  obj[QStringLiteral("target")] = profile.target;
  obj[QStringLiteral("formFactor")] = profile.formFactor;
  obj[QStringLiteral("preamp")] = static_cast<double>(profile.preampDb);

  QJsonArray filters;
  for (const auto &b : profile.filters) {
    QJsonObject fObj;
    fObj[QStringLiteral("type")] = static_cast<int>(b.type);
    fObj[QStringLiteral("freq")] = static_cast<double>(b.frequency);
    fObj[QStringLiteral("gain")] = static_cast<double>(b.gainDb);
    fObj[QStringLiteral("q")] = static_cast<double>(b.q);
    fObj[QStringLiteral("enabled")] = b.enabled;
    filters.append(fObj);
  }
  obj[QStringLiteral("filters")] = filters;

  file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
}

size_t AutoEqDatabase::profileCount() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_profiles.size();
}

std::vector<AutoEqProfile> AutoEqDatabase::search(const QString &query,
                                                  int maxResults) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  QString q = query.trimmed().toLower();

  if (q.isEmpty()) {
    std::vector<AutoEqProfile> topResults;
    size_t count = std::min(m_profiles.size(), static_cast<size_t>(maxResults));
    topResults.assign(m_profiles.begin(), m_profiles.begin() + count);
    return topResults;
  }

  auto simplify = [](const QString &s) {
    QString out;
    out.reserve(s.size());
    for (QChar ch : s.toLower()) {
      if (ch.isLetterOrNumber()) {
        out.append(ch);
      }
    }
    return out;
  };

  QString qSimplified = simplify(q);
  QStringList queryTokens = q.split(QStringLiteral(" "), Qt::SkipEmptyParts);
  std::vector<std::pair<int, AutoEqProfile>> scored;

  for (const auto &p : m_profiles) {
    QString modelLower = p.model.toLower();
    QString combined = (p.model + " " + p.source + " " + p.target).toLower();
    QString simplifiedModel = simplify(p.model);
    QString simplifiedCombined = simplify(combined);

    bool matches = false;
    int score = 0;

    // Check direct simplified match (e.g. "hd600" matches "Sennheiser HD 600")
    if (!qSimplified.isEmpty()) {
      if (simplifiedModel == qSimplified) {
        matches = true;
        score += 100;
      } else if (simplifiedModel.contains(qSimplified)) {
        matches = true;
        score += 50;
        if (simplifiedModel.startsWith(qSimplified)) {
          score += 20;
        }
      } else if (simplifiedCombined.contains(qSimplified)) {
        matches = true;
        score += 30;
      }
    }

    // Check token-based matching
    bool allTokensMatch = !queryTokens.isEmpty();
    for (const auto &tok : queryTokens) {
      QString tokSimplified = simplify(tok);
      bool tokMatches = combined.contains(tok) ||
                        (!tokSimplified.isEmpty() &&
                         simplifiedCombined.contains(tokSimplified));
      if (!tokMatches) {
        allTokensMatch = false;
        break;
      }
      if (modelLower.contains(tok) ||
          (!tokSimplified.isEmpty() &&
           simplifiedModel.contains(tokSimplified))) {
        score += 10;
        if (modelLower.startsWith(tok)) {
          score += 15;
        }
      }
    }

    if (matches || allTokensMatch) {
      scored.push_back({score, p});
    }
  }

  std::sort(scored.begin(), scored.end(),
            [](const auto &a, const auto &b) { return a.first > b.first; });

  std::vector<AutoEqProfile> results;
  size_t count = std::min(scored.size(), static_cast<size_t>(maxResults));
  results.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    results.push_back(scored[i].second);
  }

  return results;
}

std::optional<AutoEqProfile>
AutoEqDatabase::getProfile(const QString &id) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  for (const auto &p : m_profiles) {
    if (p.id == id) {
      return p;
    }
  }
  return std::nullopt;
}

void AutoEqDatabase::fetchOnlineProfile(
    const QString &url, const QString &modelName, const QString &source,
    std::function<void(bool success, const AutoEqProfile &profile)> callback) {
  QNetworkRequest request;
  request.setUrl(QUrl(url));
  request.setHeader(QNetworkRequest::UserAgentHeader,
                    QStringLiteral("PicAudio/1.0 (AutoEq Client)"));
  request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                       QNetworkRequest::NoLessSafeRedirectPolicy);

  QNetworkReply *reply = m_networkManager->get(request);
  connect(reply, &QNetworkReply::finished, this,
          [this, reply, modelName, source, callback]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
              callback(false, AutoEqProfile{});
              return;
            }

            QString content = QString::fromUtf8(reply->readAll());
            ParsedEqData parsed = EqFileParser::parse(content.toStdString());
            if (!parsed.success || parsed.peqBands.empty()) {
              callback(false, AutoEqProfile{});
              return;
            }

            AutoEqProfile profile;
            profile.model = modelName;
            profile.source =
                source.isEmpty() ? QStringLiteral("AutoEq Online") : source;
            profile.target = QStringLiteral("Harman Target");
            profile.formFactor = QStringLiteral("Headphone / IEM");
            profile.id = QStringLiteral("autoeq_online_") +
                         QString::number(qHash(modelName + source));
            profile.preampDb = parsed.preampDb;
            profile.filters = parsed.peqBands;

            {
              std::lock_guard<std::mutex> lock(m_mutex);
              auto it = std::find_if(m_profiles.begin(), m_profiles.end(),
                                     [&profile](const AutoEqProfile &p) {
                                       return p.id == profile.id;
                                     });
              if (it != m_profiles.end()) {
                *it = profile;
              } else {
                m_profiles.push_back(profile);
              }
              saveProfileToDisk(profile);
            }

            callback(true, profile);
          });
}

} // namespace adapters
