#include "core/services/TabService.hpp"
#include <filesystem>
#include <gtest/gtest.h>

using namespace core;

TEST(TabServiceTest, DefaultInitialization) {
  TabService service;
  EXPECT_EQ(service.getTabCount(), 1u);
  EXPECT_EQ(service.getActiveTabIndex(), 0u);

  const TabState *active = service.getActiveTab();
  ASSERT_NE(active, nullptr);
  EXPECT_EQ(active->title, "Music");
  EXPECT_EQ(active->sourceId, "library://music");
  EXPECT_EQ(active->viewId, "tracks_default");
  EXPECT_EQ(active->lockMode, TabLockMode::Unlocked);
  EXPECT_FALSE(active->isPinned);
  EXPECT_EQ(active->subFilterState.jumpBarLetter, "All");
}

TEST(TabServiceTest, TabLifecycle) {
  TabService service;

  // Open tabs
  std::string tab2Id =
      service.openTab("library://inbox", "Inbox", "tracks_default");
  EXPECT_EQ(service.getTabCount(), 2u);
  EXPECT_EQ(service.getActiveTabIndex(), 1u);
  EXPECT_EQ(service.getActiveTab()->title, "Inbox");

  std::string tab3Id =
      service.openTab("folder:///media/flac", "Flac Archive", "covers_default");
  EXPECT_EQ(service.getTabCount(), 3u);
  EXPECT_EQ(service.getActiveTabIndex(), 2u);

  // Switch active tab
  EXPECT_TRUE(service.setActiveTabIndex(0));
  EXPECT_EQ(service.getActiveTab()->title, "Music");
  EXPECT_FALSE(service.setActiveTabIndex(99));

  // Duplicate tab
  std::string dupId = service.duplicateTab(1);
  EXPECT_FALSE(dupId.empty());
  EXPECT_EQ(service.getTabCount(), 4u);
  EXPECT_EQ(service.getActiveTabIndex(), 2u);
  EXPECT_EQ(service.getActiveTab()->title, "Inbox");
  EXPECT_NE(service.getActiveTab()->tabId, tab2Id);

  // Rename tab
  EXPECT_TRUE(service.renameTab(2, "Renamed Inbox"));
  EXPECT_EQ(service.getTab(2)->title, "Renamed Inbox");
  EXPECT_FALSE(service.renameTab(2, ""));

  // Move tab
  EXPECT_TRUE(service.moveTab(2, 0));
  EXPECT_EQ(service.getTab(0)->title, "Renamed Inbox");

  // Cannot close last remaining tab
  TabService singleTabService;
  EXPECT_FALSE(singleTabService.closeTab(0));
  EXPECT_EQ(singleTabService.getTabCount(), 1u);

  // Close tab
  size_t countBefore = service.getTabCount();
  EXPECT_TRUE(service.closeTab(0));
  EXPECT_EQ(service.getTabCount(), countBefore - 1);

  // Reopen closed tab (Ctrl+Shift+T)
  EXPECT_TRUE(service.canReopenClosedTab());
  auto reopenedId = service.reopenLastClosedTab();
  ASSERT_TRUE(reopenedId.has_value());
  EXPECT_EQ(service.getActiveTab()->title, "Renamed Inbox");
}

TEST(TabServiceTest, LockStateTransitionsAndPinning) {
  TabService service;
  service.openTab("folder:///music/rock", "Rock");
  service.openTab("folder:///music/jazz", "Jazz");

  // Pin Tab
  EXPECT_TRUE(service.setTabPinned(0, true));
  EXPECT_TRUE(service.getTab(0)->isPinned);
  // Pinned tab cannot be closed without force
  EXPECT_FALSE(service.closeTab(0, false));
  EXPECT_TRUE(service.closeTab(0, true)); // force close works

  // Lock Tab
  EXPECT_TRUE(service.setTabLockMode(0, TabLockMode::Locked));
  EXPECT_EQ(service.getTab(0)->lockMode, TabLockMode::Locked);
  // Locked tab cannot be closed without force
  EXPECT_FALSE(service.closeTab(0, false));

  // Root Lock
  EXPECT_TRUE(service.setTabRootLock(1, "folder:///music/jazz"));
  EXPECT_EQ(service.getTab(1)->lockMode, TabLockMode::RootLocked);
  EXPECT_EQ(service.getTab(1)->rootSourceId, "folder:///music/jazz");

  // Unlock Tab
  EXPECT_TRUE(service.unlockTab(0));
  EXPECT_EQ(service.getTab(0)->lockMode, TabLockMode::Unlocked);
}

