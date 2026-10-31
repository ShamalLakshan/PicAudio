#include "ThemeLoader.hpp"
#include <QColor>
#include <QCoreApplication>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QStandardPaths>
#include <QUrl>
#include <algorithm>

namespace adapters {

namespace {
bool isValidColorName(const QString &str) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
  return QColor::isValidColorName(str);
#else
  return QColor::isValidColor(str);
#endif
}
} // namespace

ThemeLoader::ThemeLoader(QObject *parent)
    : QObject(parent), m_tokens(getDefaultDarkStudioTheme()) {

  // Ensure user themes directory exists
  QDir().mkpath(getUserThemesDir());

  setupWatcher();
  refreshAvailableThemes();

  // Load Dark Studio as initial default
  setThemeId("dark-studio");
}

QString ThemeLoader::getUserThemesDir() const {
  if (qEnvironmentVariableIsSet("PICAUDIO_DATA_DIR")) {
    return qEnvironmentVariable("PICAUDIO_DATA_DIR") + "/themes";
  }
  return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
         "/themes";
}

QStringList ThemeLoader::getThemeSearchPaths() const {
  QStringList paths;

  // 1. User themes directory (highest priority)
  paths << getUserThemesDir();

  // 2. Application-relative themes directories
  QString appDir = QCoreApplication::applicationDirPath();
  if (!appDir.isEmpty()) {
    paths << appDir + "/themes";
    paths << appDir + "/../share/picaudio/themes";
    paths << appDir +
                 "/../../themes"; // dev build location relative to build/app
  }

  // 3. Current working directory themes (development)
  paths << QDir::currentPath() + "/themes";

  // 4. Standard XDG / system AppData locations
  const auto stdLocations =
      QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
  for (const auto &loc : stdLocations) {
    QString p = loc + "/themes";
    if (!paths.contains(p)) {
      paths << p;
    }
  }

  // 5. Qt embedded resources (fail-safe fallback)
  paths << ":/themes";

  // Remove duplicates while preserving priority order
  QStringList uniquePaths;
  for (const auto &p : paths) {
    if (!uniquePaths.contains(p)) {
      uniquePaths << p;
    }
  }
  return uniquePaths;
}

QStringList ThemeLoader::themeSearchPaths() const {
  return getThemeSearchPaths();
}

void ThemeLoader::setupWatcher() {
  connect(&m_watcher, &QFileSystemWatcher::fileChanged, this,
          [this](const QString &path) {
            qDebug() << "[ThemeLoader] Hot-reloading theme file:" << path;
            if (path == m_activeThemeFilePath) {
              loadThemeFromFile(path);
            } else {
              refreshAvailableThemes();
            }
          });

  connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this,
          [this](const QString &path) {
            qDebug() << "[ThemeLoader] Themes directory changed:" << path;
            refreshAvailableThemes();
          });

  QString themesDir = getUserThemesDir();
  if (QDir(themesDir).exists()) {
    m_watcher.addPath(themesDir);
    m_watchedDirectories.append(themesDir);
  }
}

void ThemeLoader::watchActiveFile(const QString &filePath) {
  if (!m_activeThemeFilePath.isEmpty() &&
      m_watcher.files().contains(m_activeThemeFilePath)) {
    m_watcher.removePath(m_activeThemeFilePath);
  }
  m_activeThemeFilePath = filePath;
  if (!m_activeThemeFilePath.isEmpty() &&
      !m_activeThemeFilePath.startsWith(":") &&
      !m_activeThemeFilePath.startsWith("qrc:") &&
      QFile::exists(m_activeThemeFilePath)) {
    m_watcher.addPath(m_activeThemeFilePath);
  }
}

