#pragma once

#include <QCoreApplication>
#include <QDir>
#include <QFileSystemWatcher>
#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <memory>

namespace adapters {

struct ThemeTokens {
  // Meta
  QString id{"dark-studio"};
  QString name{"Dark Studio"};
  QString author{"PicAudio"};
  QString version{"1.0.0"};
  QString apiVersion{"0.1-unstable"};
  QString description{"Default dark theme"};

  // Colors
  QString background{"#000000"};
  QString surface{"#0d0d0d"};
  QString surfaceElevated{"#171717"};
  QString panelBorder{"#262626"};
  QString accent{"#ffffff"};
  QString accentHover{"#e5e5e5"};
  QString textPrimary{"#ffffff"};
  QString textSecondary{"#a3a3a3"};
  QString textMuted{"#737373"};
  QString selection{"#262626"};
  QString error{"#ef4444"};
  QString warning{"#f59e0b"};
  QString success{"#22c55e"};

  // Typography
  QString fontFamily{"sans-serif"};
  int fontSizeSmall{9};
  int fontSizeBase{11};
  int fontSizeLarge{14};
  int fontSizeTitle{18};
  qreal fontWeightScale{1.0};
  qreal lineHeightScale{1.2};

  // Metrics
  int spacingSmall{4};
  int spacingMedium{8};
  int spacingLarge{16};
  int cornerRadiusSmall{2};
  int cornerRadiusMedium{4};
  int panelPadding{10};
};

struct ThemeMetadata {
  QString id;
  QString name;
  QString author;
  QString version;
  QString apiVersion;
  QString description;
  bool isBuiltIn{false};
  bool isUserTheme{false};
  QString filePath;
  ThemeTokens tokens;
};

/**
 * @brief Manages JSON themes, multi-tier search paths, dynamic discovery, and
 * hot-reloading.
 */
class ThemeLoader : public QObject {
  Q_OBJECT

  // Meta properties
  Q_PROPERTY(QString themeId READ themeId WRITE setThemeId NOTIFY themeChanged)
  Q_PROPERTY(QString themeName READ themeName NOTIFY themeChanged)
  Q_PROPERTY(QString themeAuthor READ themeAuthor NOTIFY themeChanged)
  Q_PROPERTY(QString themeVersion READ themeVersion NOTIFY themeChanged)
  Q_PROPERTY(QString themeDescription READ themeDescription NOTIFY themeChanged)

  // Color tokens
  Q_PROPERTY(QString background READ background NOTIFY themeChanged)
  Q_PROPERTY(QString surface READ surface NOTIFY themeChanged)
  Q_PROPERTY(QString surfaceElevated READ surfaceElevated NOTIFY themeChanged)
  Q_PROPERTY(QString panelBorder READ panelBorder NOTIFY themeChanged)
  Q_PROPERTY(QString accent READ accent NOTIFY themeChanged)
  Q_PROPERTY(QString accentHover READ accentHover NOTIFY themeChanged)
  Q_PROPERTY(QString textPrimary READ textPrimary NOTIFY themeChanged)
  Q_PROPERTY(QString textSecondary READ textSecondary NOTIFY themeChanged)
  Q_PROPERTY(QString textMuted READ textMuted NOTIFY themeChanged)
  Q_PROPERTY(QString selection READ selection NOTIFY themeChanged)
  Q_PROPERTY(QString error READ error NOTIFY themeChanged)
  Q_PROPERTY(QString warning READ warning NOTIFY themeChanged)
  Q_PROPERTY(QString success READ success NOTIFY themeChanged)

  // Typography tokens
  Q_PROPERTY(QString fontFamily READ fontFamily NOTIFY themeChanged)
  Q_PROPERTY(int fontSizeSmall READ fontSizeSmall NOTIFY themeChanged)
  Q_PROPERTY(int fontSizeBase READ fontSizeBase NOTIFY themeChanged)
  Q_PROPERTY(int fontSizeLarge READ fontSizeLarge NOTIFY themeChanged)
  Q_PROPERTY(int fontSizeTitle READ fontSizeTitle NOTIFY themeChanged)
  Q_PROPERTY(qreal fontWeightScale READ fontWeightScale NOTIFY themeChanged)
  Q_PROPERTY(qreal lineHeightScale READ lineHeightScale NOTIFY themeChanged)

  // Metric tokens
  Q_PROPERTY(int spacingSmall READ spacingSmall NOTIFY themeChanged)
  Q_PROPERTY(int spacingMedium READ spacingMedium NOTIFY themeChanged)
  Q_PROPERTY(int spacingLarge READ spacingLarge NOTIFY themeChanged)
  Q_PROPERTY(int cornerRadiusSmall READ cornerRadiusSmall NOTIFY themeChanged)
  Q_PROPERTY(int cornerRadiusMedium READ cornerRadiusMedium NOTIFY themeChanged)
  Q_PROPERTY(int panelPadding READ panelPadding NOTIFY themeChanged)

  // UI Scale / Density
  Q_PROPERTY(qreal uiScale READ uiScale WRITE setUiScale NOTIFY uiScaleChanged)
  Q_PROPERTY(QString densityPreset READ densityPreset WRITE setDensityPreset
                 NOTIFY uiScaleChanged)

  // Theme list for UI selector
  Q_PROPERTY(QVariantList availableThemes READ availableThemes NOTIFY
                 availableThemesChanged)

public:
  explicit ThemeLoader(QObject *parent = nullptr);
  ~ThemeLoader() override = default;