TEST(TabServiceTest, NavigatorRoutingUnlockedTab) {
  TabService service;
  // Default tab is Unlocked
  auto res = service.routeNavigatorClick("library://podcasts", "Podcasts");
  EXPECT_EQ(res.type, NavigationResultType::CurrentTabUpdated);
  EXPECT_EQ(service.getActiveTab()->sourceId, "library://podcasts");
  EXPECT_EQ(service.getActiveTab()->title, "Podcasts");
}

TEST(TabServiceTest, NavigatorRoutingLockedTabRoutesToUnlocked) {
  TabService service;
  service.setTabLockMode(0, TabLockMode::Locked);

  // Active tab is locked, no unlocked tabs exist -> spawns new tab
  auto res1 = service.routeNavigatorClick("folder:///music/ambient", "Ambient");
  EXPECT_EQ(res1.type, NavigationResultType::NewTabSpawned);
  EXPECT_EQ(service.getTabCount(), 2u);
  EXPECT_EQ(service.getActiveTabIndex(), 1u);
  EXPECT_EQ(service.getActiveTab()->sourceId, "folder:///music/ambient");

  // Tab 0 is locked, Tab 1 is unlocked. Now focus Tab 0 again.
  service.setActiveTabIndex(0);
  EXPECT_EQ(service.getActiveTab()->lockMode, TabLockMode::Locked);

  // Click navigator again -> routes to existing unlocked Tab 1
  auto res2 = service.routeNavigatorClick("library://audiobooks", "Audiobooks");
  EXPECT_EQ(res2.type, NavigationResultType::NavigatedToUnlockedTab);
  EXPECT_EQ(res2.targetTabIndex, 1u);
  EXPECT_EQ(service.getActiveTabIndex(), 1u);
  EXPECT_EQ(service.getActiveTab()->sourceId, "library://audiobooks");
  EXPECT_EQ(service.getTabCount(), 2u); // No new tab was needed
}

TEST(TabServiceTest, NavigatorRoutingRootLockedTab) {
  TabService service;
  service.setTabRootLock(0, "folder:///media/lossless");

  // Drilldown into child folder -> allowed in-place
  auto resChild = service.routeNavigatorClick(
      "folder:///media/lossless/Classical/Bach", "Bach");
  EXPECT_EQ(resChild.type, NavigationResultType::CurrentTabUpdated);
  EXPECT_EQ(service.getActiveTab()->sourceId,
            "folder:///media/lossless/Classical/Bach");

  // Reset to root
  EXPECT_TRUE(service.resetTabToRoot(0));
  EXPECT_EQ(service.getActiveTab()->sourceId, "folder:///media/lossless");

  // Navigate outside root -> routes away or spawns new tab
  auto resExternal =
      service.routeNavigatorClick("folder:///other/mp3", "Other MP3");
  EXPECT_EQ(resExternal.type, NavigationResultType::NewTabSpawned);
  EXPECT_EQ(service.getActiveTabIndex(), 1u);
  EXPECT_EQ(service.getActiveTab()->sourceId, "folder:///other/mp3");
}

TEST(TabServiceTest, ForceNewTabRouting) {
  TabService service;
  auto res = service.routeNavigatorClick("playlist://rock_classics",
                                         "Rock Classics", true);
  EXPECT_EQ(res.type, NavigationResultType::NewTabSpawned);
  EXPECT_EQ(service.getTabCount(), 2u);
  EXPECT_EQ(service.getActiveTab()->sourceId, "playlist://rock_classics");
}