ThemeTokens ThemeLoader::getDefaultDarkStudioTheme() {
  ThemeTokens t;
  t.id = "dark-studio";
  t.name = "Dark Studio";
  t.author = "PicAudio";
  t.version = "1.0.0";
  t.apiVersion = "0.1-unstable";
  t.description = "Default dark theme";

  t.background = "#000000";
  t.surface = "#0d0d0d";
  t.surfaceElevated = "#171717";
  t.panelBorder = "#262626";
  t.accent = "#ffffff";
  t.accentHover = "#e5e5e5";
  t.textPrimary = "#ffffff";
  t.textSecondary = "#a3a3a3";
  t.textMuted = "#737373";
  t.selection = "#262626";
  t.error = "#ef4444";
  t.warning = "#f59e0b";
  t.success = "#22c55e";

  t.fontFamily = "sans-serif";
  t.fontSizeSmall = 9;
  t.fontSizeBase = 11;
  t.fontSizeLarge = 14;
  t.fontSizeTitle = 18;
  t.fontWeightScale = 1.0;
  t.lineHeightScale = 1.2;

  t.spacingSmall = 4;
  t.spacingMedium = 8;
  t.spacingLarge = 16;
  t.cornerRadiusSmall = 2;
  t.cornerRadiusMedium = 4;
  t.panelPadding = 10;
  return t;
}

ThemeTokens ThemeLoader::getBuiltInNordTheme() {
  ThemeTokens t;
  t.id = "nord-audiophile";
  t.name = "Nord";
  t.author = "PicAudio Community";
  t.version = "1.0.0";
  t.apiVersion = "0.1-unstable";
  t.description =
      "Arctic, north-bluish clean palette tailored for night listening";

  t.background = "#2e3440";
  t.surface = "#3b4252";
  t.surfaceElevated = "#434c5e";
  t.panelBorder = "#4c566a";
  t.accent = "#88c0d0";
  t.accentHover = "#8fbcbb";
  t.textPrimary = "#eceff4";
  t.textSecondary = "#d8dee9";
  t.textMuted = "#7e889b";
  t.selection = "#434c5e";
  t.error = "#bf616a";
  t.warning = "#ebcb8b";
  t.success = "#a3be8c";

  t.fontFamily = "sans-serif";
  t.fontSizeSmall = 9;
  t.fontSizeBase = 11;
  t.fontSizeLarge = 14;
  t.fontSizeTitle = 18;
  t.fontWeightScale = 1.0;
  t.lineHeightScale = 1.2;

  t.spacingSmall = 4;
  t.spacingMedium = 8;
  t.spacingLarge = 16;
  t.cornerRadiusSmall = 3;
  t.cornerRadiusMedium = 6;
  t.panelPadding = 12;
  return t;
}

ThemeTokens ThemeLoader::getBuiltInSolarizedDarkTheme() {
  ThemeTokens t;
  t.id = "solarized-dark";
  t.name = "Solarized Dark";
  t.author = "PicAudio Community";
  t.version = "1.0.0";
  t.apiVersion = "0.1-unstable";
  t.description = "Warm low-contrast palette calibrated for eye comfort";

  t.background = "#002b36";
  t.surface = "#073642";
  t.surfaceElevated = "#094452";
  t.panelBorder = "#586e75";
  t.accent = "#268bd2";
  t.accentHover = "#2aa198";
  t.textPrimary = "#fdf6e3";
  t.textSecondary = "#93a1a1";
  t.textMuted = "#657b83";
  t.selection = "#094452";
  t.error = "#dc322f";
  t.warning = "#b58900";
  t.success = "#859900";

  t.fontFamily = "sans-serif";
  t.fontSizeSmall = 9;
  t.fontSizeBase = 11;
  t.fontSizeLarge = 14;
  t.fontSizeTitle = 18;
  t.fontWeightScale = 1.0;
  t.lineHeightScale = 1.2;

  t.spacingSmall = 4;
  t.spacingMedium = 8;
  t.spacingLarge = 16;
  t.cornerRadiusSmall = 3;
  t.cornerRadiusMedium = 6;
  t.panelPadding = 10;
  return t;
}