  // Getters
  [[nodiscard]] QString themeId() const { return m_tokens.id; }
  [[nodiscard]] QString themeName() const { return m_tokens.name; }
  [[nodiscard]] QString themeAuthor() const { return m_tokens.author; }
  [[nodiscard]] QString themeVersion() const { return m_tokens.version; }
  [[nodiscard]] QString themeDescription() const {
    return m_tokens.description;
  }

  [[nodiscard]] QString background() const { return m_tokens.background; }
  [[nodiscard]] QString surface() const { return m_tokens.surface; }
  [[nodiscard]] QString surfaceElevated() const {
    return m_tokens.surfaceElevated;
  }
  [[nodiscard]] QString panelBorder() const { return m_tokens.panelBorder; }
  [[nodiscard]] QString accent() const { return m_tokens.accent; }
  [[nodiscard]] QString accentHover() const { return m_tokens.accentHover; }
  [[nodiscard]] QString textPrimary() const { return m_tokens.textPrimary; }
  [[nodiscard]] QString textSecondary() const { return m_tokens.textSecondary; }
  [[nodiscard]] QString textMuted() const { return m_tokens.textMuted; }
  [[nodiscard]] QString selection() const { return m_tokens.selection; }
  [[nodiscard]] QString error() const { return m_tokens.error; }
  [[nodiscard]] QString warning() const { return m_tokens.warning; }
  [[nodiscard]] QString success() const { return m_tokens.success; }

  [[nodiscard]] QString fontFamily() const { return m_tokens.fontFamily; }
  [[nodiscard]] int fontSizeSmall() const {
    return static_cast<int>(m_tokens.fontSizeSmall * m_uiScale);
  }
  [[nodiscard]] int fontSizeBase() const {
    return static_cast<int>(m_tokens.fontSizeBase * m_uiScale);
  }
  [[nodiscard]] int fontSizeLarge() const {
    return static_cast<int>(m_tokens.fontSizeLarge * m_uiScale);
  }
  [[nodiscard]] int fontSizeTitle() const {
    return static_cast<int>(m_tokens.fontSizeTitle * m_uiScale);
  }
  [[nodiscard]] qreal fontWeightScale() const {
    return m_tokens.fontWeightScale;
  }
  [[nodiscard]] qreal lineHeightScale() const {
    return m_tokens.lineHeightScale;
  }

  [[nodiscard]] int spacingSmall() const {
    return static_cast<int>(m_tokens.spacingSmall * m_uiScale);
  }
  [[nodiscard]] int spacingMedium() const {
    return static_cast<int>(m_tokens.spacingMedium * m_uiScale);
  }
  [[nodiscard]] int spacingLarge() const {
    return static_cast<int>(m_tokens.spacingLarge * m_uiScale);
  }
  [[nodiscard]] int cornerRadiusSmall() const {
    return m_tokens.cornerRadiusSmall;
  }
  [[nodiscard]] int cornerRadiusMedium() const {
    return m_tokens.cornerRadiusMedium;
  }
  [[nodiscard]] int panelPadding() const {
    return static_cast<int>(m_tokens.panelPadding * m_uiScale);
  }

  [[nodiscard]] qreal uiScale() const { return m_uiScale; }
  [[nodiscard]] QString densityPreset() const { return m_densityPreset; }
  [[nodiscard]] QVariantList availableThemes() const {
    return m_availableThemes;
  }
  [[nodiscard]] QString activeThemeFilePath() const {
    return m_activeThemeFilePath;
  }

  // Static validation and defaults
  static ThemeTokens getDefaultDarkStudioTheme();
  static ThemeTokens getBuiltInNordTheme();
  static ThemeTokens getBuiltInSolarizedDarkTheme();
  static bool validateThemeJson(const QJsonObject &root, QString &errorReason);
  static ThemeTokens
  parseThemeJson(const QJsonObject &root,
                 const ThemeTokens &fallback = getDefaultDarkStudioTheme());

public slots:
  void setThemeId(const QString &id);
  void setUiScale(qreal scale);
  void setDensityPreset(const QString &preset);
  bool loadThemeFromFile(const QString &filePath);
  bool installTheme(const QString &sourceFilePath);
  bool deleteTheme(const QString &id);
  bool exportTheme(const QString &id, const QString &destFilePath);
  void openThemesFolder();
  void refreshAvailableThemes();
  [[nodiscard]] QStringList themeSearchPaths() const;
  void setCustomColorToken(const QString &tokenName, const QString &hexColor);
  void applyColorOverrides(const QMap<QString, QString> &colorOverrides);

signals:
  void themeChanged();
  void uiScaleChanged();
  void availableThemesChanged();
  void themeLoadError(const QString &errorMessage);

private:
  void setupWatcher();
  void watchActiveFile(const QString &filePath);
  [[nodiscard]] QString getUserThemesDir() const;
  [[nodiscard]] QStringList getThemeSearchPaths() const;

  ThemeTokens m_tokens;
  qreal m_uiScale{1.15};
  QString m_densityPreset{"Comfortable"};
  QString m_activeThemeFilePath;
  QVariantList m_availableThemes;
  QMap<QString, ThemeMetadata> m_themeRegistry;
  QStringList m_watchedDirectories;
  QFileSystemWatcher m_watcher;
};

} // namespace adapters
