#include "core/services/TabService.hpp"
#include "models/LayoutModel.hpp"
#include "models/TabListModel.hpp"
#include "models/ViewRegistryModel.hpp"
#include <QCoreApplication>
#include <gtest/gtest.h>

class Phase6UiTest : public ::testing::Test {
protected:
  void SetUp() override {
    tabService = std::make_unique<core::TabService>();
    tabModel = std::make_unique<TabListModel>(tabService.get());
    layoutModel = std::make_unique<LayoutModel>();
    viewRegistry = std::make_unique<ViewRegistryModel>();
  }

  std::unique_ptr<core::TabService> tabService;
  std::unique_ptr<TabListModel> tabModel;
  std::unique_ptr<LayoutModel> layoutModel;
  std::unique_ptr<ViewRegistryModel> viewRegistry;
};

TEST_F(Phase6UiTest, TabListModelLifecycleAndRoles) {
  EXPECT_EQ(tabModel->tabCount(), 1);
  EXPECT_EQ(tabModel->activeTabIndex(), 0);
  EXPECT_EQ(tabModel->activeTitle(), "Music");
  EXPECT_EQ(tabModel->activeSourceId(), "library://music");
  EXPECT_EQ(tabModel->activeViewId(), "tracks_default");
  EXPECT_EQ(tabModel->activeLockMode(), 0);
  EXPECT_FALSE(tabModel->activeIsPinned());

  // Open new tab
  QString tab2 =
      tabModel->openTab("library://inbox", "Inbox", "tracks_default");
  EXPECT_FALSE(tab2.isEmpty());
  EXPECT_EQ(tabModel->tabCount(), 2);
  EXPECT_EQ(tabModel->activeTabIndex(), 1);
  EXPECT_EQ(tabModel->activeTitle(), "Inbox");

  // Duplicate tab
  QString dup = tabModel->duplicateTab(0);
  EXPECT_FALSE(dup.isEmpty());
  EXPECT_EQ(tabModel->tabCount(), 3);

  // Rename
  EXPECT_TRUE(tabModel->renameTab(1, "Cloned Tab"));
  EXPECT_EQ(
      tabModel->data(tabModel->index(1, 0), TabListModel::TitleRole).toString(),
      "Cloned Tab");

  // Move tab
  EXPECT_TRUE(tabModel->moveTab(1, 0));

  // Pin & Lock
  EXPECT_TRUE(tabModel->setTabPinned(0, true));
  EXPECT_TRUE(tabModel->data(tabModel->index(0, 0), TabListModel::IsPinnedRole)
                  .toBool());
  EXPECT_FALSE(tabModel->closeTab(0, false)); // Cannot close pinned

  EXPECT_TRUE(tabModel->setTabLockMode(1, 1));
  EXPECT_EQ(
      tabModel->data(tabModel->index(1, 0), TabListModel::LockModeRole).toInt(),
      1);
  EXPECT_FALSE(tabModel->closeTab(1, false)); // Cannot close locked

  // Close unlocked tab
  EXPECT_TRUE(tabModel->closeTab(2, false));
  EXPECT_EQ(tabModel->tabCount(), 2);

  // Reopen closed tab (Ctrl+Shift+T)
  EXPECT_TRUE(tabModel->canReopenClosedTab());
  QString reopened = tabModel->reopenLastClosedTab();
  EXPECT_FALSE(reopened.isEmpty());
  EXPECT_EQ(tabModel->tabCount(), 3);
}