bool ThemeLoader::validateThemeJson(const QJsonObject &root,
                                    QString &errorReason) {
  if (!root.contains("meta") || !root["meta"].isObject()) {
    errorReason = "Missing required 'meta' object in theme JSON";
    return false;
  }
  QJsonObject meta = root["meta"].toObject();
  for (const char *reqKey : {"name", "author", "version", "apiVersion"}) {
    if (!meta.contains(reqKey) || !meta[reqKey].isString()) {
      errorReason =
          QString("Missing required meta string field: '%1'").arg(reqKey);
      return false;
    }
  }

  if (!root.contains("colors") || !root["colors"].isObject()) {
    errorReason = "Missing required 'colors' object in theme JSON";
    return false;
  }
  QJsonObject colors = root["colors"].toObject();
  const char *reqColors[] = {"background",  "surface",       "surfaceElevated",
                             "panelBorder", "accent",        "accentHover",
                             "textPrimary", "textSecondary", "textMuted",
                             "selection",   "error",         "warning",
                             "success"};
  for (const char *cKey : reqColors) {
    if (!colors.contains(cKey) || !colors[cKey].isString()) {
      errorReason = QString("Missing required color field: '%1'").arg(cKey);
      return false;
    }
    QString colStr = colors[cKey].toString();
    if (!isValidColorName(colStr)) {
      errorReason =
          QString("Invalid color hex format for '%1': '%2'").arg(cKey, colStr);
      return false;
    }
  }

  return true;
}

ThemeTokens ThemeLoader::parseThemeJson(const QJsonObject &root,
                                        const ThemeTokens &fallback) {
  ThemeTokens t = fallback;

  if (root.contains("meta") && root["meta"].isObject()) {
    QJsonObject meta = root["meta"].toObject();
    if (meta.contains("id") && meta["id"].isString())
      t.id = meta["id"].toString();
    if (meta.contains("name") && meta["name"].isString())
      t.name = meta["name"].toString();
    if (meta.contains("author") && meta["author"].isString())
      t.author = meta["author"].toString();
    if (meta.contains("version") && meta["version"].isString())
      t.version = meta["version"].toString();
    if (meta.contains("apiVersion") && meta["apiVersion"].isString())
      t.apiVersion = meta["apiVersion"].toString();
    if (meta.contains("description") && meta["description"].isString())
      t.description = meta["description"].toString();
  }

  if (root.contains("colors") && root["colors"].isObject()) {
    QJsonObject c = root["colors"].toObject();
    auto readColor = [&](const char *key, QString &target) {
      if (c.contains(key) && c[key].isString()) {
        QString val = c[key].toString();
        if (isValidColorName(val))
          target = val;
      }
    };
    readColor("background", t.background);
    readColor("surface", t.surface);
    readColor("surfaceElevated", t.surfaceElevated);
    readColor("panelBorder", t.panelBorder);
    readColor("accent", t.accent);
    readColor("accentHover", t.accentHover);
    readColor("textPrimary", t.textPrimary);
    readColor("textSecondary", t.textSecondary);
    readColor("textMuted", t.textMuted);
    readColor("selection", t.selection);
    readColor("error", t.error);
    readColor("warning", t.warning);
    readColor("success", t.success);
  }

  if (root.contains("typography") && root["typography"].isObject()) {
    QJsonObject typo = root["typography"].toObject();
    if (typo.contains("fontFamily") && typo["fontFamily"].isString())
      t.fontFamily = typo["fontFamily"].toString();
    if (typo.contains("fontSizeSmall") && typo["fontSizeSmall"].isDouble())
      t.fontSizeSmall = typo["fontSizeSmall"].toInt();
    if (typo.contains("fontSizeBase") && typo["fontSizeBase"].isDouble())
      t.fontSizeBase = typo["fontSizeBase"].toInt();
    if (typo.contains("fontSizeLarge") && typo["fontSizeLarge"].isDouble())
      t.fontSizeLarge = typo["fontSizeLarge"].toInt();
    if (typo.contains("fontSizeTitle") && typo["fontSizeTitle"].isDouble())
      t.fontSizeTitle = typo["fontSizeTitle"].toInt();
    if (typo.contains("fontWeightScale") && typo["fontWeightScale"].isDouble())
      t.fontWeightScale = typo["fontWeightScale"].toDouble();
    if (typo.contains("lineHeightScale") && typo["lineHeightScale"].isDouble())
      t.lineHeightScale = typo["lineHeightScale"].toDouble();
  }

  if (root.contains("metrics") && root["metrics"].isObject()) {
    QJsonObject m = root["metrics"].toObject();
    if (m.contains("spacingSmall") && m["spacingSmall"].isDouble())
      t.spacingSmall = m["spacingSmall"].toInt();
    if (m.contains("spacingMedium") && m["spacingMedium"].isDouble())
      t.spacingMedium = m["spacingMedium"].toInt();
    if (m.contains("spacingLarge") && m["spacingLarge"].isDouble())
      t.spacingLarge = m["spacingLarge"].toInt();
    if (m.contains("cornerRadiusSmall") && m["cornerRadiusSmall"].isDouble())
      t.cornerRadiusSmall = m["cornerRadiusSmall"].toInt();
    if (m.contains("cornerRadiusMedium") && m["cornerRadiusMedium"].isDouble())
      t.cornerRadiusMedium = m["cornerRadiusMedium"].toInt();
    if (m.contains("panelPadding") && m["panelPadding"].isDouble())
      t.panelPadding = m["panelPadding"].toInt();
  }

  return t;
}

