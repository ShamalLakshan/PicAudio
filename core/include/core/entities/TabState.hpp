#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace core {

/**
 * @brief Lock state for a tab controlling navigation behavior.
 */
enum class TabLockMode {
  Unlocked = 0, ///< Dynamic target; Navigator clicks update sourceId and title.
  Locked = 1,   ///< Frozen source; Navigator clicks route to an unlocked tab or
                ///< open a new tab.
  RootLocked = 2 ///< Restricted to rootSourceId; subfolder navigation allowed,
                 ///< external clicks route away.
};

/**
 * @brief Single column sort specification.
 */
struct TabSortColumn {
  std::string field;
  bool ascending{true};

  bool operator==(const TabSortColumn &other) const {
    return field == other.field && ascending == other.ascending;
  }
};

/**
 * @brief Sub-filter state for in-tab secondary browsers.
 */
struct TabSubFilterState {
  std::string jumpBarLetter{"All"}; ///< Selected A-Z jump bar letter
  std::string thumbnailSelection;   ///< Selected item in Thumbnail Browser
  std::map<std::string, std::string>
      columnBrowserSelections; ///< Cascading column key -> selected value

  bool operator==(const TabSubFilterState &other) const {
    return jumpBarLetter == other.jumpBarLetter &&
           thumbnailSelection == other.thumbnailSelection &&
           columnBrowserSelections == other.columnBrowserSelections;
  }
};

/**
 * @brief Represents an isolated tab workspace instance.
 */
struct TabState {
  std::string tabId;                           ///< Unique identifier
  std::string title{"Music"};                  ///< Display label
  TabLockMode lockMode{TabLockMode::Unlocked}; ///< Lock mode
  bool isPinned{false};     ///< True if pinned (compact icon/fixed label)
  std::string rootSourceId; ///< Locked root URI for root-locked navigation
  std::string sourceId{"library://music"}; ///< Bound Data Source identifier
  std::string viewId{"tracks_default"};    ///< Active View Definition key
  TabSubFilterState subFilterState;        ///< Secondary browser sub-filters
  std::vector<TabSortColumn> sortOrder;    ///< Multi-column sort hierarchy
  std::string groupBy; ///< Grouping header field (e.g. "album_artist")
  std::map<std::string, std::string>
      layoutOverrides;      ///< panelId -> "visible" | "hidden" | "auto_open"
  double scrollOffset{0.0}; ///< Preserved scroll offset in main viewport

  bool operator==(const TabState &other) const {
    return tabId == other.tabId && title == other.title &&
           lockMode == other.lockMode && isPinned == other.isPinned &&
           rootSourceId == other.rootSourceId && sourceId == other.sourceId &&
           viewId == other.viewId && subFilterState == other.subFilterState &&
           sortOrder == other.sortOrder && groupBy == other.groupBy &&
           layoutOverrides == other.layoutOverrides &&
           scrollOffset == other.scrollOffset;
  }
};

} // namespace core
