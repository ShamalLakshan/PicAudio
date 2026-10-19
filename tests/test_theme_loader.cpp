#include "theming/ThemeLoader.hpp"
#include <QCoreApplication>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryFile>
#include <gtest/gtest.h>

namespace {

// Helper to construct a valid theme JSON string
QString makeValidThemeJson() {
  return QStringLiteral(R"({
        "meta": {
            "id": "test-theme",
            "name": "Test Theme",
            "author": "Tester",
            "version": "1.0.0",
            "apiVersion": "0.1-unstable",
            "description": "Test description"
        },
        "colors": {
            "background": "#112233",
            "surface": "#223344",
            "surfaceElevated": "#334455",
            "panelBorder": "#445566",
            "accent": "#556677",
            "accentHover": "#667788",
            "textPrimary": "#ffffff",
            "textSecondary": "#cccccc",
            "textMuted": "#888888",
            "selection": "#334455",
            "error": "#ff0000",
            "warning": "#ffaa00",
            "success": "#00ff00"
        },
        "typography": {
            "fontFamily": "Roboto",
            "fontSizeSmall": 10,
            "fontSizeBase": 12,
            "fontSizeLarge": 15,
            "fontSizeTitle": 20,
            "fontWeightScale": 1.0,
            "lineHeightScale": 1.2
        },
        "metrics": {
            "spacingSmall": 5,
            "spacingMedium": 10,
            "spacingLarge": 20,
            "cornerRadiusSmall": 3,
            "cornerRadiusMedium": 6,
            "panelPadding": 12
        }
    })");
}

} // namespace

class ThemeLoaderTest : public ::testing::Test {
protected:
  static void SetUpTestSuite() {
    if (!QCoreApplication::instance()) {
      qputenv("QT_QPA_PLATFORM", "offscreen");
      static int argc = 1;
      static char *argv[] = {const_cast<char *>("picaudio_tests"), nullptr};
      new QGuiApplication(argc, argv);
    }
  }
};

TEST_F(ThemeLoaderTest, ValidThemeValidationAndParsing) {
  QString jsonStr = makeValidThemeJson();
  QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
  ASSERT_TRUE(doc.isObject());

  QString errorReason;
  EXPECT_TRUE(
      adapters::ThemeLoader::validateThemeJson(doc.object(), errorReason));
  EXPECT_TRUE(errorReason.isEmpty());

  adapters::ThemeTokens tokens =
      adapters::ThemeLoader::parseThemeJson(doc.object());
  EXPECT_EQ(tokens.id, "test-theme");
  EXPECT_EQ(tokens.name, "Test Theme");
  EXPECT_EQ(tokens.background, "#112233");
  EXPECT_EQ(tokens.accent, "#556677");
  EXPECT_EQ(tokens.fontFamily, "Roboto");
  EXPECT_EQ(tokens.fontSizeBase, 12);
  EXPECT_EQ(tokens.spacingSmall, 5);
}

TEST_F(ThemeLoaderTest, MalformedJsonSyntaxFallback) {
  adapters::ThemeLoader loader;
  QString defaultBg = loader.background();

  QTemporaryFile tempFile;
  ASSERT_TRUE(tempFile.open());
  // Write invalid JSON with trailing comma
  tempFile.write("{ \"meta\": { \"name\": \"Broken\", }, }");
  tempFile.close();

  bool loaded = loader.loadThemeFromFile(tempFile.fileName());
  EXPECT_FALSE(loaded);
  // Loader must safely retain or fall back to default theme
  EXPECT_EQ(loader.background(), defaultBg);
}

TEST_F(ThemeLoaderTest, MissingRequiredFieldsFailsValidation) {
  // Missing required "colors" block
  QString invalidJson = QStringLiteral(R"({
        "meta": {
            "name": "Missing Colors",
            "author": "Tester",
            "version": "1.0",
            "apiVersion": "0.1"
        }
    })");

  QJsonDocument doc = QJsonDocument::fromJson(invalidJson.toUtf8());
  ASSERT_TRUE(doc.isObject());

  QString errorReason;
  EXPECT_FALSE(
      adapters::ThemeLoader::validateThemeJson(doc.object(), errorReason));
  EXPECT_FALSE(errorReason.isEmpty());
}

TEST_F(ThemeLoaderTest, NonExistentFilePathHandledGracefully) {
  adapters::ThemeLoader loader;
  bool loaded = loader.loadThemeFromFile("/non/existent/path/to/theme.json");
  EXPECT_FALSE(loaded);
  EXPECT_EQ(loader.themeId(), "dark-studio");
}