bool ThemeLoader::loadThemeFromFile(const QString &filePath) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QString err = QString("Cannot open theme file: %1").arg(filePath);
    qWarning() << "[ThemeLoader]" << err;
    emit themeLoadError(err);
    return false;
  }

  QJsonParseError parseError;
  QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
  if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
    QString err = QString("JSON syntax error in %1: %2")
                      .arg(filePath, parseError.errorString());
    qWarning() << "[ThemeLoader]" << err;
    emit themeLoadError(err);
    return false;
  }

  QJsonObject root = doc.object();
  QString validationError;
  if (!validateThemeJson(root, validationError)) {
    qWarning() << "[ThemeLoader] Theme validation failed:" << validationError
               << "Falling back to default.";
    emit themeLoadError(validationError);
    return false;
  }

  // Parse theme with default fallback
  ThemeTokens tokens = parseThemeJson(root, getDefaultDarkStudioTheme());
  if (tokens.id.isEmpty()) {
    tokens.id = QFileInfo(filePath).baseName();
  }

  m_tokens = tokens;
  watchActiveFile(filePath);
  emit themeChanged();
  return true;
}

void ThemeLoader::setThemeId(const QString &id) {
  // If found in dynamic theme registry, activate it
  if (m_themeRegistry.contains(id)) {
    const auto &entry = m_themeRegistry[id];
    m_tokens = entry.tokens;
    if (!entry.filePath.isEmpty() && !entry.filePath.startsWith(":")) {
      watchActiveFile(entry.filePath);
    } else {
      watchActiveFile("");
    }
    emit themeChanged();
    return;
  }

  // Check if id is an explicit file path on disk
  if (QFile::exists(id) && loadThemeFromFile(id)) {
    return;
  }

  // Check user themes directory
  QString candidate = getUserThemesDir() + "/" + id + ".json";
  if (QFile::exists(candidate)) {
    if (loadThemeFromFile(candidate)) {
      return;
    }
  }

  // Fall back to default dark studio
  qWarning() << "[ThemeLoader] Theme id not found or invalid:" << id
             << "- falling back to default.";
  if (m_themeRegistry.contains("dark-studio")) {
    const auto &entry = m_themeRegistry["dark-studio"];
    m_tokens = entry.tokens;
    if (!entry.filePath.isEmpty() && !entry.filePath.startsWith(":")) {
      watchActiveFile(entry.filePath);
    } else {
      watchActiveFile("");
    }
  } else {
    m_tokens = getDefaultDarkStudioTheme();
    watchActiveFile("");
  }
  emit themeChanged();
}

