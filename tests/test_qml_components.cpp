#include <QApplication>
#include <QDir>
#include <QFile>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

#include "QtBridge.hpp"
#include "audio/BitPerfectAudioAdapter.hpp"
#include "core/services/LibraryService.hpp"
#include "core/services/PlayerService.hpp"
#include "metadata/TagLibMetadataAdapter.hpp"
#include "providers/AlbumArtImageProvider.hpp"
#include "storage/SqliteDatabaseAdapter.hpp"
#include "theming/ThemeLoader.hpp"

extern int qInitResources_icons();

namespace {

void ensureQmlTestApp() {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  if (!qApp) {
    static int argc = 1;
    static char appName[] = "picaudio_qml_tests";
    static char *argv[] = {appName, nullptr};
    new QApplication(argc, argv);
    qInitResources_icons();
  }
}

class QmlComponentValidationTest : public ::testing::Test {
protected:
  void SetUp() override {
    ensureQmlTestApp();
    m_tempDir = std::make_unique<QTemporaryDir>();
    m_dbAdapter = std::make_unique<adapters::SqliteDatabaseAdapter>();
    std::string dbPath = (m_tempDir->path() + "/test_qml.db").toStdString();
    m_dbAdapter->initialize(dbPath);

    m_metadataAdapter = std::make_unique<adapters::TagLibMetadataAdapter>();
    m_audioEngine = std::make_shared<adapters::BitPerfectAudioEngine>();
    m_audioEngine->initialize(44100, 2);

    m_libraryService = std::make_unique<core::LibraryService>(
        *m_dbAdapter, *m_metadataAdapter);
    m_playerService = std::make_unique<core::PlayerService>(m_audioEngine);
    m_themeLoader = std::make_unique<adapters::ThemeLoader>();

    m_bridge = std::make_unique<QtBridge>(*m_playerService, *m_libraryService,
                                          m_themeLoader.get());

    m_engine = std::make_unique<QQmlApplicationEngine>();
    m_engine->addImageProvider(
        "albumart",
        new AlbumArtImageProvider(m_metadataAdapter.get(), m_dbAdapter.get(),
                                  m_tempDir->path() + "/covers"));
    m_engine->rootContext()->setContextProperty("Theme", m_themeLoader.get());
    m_engine->rootContext()->setContextProperty("bridge", m_bridge.get());
    m_engine->rootContext()->setContextProperty("desktopIntegration", nullptr);
    m_bridge->setQmlEngine(m_engine.get());
  }

  void TearDown() override {
    m_engine.reset();
    m_bridge.reset();
    m_themeLoader.reset();
    m_playerService.reset();
    m_libraryService.reset();
    m_audioEngine.reset();
    m_metadataAdapter.reset();
    m_dbAdapter.reset();
    m_tempDir.reset();
  }

  std::unique_ptr<QTemporaryDir> m_tempDir;
  std::unique_ptr<adapters::SqliteDatabaseAdapter> m_dbAdapter;
  std::unique_ptr<adapters::TagLibMetadataAdapter> m_metadataAdapter;
  std::shared_ptr<adapters::BitPerfectAudioEngine> m_audioEngine;
  std::unique_ptr<core::LibraryService> m_libraryService;
  std::unique_ptr<core::PlayerService> m_playerService;
  std::unique_ptr<adapters::ThemeLoader> m_themeLoader;
  std::unique_ptr<QtBridge> m_bridge;
  std::unique_ptr<QQmlApplicationEngine> m_engine;
};

TEST_F(QmlComponentValidationTest, MainQmlLoadsWithoutError) {
  QUrl url(QStringLiteral("qrc:/PlayerUI/ui_qt/qml/Main.qml"));
  if (!QFile::exists(QStringLiteral(":/PlayerUI/ui_qt/qml/Main.qml")) &&
      QFile::exists(QStringLiteral(":/qt/qml/PlayerUI/ui_qt/qml/Main.qml"))) {
    url = QUrl(QStringLiteral("qrc:/qt/qml/PlayerUI/ui_qt/qml/Main.qml"));
  }

  m_engine->load(url);

  const auto rootObjects = m_engine->rootObjects();
  ASSERT_FALSE(rootObjects.isEmpty())
      << "Main.qml failed to load or create any root object!";
  EXPECT_NE(rootObjects.first(), nullptr);
}

TEST_F(QmlComponentValidationTest, DialogComponentsInstantiateWithoutError) {
  const std::vector<QString> dialogFiles = {
      "ChannelMatrixDialog.qml", "PreferencesDialog.qml",
      "ShortcutsDialog.qml",     "DiagnosticsDialog.qml",
      "AboutDialog.qml",         "SleepTimerDialog.qml",
      "NewPlaylistDialog.qml",   "RenamePlaylistDialog.qml",
      "DeleteConfirmDialog.qml", "OpenUrlDialog.qml",
      "TagEditorDialog.qml"};

  for (const auto &file : dialogFiles) {
    QString qrcPath = "qrc:/PlayerUI/ui_qt/qml/dialogs/" + file;
    if (!QFile::exists(":/" + qrcPath.mid(5)) &&
        QFile::exists(":/qt/qml/PlayerUI/ui_qt/qml/dialogs/" + file)) {
      qrcPath = "qrc:/qt/qml/PlayerUI/ui_qt/qml/dialogs/" + file;
    }

    QQmlComponent component(m_engine.get(), QUrl(qrcPath));
    EXPECT_FALSE(component.isError())
        << "QML Component compilation error in " << file.toStdString() << ":\n"
        << component.errorString().toStdString();

    std::unique_ptr<QObject> instance(component.create());
    EXPECT_FALSE(component.isError()) << "QML Component instantiation error in "
                                      << file.toStdString() << ":\n"
                                      << component.errorString().toStdString();
    EXPECT_NE(instance, nullptr)
        << "Failed to instantiate QML dialog: " << file.toStdString();
  }
}

TEST_F(QmlComponentValidationTest, ViewsAndComponentsInstantiateWithoutError) {
  const std::vector<QString> componentPaths = {
      "components/EqualizerOverlay.qml",
      "components/ToastBanner.qml",
      "components/MovableHeaderArea.qml",
      "components/ResizeGrip.qml",
      "player/MiniPlayer.qml",
      "player/PlayerDeck.qml",
      "navigation/LeftSidebar.qml",
      "views/NowPlayingView.qml",
      "views/QueueView.qml",
      "views/TrackTableView.qml",
      "views/AlbumGridView.qml",
      "views/AlbumDetailView.qml",
      "views/HistoryView.qml"};

  for (const auto &relPath : componentPaths) {
    QString qrcPath = "qrc:/PlayerUI/ui_qt/qml/" + relPath;
    if (!QFile::exists(":/" + qrcPath.mid(5)) &&
        QFile::exists(":/qt/qml/PlayerUI/ui_qt/qml/" + relPath)) {
      qrcPath = "qrc:/qt/qml/PlayerUI/ui_qt/qml/" + relPath;
    }

    QQmlComponent component(m_engine.get(), QUrl(qrcPath));
    EXPECT_FALSE(component.isError()) << "QML Component compilation error in "
                                      << relPath.toStdString() << ":\n"
                                      << component.errorString().toStdString();

    std::unique_ptr<QObject> instance(component.create());
    EXPECT_FALSE(component.isError()) << "QML Component instantiation error in "
                                      << relPath.toStdString() << ":\n"
                                      << component.errorString().toStdString();
    EXPECT_NE(instance, nullptr)
        << "Failed to instantiate QML component: " << relPath.toStdString();
  }
}

} // namespace
