#pragma once

#include "core/entities/TabState.hpp"
#include <cstddef>
#include <deque>
#include <optional>
#include <string>
#include <vector>

namespace core {

/**
 * @brief Result outcome of a navigator routing action against tab locking
 * rules.
 */
enum class NavigationResultType {
  CurrentTabUpdated, ///< Active tab was unlocked or within root and updated
                     ///< in-place.
  NavigatedToUnlockedTab, ///< Active tab was locked; navigated to existing
                          ///< unlocked tab.
  NewTabSpawned, ///< Active tab was locked and no unlocked tab was available
                 ///< (or forced).
  Denied         ///< Navigation request rejected.
};

/**
 * @brief Navigation outcome data containing target tab index and ID.
 */
struct NavigationResult {
  NavigationResultType type{NavigationResultType::Denied};
  size_t targetTabIndex{0};
  std::string targetTabId;
};

/**
 * @brief Core domain service managing tab instances, navigation routing,
 * lock state transitions, and workspace session persistence.
 */
class TabService {
public:
  TabService();
  ~TabService() = default;

  // Tab collection access
  [[nodiscard]] const std::vector<TabState> &getTabs() const noexcept {
    return m_tabs;
  }
  [[nodiscard]] size_t getActiveTabIndex() const noexcept {
    return m_activeTabIndex;
  }
  [[nodiscard]] const TabState *getActiveTab() const noexcept;
  [[nodiscard]] TabState *getActiveTab() noexcept;
  [[nodiscard]] const TabState *getTab(size_t index) const noexcept;
  [[nodiscard]] TabState *getTab(size_t index) noexcept;
  [[nodiscard]] size_t getTabCount() const noexcept { return m_tabs.size(); }

  // Tab activation
  bool setActiveTabIndex(size_t index);

  // Tab lifecycle
  std::string openTab(const std::string &sourceId = "library://music",
                      const std::string &title = "Music",
                      const std::string &viewId = "tracks_default",
                      size_t insertIndex = static_cast<size_t>(-1));
  bool closeTab(size_t index, bool force = false);
  void closeOtherTabs(size_t keepIndex);
  std::string duplicateTab(size_t index);
  bool renameTab(size_t index, const std::string &newTitle);
  bool moveTab(size_t fromIndex, size_t toIndex);

  // Lock state transitions
  bool setTabLockMode(size_t index, TabLockMode mode);
  bool setTabPinned(size_t index, bool pinned);
  bool setTabRootLock(size_t index, const std::string &rootSourceId);
  bool unlockTab(size_t index);
  bool resetTabToRoot(size_t index);

  // Navigator navigation routing
  NavigationResult routeNavigatorClick(const std::string &targetSourceId,
                                       const std::string &defaultTitle,
                                       bool forceNewTab = false);

  // In-tab state mutations
  bool updateActiveSource(const std::string &sourceId,
                          const std::string &title = "");
  bool updateActiveView(const std::string &viewId);
  bool updateActiveSubFilter(const TabSubFilterState &filterState);
  bool updateActiveSort(const std::vector<TabSortColumn> &sortOrder,
                        const std::string &groupBy = "");
  bool updateActiveScrollOffset(double offset);
  bool updateActiveLayoutOverride(const std::string &panelId,
                                  const std::string &state);
  bool clearActiveLayoutOverrides();

  // Closed tab stack (Ctrl+Shift+T)
  [[nodiscard]] bool canReopenClosedTab() const noexcept {
    return !m_closedTabsHistory.empty();
  }
  std::optional<std::string> reopenLastClosedTab();

  // Serialization & Persistence
  [[nodiscard]] std::string serializeToJson() const;
  bool deserializeFromJson(const std::string &jsonStr);
  bool saveToFile(const std::string &filePath) const;
  bool loadFromFile(const std::string &filePath);

  // Reset to default fresh state
  void resetToDefault();

private:
  [[nodiscard]] std::string generateUniqueTabId();
  [[nodiscard]] bool isChildOfRoot(const std::string &sourceId,
                                   const std::string &rootSourceId) const;

  std::vector<TabState> m_tabs;
  size_t m_activeTabIndex{0};
  std::deque<TabState> m_closedTabsHistory;
  uint64_t m_idCounter{1};
};

} // namespace core