void ThemeLoader::setUiScale(qreal scale) {
  qreal clamped = std::clamp(scale, 0.7, 1.5);
  if (qFuzzyCompare(m_uiScale, clamped))
    return;
  m_uiScale = clamped;
  emit uiScaleChanged();
  emit themeChanged();
}

void ThemeLoader::setDensityPreset(const QString &preset) {
  if (m_densityPreset == preset)
    return;
  m_densityPreset = preset;
  if (preset == "Compact") {
    setUiScale(0.85);
  } else if (preset == "Comfortable") {
    setUiScale(1.15);
  } else {
    setUiScale(1.0); // Standard
  }
}

bool ThemeLoader::installTheme(const QString &sourceFilePath) {
  QString cleanSource = sourceFilePath;
  QUrl url = QUrl::fromUserInput(sourceFilePath);
  if (url.isLocalFile()) {
    cleanSource = url.toLocalFile();
  } else if (cleanSource.startsWith("file://")) {
    cleanSource = cleanSource.mid(7);
  }

  QFileInfo srcInfo(cleanSource);
  if (!srcInfo.exists() || !srcInfo.isFile()) {
    QString err = QString("Theme file does not exist: %1").arg(cleanSource);
    qWarning() << "[ThemeLoader]" << err;
    emit themeLoadError(err);
    return false;
  }

  // Validate before copying
  QFile file(cleanSource);
  if (!file.open(QIODevice::ReadOnly)) {
    emit themeLoadError("Could not read theme file");
    return false;
  }
  QJsonParseError pErr;
  QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &pErr);
  if (pErr.error != QJsonParseError::NoError || !doc.isObject()) {
    emit themeLoadError(
        QString("Invalid theme JSON: %1").arg(pErr.errorString()));
    return false;
  }

  QString valErr;
  if (!validateThemeJson(doc.object(), valErr)) {
    emit themeLoadError(valErr);
    return false;
  }

  ThemeTokens tokens = parseThemeJson(doc.object());
  QString targetFileName =
      tokens.id.isEmpty() ? srcInfo.fileName() : (tokens.id + ".json");
  QString destPath = getUserThemesDir() + "/" + targetFileName;
  if (QFile::exists(destPath)) {
    QFile::remove(destPath);
  }

  if (!QFile::copy(cleanSource, destPath)) {
    emit themeLoadError("Failed to copy theme file to user themes folder");
    return false;
  }

  refreshAvailableThemes();
  setThemeId(tokens.id.isEmpty() ? QFileInfo(targetFileName).baseName()
                                 : tokens.id);
  return true;
}

bool ThemeLoader::deleteTheme(const QString &id) {
  if (!m_themeRegistry.contains(id)) {
    return false;
  }
  const auto &entry = m_themeRegistry[id];
  if (entry.isBuiltIn || !entry.isUserTheme) {
    qWarning() << "[ThemeLoader] Cannot delete built-in theme:" << id;
    emit themeLoadError("Cannot delete built-in theme");
    return false;
  }

  if (!entry.filePath.isEmpty() && QFile::exists(entry.filePath)) {
    if (!QFile::remove(entry.filePath)) {
      emit themeLoadError("Failed to remove theme file");
      return false;
    }
  }

  bool wasActive = (m_tokens.id == id);
  refreshAvailableThemes();

  if (wasActive) {
    setThemeId("dark-studio");
  }

  return true;
}