TEST_F(Phase6UiTest, TabLockRoutingLogic) {
  // Tab 0: Pin and Lock
  tabModel->setTabPinned(0, true);
  tabModel->setTabLockMode(0, 1);

  // Route navigator click while active tab is locked: should open new tab or
  // route to unlocked
  QVariantMap res = tabModel->routeNavigatorClick("library://audiobooks",
                                                  "Audiobooks", false);
  EXPECT_NE(res["type"].toString(), "Denied");
  EXPECT_EQ(tabModel->activeSourceId(), "library://audiobooks");

  // Tab 1 is now active and unlocked: subsequent navigator click updates
  // in-place
  QVariantMap res2 =
      tabModel->routeNavigatorClick("library://podcasts", "Podcasts", false);
  EXPECT_EQ(res2["type"].toString(), "CurrentTabUpdated");
  EXPECT_EQ(tabModel->activeSourceId(), "library://podcasts");

  // Force new tab opens a new tab regardless
  QVariantMap res3 = tabModel->routeNavigatorClick("folder:///media/lossless",
                                                   "Lossless", true);
  EXPECT_EQ(res3["type"].toString(), "NewTabSpawned");
  EXPECT_EQ(tabModel->activeSourceId(), "folder:///media/lossless");

  // Root Lock navigation
  int rootTabIdx = tabModel->activeTabIndex();
  tabModel->setTabRootLock(rootTabIdx, "folder:///media/lossless");
  EXPECT_EQ(tabModel->activeLockMode(), 2);

  // Child subfolder navigation stays in tab
  QVariantMap resChild = tabModel->routeNavigatorClick(
      "folder:///media/lossless/Rock", "Rock", false);
  EXPECT_EQ(resChild["type"].toString(), "CurrentTabUpdated");
  EXPECT_EQ(tabModel->activeSourceId(), "folder:///media/lossless/Rock");

  // Reset to root
  EXPECT_TRUE(tabModel->resetTabToRoot(rootTabIdx));
  EXPECT_EQ(tabModel->activeSourceId(), "folder:///media/lossless");
}

TEST_F(Phase6UiTest, LayoutModelZonesAndStacking) {
  EXPECT_EQ(layoutModel->effectiveLeftSidebarMode(), "permanent");
  EXPECT_EQ(layoutModel->effectiveRightSidebarMode(), "permanent");
  EXPECT_FALSE(layoutModel->effectiveLeftMainPanelVisible());
  EXPECT_FALSE(layoutModel->effectiveRightMainPanelVisible());

  // Check 21 modular elements registered
  QVariantList allElements = layoutModel->getAllAvailableElements();
  EXPECT_EQ(allElements.size(), 21);

  // Verify default elements in Header Bar
  EXPECT_TRUE(layoutModel->hasElement("headerBar", "tabs_bar"));
  EXPECT_TRUE(layoutModel->hasElement("headerBar", "search_box"));
  EXPECT_TRUE(layoutModel->hasElement("headerBar", "command_toolbar"));

  // Verify stacking
  layoutModel->assignElement("lyrics", "rightSidebar", -1, true);
  layoutModel->setElementStacked("lyrics", true, "header_cycle");
  layoutModel->setElementActiveStackIndex("lyrics", 1);
  EXPECT_EQ(layoutModel->getElementActiveStackIndex("lyrics"), 1);

  // Dimension changes
  layoutModel->setLeftSidebarWidth(260.0);
  EXPECT_DOUBLE_EQ(layoutModel->leftSidebarWidth(), 260.0);
  layoutModel->setTopPanelHeight(72.0);
  EXPECT_DOUBLE_EQ(layoutModel->topPanelHeight(), 72.0);

  // Stacking & element movement
  layoutModel->moveElement("lyrics", "floatingWindow", 0);
  EXPECT_TRUE(layoutModel->isFloatingWindowActive());
  EXPECT_TRUE(layoutModel->hasElement("floatingWindow", "lyrics"));
  EXPECT_FALSE(layoutModel->hasElement("rightSidebar", "lyrics"));
}