TEST(TabServiceTest, InTabMutations) {
  TabService service;

  EXPECT_TRUE(service.updateActiveView("album_and_tracks"));
  EXPECT_EQ(service.getActiveTab()->viewId, "album_and_tracks");

  TabSubFilterState subF;
  subF.jumpBarLetter = "D";
  subF.thumbnailSelection = "Deep Purple";
  subF.columnBrowserSelections["Genre"] = "Hard Rock";
  EXPECT_TRUE(service.updateActiveSubFilter(subF));
  EXPECT_EQ(service.getActiveTab()->subFilterState, subF);

  std::vector<TabSortColumn> sort = {{"year", true}, {"track_number", true}};
  EXPECT_TRUE(service.updateActiveSort(sort, "album_artist"));
  EXPECT_EQ(service.getActiveTab()->sortOrder, sort);
  EXPECT_EQ(service.getActiveTab()->groupBy, "album_artist");

  EXPECT_TRUE(service.updateActiveScrollOffset(350.5));
  EXPECT_DOUBLE_EQ(service.getActiveTab()->scrollOffset, 350.5);

  EXPECT_TRUE(service.updateActiveLayoutOverride("rightSidebar", "auto_open"));
  EXPECT_EQ(service.getActiveTab()->layoutOverrides["rightSidebar"],
            "auto_open");
  EXPECT_TRUE(service.clearActiveLayoutOverrides());
  EXPECT_TRUE(service.getActiveTab()->layoutOverrides.empty());
}

TEST(TabServiceTest, SerializationRoundTripAndFileIO) {
  TabService original;
  original.openTab("folder:///media/flac", "FLAC", "covers_default");
  original.setTabRootLock(1, "folder:///media/flac");
  original.updateActiveLayoutOverride("leftSidebar", "permanent");
  original.updateActiveScrollOffset(120.0);

  TabSubFilterState subF;
  subF.jumpBarLetter = "P";
  subF.thumbnailSelection = "Pink Floyd";
  subF.columnBrowserSelections["Genre"] = "Prog Rock";
  original.updateActiveSubFilter(subF);

  // Serialize to JSON
  std::string json = original.serializeToJson();
  EXPECT_FALSE(json.empty());

  // Deserialize into new service
  TabService restored;
  EXPECT_TRUE(restored.deserializeFromJson(json));
  EXPECT_EQ(restored.getTabCount(), 2u);
  EXPECT_EQ(restored.getActiveTabIndex(), 1u);

  const TabState *tab0 = restored.getTab(0);
  ASSERT_NE(tab0, nullptr);
  EXPECT_EQ(tab0->title, "Music");

  const TabState *tab1 = restored.getTab(1);
  ASSERT_NE(tab1, nullptr);
  EXPECT_EQ(tab1->title, "FLAC");
  EXPECT_EQ(tab1->sourceId, "folder:///media/flac");
  EXPECT_EQ(tab1->viewId, "covers_default");
  EXPECT_EQ(tab1->lockMode, TabLockMode::RootLocked);
  EXPECT_EQ(tab1->rootSourceId, "folder:///media/flac");
  EXPECT_EQ(tab1->subFilterState.jumpBarLetter, "P");
  EXPECT_EQ(tab1->subFilterState.thumbnailSelection, "Pink Floyd");
  EXPECT_EQ(tab1->subFilterState.columnBrowserSelections.at("Genre"),
            "Prog Rock");
  EXPECT_DOUBLE_EQ(tab1->scrollOffset, 120.0);
  EXPECT_EQ(tab1->layoutOverrides.at("leftSidebar"), "permanent");

  // Save to file and load from file
  std::filesystem::path tempPath =
      std::filesystem::temp_directory_path() / "picaudio_tabs_test.json";
  EXPECT_TRUE(original.saveToFile(tempPath.string()));

  TabService fileRestored;
  EXPECT_TRUE(fileRestored.loadFromFile(tempPath.string()));
  EXPECT_EQ(fileRestored.getTabCount(), 2u);
  EXPECT_EQ(fileRestored.getActiveTab()->title, "FLAC");

  std::filesystem::remove(tempPath);
}