bool ThemeLoader::exportTheme(const QString &id, const QString &destFilePath) {
  if (!m_themeRegistry.contains(id)) {
    emit themeLoadError("Theme not found for export");
    return false;
  }

  const auto &entry = m_themeRegistry[id];
  QFile outFile(destFilePath);
  if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate |
                    QIODevice::Text)) {
    emit themeLoadError(
        QString("Cannot write to destination: %1").arg(destFilePath));
    return false;
  }

  // If source file exists on disk, copy raw content
  if (!entry.filePath.isEmpty() && QFile::exists(entry.filePath)) {
    QFile inFile(entry.filePath);
    if (inFile.open(QIODevice::ReadOnly)) {
      outFile.write(inFile.readAll());
      return true;
    }
  }

  // Otherwise construct JSON document from tokens
  QJsonObject root;
  QJsonObject meta;
  meta["id"] = entry.tokens.id;
  meta["name"] = entry.tokens.name;
  meta["author"] = entry.tokens.author;
  meta["version"] = entry.tokens.version;
  meta["apiVersion"] = entry.tokens.apiVersion;
  meta["description"] = entry.tokens.description;
  root["meta"] = meta;

  QJsonObject colors;
  colors["background"] = entry.tokens.background;
  colors["surface"] = entry.tokens.surface;
  colors["surfaceElevated"] = entry.tokens.surfaceElevated;
  colors["panelBorder"] = entry.tokens.panelBorder;
  colors["accent"] = entry.tokens.accent;
  colors["accentHover"] = entry.tokens.accentHover;
  colors["textPrimary"] = entry.tokens.textPrimary;
  colors["textSecondary"] = entry.tokens.textSecondary;
  colors["textMuted"] = entry.tokens.textMuted;
  colors["selection"] = entry.tokens.selection;
  colors["error"] = entry.tokens.error;
  colors["warning"] = entry.tokens.warning;
  colors["success"] = entry.tokens.success;
  root["colors"] = colors;

  QJsonObject typo;
  typo["fontFamily"] = entry.tokens.fontFamily;
  typo["fontSizeSmall"] = entry.tokens.fontSizeSmall;
  typo["fontSizeBase"] = entry.tokens.fontSizeBase;
  typo["fontSizeLarge"] = entry.tokens.fontSizeLarge;
  typo["fontSizeTitle"] = entry.tokens.fontSizeTitle;
  typo["fontWeightScale"] = entry.tokens.fontWeightScale;
  typo["lineHeightScale"] = entry.tokens.lineHeightScale;
  root["typography"] = typo;

  QJsonObject metrics;
  metrics["spacingSmall"] = entry.tokens.spacingSmall;
  metrics["spacingMedium"] = entry.tokens.spacingMedium;
  metrics["spacingLarge"] = entry.tokens.spacingLarge;
  metrics["cornerRadiusSmall"] = entry.tokens.cornerRadiusSmall;
  metrics["cornerRadiusMedium"] = entry.tokens.cornerRadiusMedium;
  metrics["panelPadding"] = entry.tokens.panelPadding;
  root["metrics"] = metrics;

  QJsonDocument doc(root);
  outFile.write(doc.toJson(QJsonDocument::Indented));
  return true;
}

void ThemeLoader::openThemesFolder() {
  QDir().mkpath(getUserThemesDir());
  QDesktopServices::openUrl(QUrl::fromLocalFile(getUserThemesDir()));
}