TEST_F(Phase6UiTest, ViewRegistryAndDynamicLayoutOverrides) {
  // Builtin views
  QVariantList views = viewRegistry->getAllViews();
  EXPECT_GE(views.size(), 5);

  QVariantMap tracksView = viewRegistry->getView("tracks_default");
  EXPECT_EQ(tracksView["baseRendererType"].toString(), "tracks");

  QVariantMap coversView = viewRegistry->getView("covers_default");
  EXPECT_EQ(coversView["baseRendererType"].toString(), "covers");

  QVariantMap jukeboxView = viewRegistry->getView("jukebox_default");
  EXPECT_EQ(jukeboxView["baseRendererType"].toString(), "jukebox");
  QVariantMap jukeOverrides =
      viewRegistry->getPanelOverrides("jukebox_default");
  EXPECT_EQ(jukeOverrides["leftSidebar"].toString(), "hidden");

  // Create custom saved view with overrides
  QString customId = viewRegistry->createCustomView("Classical View", "tracks",
                                                    "tracks_default");
  EXPECT_FALSE(customId.isEmpty());

  QVariantMap customOverrides;
  customOverrides["leftSidebar"] = "auto_open";
  customOverrides["rightSidebar"] = "auto_open";
  customOverrides["leftMainPanel"] = "visible";
  customOverrides["topPanel"] = "hidden";
  customOverrides["columnBrowser"] = "visible";

  EXPECT_TRUE(viewRegistry->updateCustomView(customId, "Classical View",
                                             {"trackNumber", "title", "artist"},
                                             "composer", 180, customOverrides));

  // Apply layout overrides from custom view to LayoutModel
  layoutModel->applyLayoutOverrides(viewRegistry->getPanelOverrides(customId));
  EXPECT_EQ(layoutModel->effectiveLeftSidebarMode(), "auto_open");
  EXPECT_EQ(layoutModel->effectiveRightSidebarMode(), "auto_open");
  EXPECT_TRUE(layoutModel->effectiveLeftMainPanelVisible());
  EXPECT_FALSE(layoutModel->effectiveTopPanelVisible());
  EXPECT_TRUE(layoutModel->columnBrowserVisible());

  // Clear overrides (restores default global layout instantaneously)
  layoutModel->clearLayoutOverrides();
  EXPECT_EQ(layoutModel->effectiveLeftSidebarMode(), "permanent");
  EXPECT_EQ(layoutModel->effectiveRightSidebarMode(), "permanent");
  EXPECT_FALSE(layoutModel->effectiveLeftMainPanelVisible());

  // Delete custom view
  EXPECT_TRUE(viewRegistry->deleteCustomView(customId));
}

TEST_F(Phase6UiTest, IndependentTabConfigurationsAndDirectoryScoping) {
  // Tab 0: Library Music, set view to covers_default
  EXPECT_EQ(tabModel->activeSourceId(), "library://music");
  EXPECT_TRUE(tabModel->updateActiveView("covers_default"));
  EXPECT_EQ(tabModel->activeViewId(), "covers_default");

  // Open Tab 1: Directory Rock, view tracks_default
  QString rockTab = tabModel->openTab("folder:///home/user/Music/Rock", "Rock",
                                      "tracks_default");
  EXPECT_FALSE(rockTab.isEmpty());
  EXPECT_EQ(tabModel->activeTabIndex(), 1);
  EXPECT_EQ(tabModel->activeSourceId(), "folder:///home/user/Music/Rock");
  EXPECT_EQ(tabModel->activeViewId(), "tracks_default");

  // Open Tab 2: Directory Jazz, view album_and_tracks_default
  QString jazzTab = tabModel->openTab("folder:///home/user/Music/Jazz", "Jazz",
                                      "album_and_tracks_default");
  EXPECT_FALSE(jazzTab.isEmpty());
  EXPECT_EQ(tabModel->activeTabIndex(), 2);
  EXPECT_EQ(tabModel->activeSourceId(), "folder:///home/user/Music/Jazz");
  EXPECT_EQ(tabModel->activeViewId(), "album_and_tracks_default");

  // Open Tab 3: Playlist, view jukebox_default
  QString plTab =
      tabModel->openTab("playlist://Chillout", "Chillout", "jukebox_default");
  EXPECT_FALSE(plTab.isEmpty());
  EXPECT_EQ(tabModel->activeTabIndex(), 3);
  EXPECT_EQ(tabModel->activeSourceId(), "playlist://Chillout");
  EXPECT_EQ(tabModel->activeViewId(), "jukebox_default");

  // Open Tab 4: Now Playing view
  QString npTab = tabModel->openTab("library://now_playing", "Now Playing",
                                    "now_playing_default");
  EXPECT_FALSE(npTab.isEmpty());
  EXPECT_EQ(tabModel->activeTabIndex(), 4);
  EXPECT_EQ(tabModel->activeSourceId(), "library://now_playing");
  EXPECT_EQ(tabModel->activeViewId(), "now_playing_default");

  // Switch back to Tab 0: retains covers_default and library://music
  EXPECT_TRUE(tabModel->setActiveTabIndex(0));
  EXPECT_EQ(tabModel->activeSourceId(), "library://music");
  EXPECT_EQ(tabModel->activeViewId(), "covers_default");

  // Switch to Tab 1: retains tracks_default and folder:///home/user/Music/Rock
  EXPECT_TRUE(tabModel->setActiveTabIndex(1));
  EXPECT_EQ(tabModel->activeSourceId(), "folder:///home/user/Music/Rock");
  EXPECT_EQ(tabModel->activeViewId(), "tracks_default");

  // Switch to Tab 2: retains album_and_tracks_default and
  // folder:///home/user/Music/Jazz
  EXPECT_TRUE(tabModel->setActiveTabIndex(2));
  EXPECT_EQ(tabModel->activeSourceId(), "folder:///home/user/Music/Jazz");
  EXPECT_EQ(tabModel->activeViewId(), "album_and_tracks_default");

  // Switch to Tab 3: retains jukebox_default and playlist://Chillout
  EXPECT_TRUE(tabModel->setActiveTabIndex(3));
  EXPECT_EQ(tabModel->activeSourceId(), "playlist://Chillout");
  EXPECT_EQ(tabModel->activeViewId(), "jukebox_default");

  // Verify Tab 3 layout overrides for jukebox mode
  QVariantMap jukeOverrides =
      viewRegistry->getPanelOverrides(tabModel->activeViewId());
  layoutModel->applyLayoutOverrides(jukeOverrides);
  EXPECT_EQ(layoutModel->effectiveLeftSidebarMode(), "hidden");

  // Switch to Tab 0: verify clearing overrides restores permanent left sidebar
  EXPECT_TRUE(tabModel->setActiveTabIndex(0));
  layoutModel->clearLayoutOverrides();
  EXPECT_EQ(layoutModel->effectiveLeftSidebarMode(), "permanent");
}