TEST_F(ThemeLoaderTest, PartialThemeFallsBackToDefaults) {
  adapters::ThemeTokens defaultTokens =
      adapters::ThemeLoader::getDefaultDarkStudioTheme();

  // Partial JSON providing only accent color
  QString partialJson = QStringLiteral(R"({
        "colors": {
            "accent": "#ff0077"
        }
    })");

  QJsonDocument doc = QJsonDocument::fromJson(partialJson.toUtf8());
  ASSERT_TRUE(doc.isObject());

  adapters::ThemeTokens tokens =
      adapters::ThemeLoader::parseThemeJson(doc.object(), defaultTokens);
  EXPECT_EQ(tokens.accent, "#ff0077");
  // Missing fields fallback to default dark studio
  EXPECT_EQ(tokens.background, defaultTokens.background);
  EXPECT_EQ(tokens.surface, defaultTokens.surface);
  EXPECT_EQ(tokens.fontFamily, defaultTokens.fontFamily);
}

TEST_F(ThemeLoaderTest, BuiltInThemesSwitching) {
  adapters::ThemeLoader loader;
  EXPECT_EQ(loader.themeId(), "dark-studio");

  loader.setThemeId("nord-audiophile");
  EXPECT_EQ(loader.themeId(), "nord-audiophile");
  EXPECT_EQ(loader.background(), "#2e3440");

  loader.setThemeId("solarized-dark");
  EXPECT_EQ(loader.themeId(), "solarized-dark");
  EXPECT_EQ(loader.background(), "#002b36");

  loader.setThemeId("dark-studio");
  EXPECT_EQ(loader.themeId(), "dark-studio");
  EXPECT_EQ(loader.background(), "#141416");
}

TEST_F(ThemeLoaderTest, DynamicDiscoveryAndAvailableThemes) {
  adapters::ThemeLoader loader;
  QVariantList list = loader.availableThemes();
  EXPECT_FALSE(list.isEmpty());

  bool foundDarkStudio = false;
  for (const auto &item : list) {
    QVariantMap map = item.toMap();
    if (map["id"].toString() == "dark-studio") {
      foundDarkStudio = true;
      EXPECT_EQ(map["name"].toString(), "Dark Studio");
      EXPECT_TRUE(map.contains("colors"));
      QVariantMap colors = map["colors"].toMap();
      EXPECT_EQ(colors["background"].toString(), "#141416");
      EXPECT_EQ(colors["accent"].toString(), "#3a82f7");
    }
  }
  EXPECT_TRUE(foundDarkStudio);
}

TEST_F(ThemeLoaderTest, ThemeInstallationExportAndDelete) {
  adapters::ThemeLoader loader;
  QString jsonContent = makeValidThemeJson();

  QTemporaryFile tempThemeFile;
  ASSERT_TRUE(tempThemeFile.open());
  tempThemeFile.write(jsonContent.toUtf8());
  tempThemeFile.close();

  // Install theme
  bool installed = loader.installTheme(tempThemeFile.fileName());
  EXPECT_TRUE(installed);
  EXPECT_EQ(loader.themeId(), "test-theme");
  EXPECT_EQ(loader.background(), "#112233");

  // Export theme
  QTemporaryFile tempExportFile;
  ASSERT_TRUE(tempExportFile.open());
  QString exportPath = tempExportFile.fileName();
  tempExportFile.close();

  bool exported = loader.exportTheme("test-theme", exportPath);
  EXPECT_TRUE(exported);
  QFile exportedFile(exportPath);
  EXPECT_TRUE(exportedFile.open(QIODevice::ReadOnly));
  QJsonDocument exportDoc = QJsonDocument::fromJson(exportedFile.readAll());
  EXPECT_TRUE(exportDoc.isObject());
  EXPECT_EQ(exportDoc.object()["meta"].toObject()["id"].toString(),
            "test-theme");

  // Delete user theme
  bool deleted = loader.deleteTheme("test-theme");
  EXPECT_TRUE(deleted);
  EXPECT_EQ(loader.themeId(), "dark-studio");

  // Deleting built-in theme should fail
  bool deleteBuiltIn = loader.deleteTheme("dark-studio");
  EXPECT_FALSE(deleteBuiltIn);
}

TEST_F(ThemeLoaderTest, ThemeSearchPathsInspection) {
  adapters::ThemeLoader loader;
  QStringList paths = loader.themeSearchPaths();
  EXPECT_FALSE(paths.isEmpty());
  EXPECT_TRUE(paths.contains(":/themes"));
}