void ThemeLoader::refreshAvailableThemes() {
  m_availableThemes.clear();
  m_themeRegistry.clear();

  QString userDir = getUserThemesDir();

  // Scan paths in reverse priority order (lowest priority first, highest last)
  // so that user themes overwrite bundled ones of the same id
  QStringList searchPaths = getThemeSearchPaths();
  std::reverse(searchPaths.begin(), searchPaths.end());

  for (const QString &dirPath : searchPaths) {
    QDir dir(dirPath);
    if (!dir.exists()) {
      continue;
    }

    // Watch physical directories on disk for live changes
    if (!dirPath.startsWith(":") && !dirPath.startsWith("qrc:")) {
      if (!m_watchedDirectories.contains(dirPath)) {
        m_watcher.addPath(dirPath);
        m_watchedDirectories.append(dirPath);
      }
    }

    QStringList files =
        dir.entryList(QStringList() << "*.json", QDir::Files, QDir::Name);
    for (const QString &fileName : files) {
      QString fullPath = dir.filePath(fileName);
      QFile file(fullPath);
      if (!file.open(QIODevice::ReadOnly)) {
        continue;
      }
      QJsonParseError parseErr;
      QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseErr);
      if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        continue;
      }
      QJsonObject obj = doc.object();
      QString valErr;
      if (!validateThemeJson(obj, valErr)) {
        continue;
      }

      ThemeTokens tokens = parseThemeJson(obj, getDefaultDarkStudioTheme());
      if (tokens.id.isEmpty()) {
        tokens.id = QFileInfo(fileName).baseName();
      }

      bool isUser = (QDir(dirPath) == QDir(userDir));
      bool isBuiltIn = !isUser;

      ThemeMetadata meta;
      meta.id = tokens.id;
      meta.name = tokens.name;
      meta.author = tokens.author;
      meta.version = tokens.version;
      meta.apiVersion = tokens.apiVersion;
      meta.description = tokens.description;
      meta.isBuiltIn = isBuiltIn;
      meta.isUserTheme = isUser;
      meta.filePath = fullPath;
      meta.tokens = tokens;

      m_themeRegistry[tokens.id] = meta;
    }
  }

  // Ensure built-in fallbacks exist in registry even if disk/resource scanning
  // found nothing
  if (!m_themeRegistry.contains("dark-studio")) {
    ThemeTokens ds = getDefaultDarkStudioTheme();
    m_themeRegistry["dark-studio"] = ThemeMetadata{
        ds.id,          ds.name, ds.author, ds.version, ds.apiVersion,
        ds.description, true,    false,     "",         ds};
  }
  if (!m_themeRegistry.contains("nord-audiophile")) {
    ThemeTokens nord = getBuiltInNordTheme();
    m_themeRegistry["nord-audiophile"] = ThemeMetadata{
        nord.id,          nord.name, nord.author, nord.version, nord.apiVersion,
        nord.description, true,      false,       "",           nord};
  }
  if (!m_themeRegistry.contains("solarized-dark")) {
    ThemeTokens sol = getBuiltInSolarizedDarkTheme();
    m_themeRegistry["solarized-dark"] = ThemeMetadata{
        sol.id,          sol.name, sol.author, sol.version, sol.apiVersion,
        sol.description, true,     false,      "",          sol};
  }

  // Sort: dark-studio first, then other built-in themes, then user themes
  // alphabetically
  QList<ThemeMetadata> sortedList = m_themeRegistry.values();
  std::sort(sortedList.begin(), sortedList.end(),
            [](const ThemeMetadata &a, const ThemeMetadata &b) {
              if (a.id == "dark-studio")
                return true;
              if (b.id == "dark-studio")
                return false;
              if (a.isBuiltIn != b.isBuiltIn)
                return a.isBuiltIn > b.isBuiltIn;
              return a.name.localeAwareCompare(b.name) < 0;
            });

  for (const auto &meta : sortedList) {
    QVariantMap map;
    map["id"] = meta.id;
    map["name"] = meta.name;
    map["author"] = meta.author;
    map["version"] = meta.version;
    map["apiVersion"] = meta.apiVersion;
    map["description"] = meta.description;
    map["isBuiltIn"] = meta.isBuiltIn;
    map["isUserTheme"] = meta.isUserTheme;
    map["filePath"] = meta.filePath;

    QVariantMap colors;
    colors["background"] = meta.tokens.background;
    colors["surface"] = meta.tokens.surface;
    colors["surfaceElevated"] = meta.tokens.surfaceElevated;
    colors["panelBorder"] = meta.tokens.panelBorder;
    colors["accent"] = meta.tokens.accent;
    colors["accentHover"] = meta.tokens.accentHover;
    colors["textPrimary"] = meta.tokens.textPrimary;
    colors["textSecondary"] = meta.tokens.textSecondary;
    colors["textMuted"] = meta.tokens.textMuted;
    colors["selection"] = meta.tokens.selection;
    colors["error"] = meta.tokens.error;
    colors["warning"] = meta.tokens.warning;
    colors["success"] = meta.tokens.success;
    map["colors"] = colors;

    m_availableThemes.append(map);
  }

  emit availableThemesChanged();

  // If active theme exists in registry, reload its tokens (handles live
  // editing)
  if (!m_tokens.id.isEmpty() && m_themeRegistry.contains(m_tokens.id)) {
    m_tokens = m_themeRegistry[m_tokens.id].tokens;
    emit themeChanged();
  }
}