TEST_F(Phase6UiTest, LayoutPresetsApplication) {
  auto presets = layoutModel->getAvailablePresets();
  EXPECT_EQ(presets.size(), 5);

  QString recView;
  QObject::connect(layoutModel.get(), &LayoutModel::presetApplied,
                   [&recView](const QString & /*presetId*/,
                              const QString &recommendedViewId) {
                     recView = recommendedViewId;
                   });

  // Test Showcase
  layoutModel->applyPreset("showcase");
  EXPECT_EQ(layoutModel->currentPreset(), "showcase");
  EXPECT_EQ(recView, "covers_default");
  EXPECT_EQ(layoutModel->effectiveLeftSidebarMode(), "permanent");
  EXPECT_DOUBLE_EQ(layoutModel->leftSidebarWidth(), 200.0);
  EXPECT_FALSE(layoutModel->columnBrowserVisible());
  EXPECT_TRUE(layoutModel->jumpBarVisible());

  // Test Catalog
  layoutModel->applyPreset("catalog");
  EXPECT_EQ(layoutModel->currentPreset(), "catalog");
  EXPECT_EQ(recView, "album_and_tracks_default");
  EXPECT_TRUE(layoutModel->effectiveLeftMainPanelVisible());
  EXPECT_DOUBLE_EQ(layoutModel->leftMainPanelWidth(), 220.0);
  EXPECT_TRUE(layoutModel->columnBrowserVisible());

  // Test Stage
  layoutModel->applyPreset("stage");
  EXPECT_EQ(layoutModel->currentPreset(), "stage");
  EXPECT_EQ(recView, "now_playing_default");
  EXPECT_EQ(layoutModel->effectiveLeftSidebarMode(), "auto_open");
  EXPECT_FALSE(layoutModel->columnBrowserVisible());
  EXPECT_FALSE(layoutModel->jumpBarVisible());

  // Test Compact
  layoutModel->applyPreset("compact");
  EXPECT_EQ(layoutModel->currentPreset(), "compact");
  EXPECT_EQ(recView, "tracks_default");
  EXPECT_EQ(layoutModel->effectiveRightSidebarMode(), "hidden");
  EXPECT_DOUBLE_EQ(layoutModel->leftSidebarWidth(), 180.0);

  // Test Studio (Default)
  layoutModel->applyPreset("studio");
  EXPECT_EQ(layoutModel->currentPreset(), "studio");
  EXPECT_EQ(recView, "tracks_default");
  EXPECT_EQ(layoutModel->effectiveLeftSidebarMode(), "permanent");
  EXPECT_DOUBLE_EQ(layoutModel->leftSidebarWidth(), 240.0);
  EXPECT_DOUBLE_EQ(layoutModel->rightSidebarWidth(), 280.0);
  EXPECT_TRUE(layoutModel->columnBrowserVisible());
  EXPECT_TRUE(layoutModel->jumpBarVisible());
}