void ThemeLoader::setCustomColorToken(const QString &tokenName,
                                      const QString &hexColor) {
  if (!isValidColorName(hexColor))
    return;
  if (tokenName == QLatin1String("background"))
    m_tokens.background = hexColor;
  else if (tokenName == QLatin1String("surface"))
    m_tokens.surface = hexColor;
  else if (tokenName == QLatin1String("surfaceElevated"))
    m_tokens.surfaceElevated = hexColor;
  else if (tokenName == QLatin1String("panelBorder"))
    m_tokens.panelBorder = hexColor;
  else if (tokenName == QLatin1String("accent"))
    m_tokens.accent = hexColor;
  else if (tokenName == QLatin1String("accentHover"))
    m_tokens.accentHover = hexColor;
  else if (tokenName == QLatin1String("textPrimary"))
    m_tokens.textPrimary = hexColor;
  else if (tokenName == QLatin1String("textSecondary"))
    m_tokens.textSecondary = hexColor;
  else if (tokenName == QLatin1String("textMuted"))
    m_tokens.textMuted = hexColor;
  else if (tokenName == QLatin1String("selection"))
    m_tokens.selection = hexColor;
  else if (tokenName == QLatin1String("error"))
    m_tokens.error = hexColor;
  else if (tokenName == QLatin1String("warning"))
    m_tokens.warning = hexColor;
  else if (tokenName == QLatin1String("success"))
    m_tokens.success = hexColor;
  else
    return;
  emit themeChanged();
}

void ThemeLoader::applyColorOverrides(
    const QMap<QString, QString> &colorOverrides) {
  if (colorOverrides.isEmpty())
    return;
  bool changed = false;
  for (auto it = colorOverrides.constBegin(); it != colorOverrides.constEnd();
       ++it) {
    if (!isValidColorName(it.value()))
      continue;
    const QString &k = it.key();
    const QString &v = it.value();
    if (k == QLatin1String("background")) {
      m_tokens.background = v;
      changed = true;
    } else if (k == QLatin1String("surface")) {
      m_tokens.surface = v;
      changed = true;
    } else if (k == QLatin1String("surfaceElevated")) {
      m_tokens.surfaceElevated = v;
      changed = true;
    } else if (k == QLatin1String("panelBorder")) {
      m_tokens.panelBorder = v;
      changed = true;
    } else if (k == QLatin1String("accent")) {
      m_tokens.accent = v;
      changed = true;
    } else if (k == QLatin1String("accentHover")) {
      m_tokens.accentHover = v;
      changed = true;
    } else if (k == QLatin1String("textPrimary")) {
      m_tokens.textPrimary = v;
      changed = true;
    } else if (k == QLatin1String("textSecondary")) {
      m_tokens.textSecondary = v;
      changed = true;
    } else if (k == QLatin1String("textMuted")) {
      m_tokens.textMuted = v;
      changed = true;
    } else if (k == QLatin1String("selection")) {
      m_tokens.selection = v;
      changed = true;
    } else if (k == QLatin1String("error")) {
      m_tokens.error = v;
      changed = true;
    } else if (k == QLatin1String("warning")) {
      m_tokens.warning = v;
      changed = true;
    } else if (k == QLatin1String("success")) {
      m_tokens.success = v;
      changed = true;
    }
  }
  if (changed) {
    emit themeChanged();
  }
}

} // namespace adapters
