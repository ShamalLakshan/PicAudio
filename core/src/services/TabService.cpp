#include "core/services/TabService.hpp"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace core {

namespace {

std::string escapeJsonString(const std::string &input) {
  std::string out;
  out.reserve(input.size() + 8);
  for (char c : input) {
    switch (c) {
    case '\"':
      out.append("\\\"");
      break;
    case '\\':
      out.append("\\\\");
      break;
    case '\b':
      out.append("\\b");
      break;
    case '\f':
      out.append("\\f");
      break;
    case '\n':
      out.append("\\n");
      break;
    case '\r':
      out.append("\\r");
      break;
    case '\t':
      out.append("\\t");
      break;
    default:
      out.push_back(c);
      break;
    }
  }
  return out;
}

// Lightweight JSON value parser for TabService persistence
struct SimpleJsonValue {
  enum class Type { Null, Bool, Number, String, Array, Object };
  Type type{Type::Null};
  bool boolVal{false};
  double numVal{0.0};
  std::string strVal;
  std::vector<SimpleJsonValue> arrVal;
  std::map<std::string, SimpleJsonValue> objVal;

  [[nodiscard]] const SimpleJsonValue &
  operator[](const std::string &key) const {
    static const SimpleJsonValue s_nullVal;
    if (type != Type::Object)
      return s_nullVal;
    auto it = objVal.find(key);
    return it != objVal.end() ? it->second : s_nullVal;
  }
};

class SimpleJsonParser {
public:
  explicit SimpleJsonParser(const std::string &src) : m_src(src), m_pos(0) {}

  bool parse(SimpleJsonValue &out) {
    skipWhitespace();
    if (m_pos >= m_src.size())
      return false;
    return parseValue(out);
  }

private:
  void skipWhitespace() {
    while (m_pos < m_src.size() &&
           (m_src[m_pos] == ' ' || m_src[m_pos] == '\t' ||
            m_src[m_pos] == '\r' || m_src[m_pos] == '\n')) {
      ++m_pos;
    }
  }

  bool parseValue(SimpleJsonValue &val) {
    skipWhitespace();
    if (m_pos >= m_src.size())
      return false;

    char c = m_src[m_pos];
    if (c == '{')
      return parseObject(val);
    if (c == '[')
      return parseArray(val);
    if (c == '\"')
      return parseString(val);
    if (c == 't' || c == 'f')
      return parseBool(val);
    if (c == 'n')
      return parseNull(val);
    if (c == '-' || (c >= '0' && c <= '9'))
      return parseNumber(val);

    return false;
  }

  bool parseObject(SimpleJsonValue &val) {
    val.type = SimpleJsonValue::Type::Object;
    val.objVal.clear();
    ++m_pos; // Skip '{'

    skipWhitespace();
    if (m_pos < m_src.size() && m_src[m_pos] == '}') {
      ++m_pos;
      return true;
    }

    while (m_pos < m_src.size()) {
      skipWhitespace();
      if (m_pos >= m_src.size() || m_src[m_pos] != '\"')
        return false;

      SimpleJsonValue keyVal;
      if (!parseString(keyVal))
        return false;

      skipWhitespace();
      if (m_pos >= m_src.size() || m_src[m_pos] != ':')
        return false;
      ++m_pos; // Skip ':'

      SimpleJsonValue subVal;
      if (!parseValue(subVal))
        return false;

      val.objVal[keyVal.strVal] = std::move(subVal);

      skipWhitespace();
      if (m_pos >= m_src.size())
        return false;
      if (m_src[m_pos] == '}') {
        ++m_pos;
        return true;
      }
      if (m_src[m_pos] == ',') {
        ++m_pos;
        continue;
      }
      return false;
    }
    return false;
  }

  bool parseArray(SimpleJsonValue &val) {
    val.type = SimpleJsonValue::Type::Array;
    val.arrVal.clear();
    ++m_pos; // Skip '['

    skipWhitespace();
    if (m_pos < m_src.size() && m_src[m_pos] == ']') {
      ++m_pos;
      return true;
    }

    while (m_pos < m_src.size()) {
      SimpleJsonValue item;
      if (!parseValue(item))
        return false;

      val.arrVal.push_back(std::move(item));

      skipWhitespace();
      if (m_pos >= m_src.size())
        return false;
      if (m_src[m_pos] == ']') {
        ++m_pos;
        return true;
      }
      if (m_src[m_pos] == ',') {
        ++m_pos;
        continue;
      }
      return false;
    }
    return false;
  }

  bool parseString(SimpleJsonValue &val) {
    val.type = SimpleJsonValue::Type::String;
    val.strVal.clear();
    ++m_pos; // Skip opening quote

    while (m_pos < m_src.size()) {
      char c = m_src[m_pos++];
      if (c == '\"')
        return true;
      if (c == '\\') {
        if (m_pos >= m_src.size())
          return false;
        char esc = m_src[m_pos++];
        switch (esc) {
        case '\"':
          val.strVal.push_back('\"');
          break;
        case '\\':
          val.strVal.push_back('\\');
          break;
        case '/':
          val.strVal.push_back('/');
          break;
        case 'b':
          val.strVal.push_back('\b');
          break;
        case 'f':
          val.strVal.push_back('\f');
          break;
        case 'n':
          val.strVal.push_back('\n');
          break;
        case 'r':
          val.strVal.push_back('\r');
          break;
        case 't':
          val.strVal.push_back('\t');
          break;
        default:
          val.strVal.push_back(esc);
          break;
        }
      } else {
        val.strVal.push_back(c);
      }
    }
    return false;
  }

  bool parseBool(SimpleJsonValue &val) {
    val.type = SimpleJsonValue::Type::Bool;
    if (m_src.compare(m_pos, 4, "true") == 0) {
      val.boolVal = true;
      m_pos += 4;
      return true;
    }
    if (m_src.compare(m_pos, 5, "false") == 0) {
      val.boolVal = false;
      m_pos += 5;
      return true;
    }
    return false;
  }

  bool parseNull(SimpleJsonValue &val) {
    val.type = SimpleJsonValue::Type::Null;
    if (m_src.compare(m_pos, 4, "null") == 0) {
      m_pos += 4;
      return true;
    }
    return false;
  }

  bool parseNumber(SimpleJsonValue &val) {
    val.type = SimpleJsonValue::Type::Number;
    size_t start = m_pos;
    if (m_src[m_pos] == '-')
      ++m_pos;
    while (m_pos < m_src.size() &&
           (std::isdigit(static_cast<unsigned char>(m_src[m_pos])) ||
            m_src[m_pos] == '.' || m_src[m_pos] == 'e' || m_src[m_pos] == 'E' ||
            m_src[m_pos] == '+' || m_src[m_pos] == '-')) {
      ++m_pos;
    }
    std::string numStr = m_src.substr(start, m_pos - start);
    try {
      val.numVal = std::stod(numStr);
      return true;
    } catch (...) {
      return false;
    }
  }

  const std::string &m_src;
  size_t m_pos{0};
};

} // namespace

TabService::TabService() { resetToDefault(); }

void TabService::resetToDefault() {
  m_tabs.clear();
  m_closedTabsHistory.clear();
  m_activeTabIndex = 0;
  m_idCounter = 1;

  TabState defaultTab;
  defaultTab.tabId = "tab_default";
  defaultTab.title = "Music";
  defaultTab.sourceId = "library://music";
  defaultTab.viewId = "tracks_default";
  defaultTab.lockMode = TabLockMode::Unlocked;
  defaultTab.isPinned = false;
  defaultTab.subFilterState.jumpBarLetter = "All";

  m_tabs.push_back(defaultTab);
}

const TabState *TabService::getActiveTab() const noexcept {
  if (m_activeTabIndex < m_tabs.size()) {
    return &m_tabs[m_activeTabIndex];
  }
  return nullptr;
}

TabState *TabService::getActiveTab() noexcept {
  if (m_activeTabIndex < m_tabs.size()) {
    return &m_tabs[m_activeTabIndex];
  }
  return nullptr;
}

const TabState *TabService::getTab(size_t index) const noexcept {
  if (index < m_tabs.size()) {
    return &m_tabs[index];
  }
  return nullptr;
}

TabState *TabService::getTab(size_t index) noexcept {
  if (index < m_tabs.size()) {
    return &m_tabs[index];
  }
  return nullptr;
}

bool TabService::setActiveTabIndex(size_t index) {
  if (index >= m_tabs.size()) {
    return false;
  }
  m_activeTabIndex = index;
  return true;
}

std::string TabService::openTab(const std::string &sourceId,
                                const std::string &title,
                                const std::string &viewId, size_t insertIndex) {
  TabState tab;
  tab.tabId = generateUniqueTabId();
  tab.sourceId = sourceId.empty() ? "library://music" : sourceId;
  tab.title = title.empty() ? "Music" : title;
  tab.viewId = viewId.empty() ? "tracks_default" : viewId;
  tab.lockMode = TabLockMode::Unlocked;
  tab.isPinned = false;
  tab.subFilterState.jumpBarLetter = "All";

  if (insertIndex >= m_tabs.size()) {
    m_tabs.push_back(tab);
    m_activeTabIndex = m_tabs.size() - 1;
  } else {
    m_tabs.insert(m_tabs.begin() + static_cast<ptrdiff_t>(insertIndex), tab);
    m_activeTabIndex = insertIndex;
  }

  return tab.tabId;
}

bool TabService::closeTab(size_t index, bool force) {
  if (index >= m_tabs.size() || m_tabs.size() <= 1) {
    return false;
  }

  const TabState &tab = m_tabs[index];
  if (!force && (tab.isPinned || tab.lockMode == TabLockMode::Locked)) {
    return false;
  }

  // Push to reopen history stack (limit to 20 entries)
  m_closedTabsHistory.push_front(tab);
  if (m_closedTabsHistory.size() > 20) {
    m_closedTabsHistory.pop_back();
  }

  m_tabs.erase(m_tabs.begin() + static_cast<ptrdiff_t>(index));

  // Adjust active tab index
  if (m_activeTabIndex >= m_tabs.size()) {
    m_activeTabIndex = m_tabs.size() - 1;
  } else if (index < m_activeTabIndex) {
    --m_activeTabIndex;
  }

  return true;
}

void TabService::closeOtherTabs(size_t keepIndex) {
  if (keepIndex >= m_tabs.size()) {
    return;
  }

  TabState keepTab = m_tabs[keepIndex];
  std::vector<TabState> remaining;
  remaining.reserve(m_tabs.size());

  for (size_t i = 0; i < m_tabs.size(); ++i) {
    if (i == keepIndex || m_tabs[i].isPinned ||
        m_tabs[i].lockMode == TabLockMode::Locked) {
      remaining.push_back(m_tabs[i]);
    } else {
      m_closedTabsHistory.push_front(m_tabs[i]);
    }
  }

  while (m_closedTabsHistory.size() > 20) {
    m_closedTabsHistory.pop_back();
  }

  m_tabs = std::move(remaining);

  // Find index of keepTab in new list
  m_activeTabIndex = 0;
  for (size_t i = 0; i < m_tabs.size(); ++i) {
    if (m_tabs[i].tabId == keepTab.tabId) {
      m_activeTabIndex = i;
      break;
    }
  }
}

std::string TabService::duplicateTab(size_t index) {
  if (index >= m_tabs.size()) {
    return "";
  }

  TabState dup = m_tabs[index];
  dup.tabId = generateUniqueTabId();
  dup.isPinned = false; // Duplicates do not inherit pinned state

  size_t insertPos = index + 1;
  m_tabs.insert(m_tabs.begin() + static_cast<ptrdiff_t>(insertPos), dup);
  m_activeTabIndex = insertPos;

  return dup.tabId;
}

bool TabService::renameTab(size_t index, const std::string &newTitle) {
  if (index >= m_tabs.size() || newTitle.empty()) {
    return false;
  }
  m_tabs[index].title = newTitle;
  return true;
}

bool TabService::moveTab(size_t fromIndex, size_t toIndex) {
  if (fromIndex >= m_tabs.size() || toIndex >= m_tabs.size() ||
      fromIndex == toIndex) {
    return false;
  }

  TabState moved = std::move(m_tabs[fromIndex]);
  m_tabs.erase(m_tabs.begin() + static_cast<ptrdiff_t>(fromIndex));
  m_tabs.insert(m_tabs.begin() + static_cast<ptrdiff_t>(toIndex),
                std::move(moved));

  if (m_activeTabIndex == fromIndex) {
    m_activeTabIndex = toIndex;
  } else if (fromIndex < m_activeTabIndex && toIndex >= m_activeTabIndex) {
    --m_activeTabIndex;
  } else if (fromIndex > m_activeTabIndex && toIndex <= m_activeTabIndex) {
    ++m_activeTabIndex;
  }

  return true;
}

bool TabService::setTabLockMode(size_t index, TabLockMode mode) {
  if (index >= m_tabs.size()) {
    return false;
  }
  m_tabs[index].lockMode = mode;
  return true;
}

bool TabService::setTabPinned(size_t index, bool pinned) {
  if (index >= m_tabs.size()) {
    return false;
  }
  m_tabs[index].isPinned = pinned;
  return true;
}

bool TabService::setTabRootLock(size_t index, const std::string &rootSourceId) {
  if (index >= m_tabs.size()) {
    return false;
  }
  m_tabs[index].lockMode = TabLockMode::RootLocked;
  m_tabs[index].rootSourceId =
      rootSourceId.empty() ? m_tabs[index].sourceId : rootSourceId;
  return true;
}

bool TabService::unlockTab(size_t index) {
  if (index >= m_tabs.size()) {
    return false;
  }
  m_tabs[index].lockMode = TabLockMode::Unlocked;
  m_tabs[index].isPinned = false;
  m_tabs[index].rootSourceId.clear();
  return true;
}

bool TabService::resetTabToRoot(size_t index) {
  if (index >= m_tabs.size()) {
    return false;
  }
  TabState &tab = m_tabs[index];
  if (tab.lockMode == TabLockMode::RootLocked && !tab.rootSourceId.empty()) {
    tab.sourceId = tab.rootSourceId;
    return true;
  }
  return false;
}

NavigationResult
TabService::routeNavigatorClick(const std::string &targetSourceId,
                                const std::string &defaultTitle,
                                bool forceNewTab) {
  NavigationResult result;

  if (forceNewTab || m_tabs.empty()) {
    std::string newId = openTab(targetSourceId, defaultTitle);
    result.type = NavigationResultType::NewTabSpawned;
    result.targetTabIndex = m_activeTabIndex;
    result.targetTabId = newId;
    return result;
  }

  TabState &active = m_tabs[m_activeTabIndex];

  if (active.lockMode == TabLockMode::Unlocked) {
    active.sourceId = targetSourceId;
    if (!defaultTitle.empty()) {
      active.title = defaultTitle;
    }
    result.type = NavigationResultType::CurrentTabUpdated;
    result.targetTabIndex = m_activeTabIndex;
    result.targetTabId = active.tabId;
    return result;
  }

  if (active.lockMode == TabLockMode::RootLocked) {
    if (isChildOfRoot(targetSourceId, active.rootSourceId)) {
      active.sourceId = targetSourceId;
      result.type = NavigationResultType::CurrentTabUpdated;
      result.targetTabIndex = m_activeTabIndex;
      result.targetTabId = active.tabId;
      return result;
    }
    // Outside root: fall through to route to another tab or spawn new
  }

  // Active tab is Locked or RootLocked navigation was outside root
  // Find first unlocked tab
  for (size_t i = 0; i < m_tabs.size(); ++i) {
    if (m_tabs[i].lockMode == TabLockMode::Unlocked && !m_tabs[i].isPinned) {
      m_activeTabIndex = i;
      m_tabs[i].sourceId = targetSourceId;
      if (!defaultTitle.empty()) {
        m_tabs[i].title = defaultTitle;
      }
      result.type = NavigationResultType::NavigatedToUnlockedTab;
      result.targetTabIndex = i;
      result.targetTabId = m_tabs[i].tabId;
      return result;
    }
  }

  // No unlocked tab available: spawn a new tab
  std::string newId = openTab(targetSourceId, defaultTitle);
  result.type = NavigationResultType::NewTabSpawned;
  result.targetTabIndex = m_activeTabIndex;
  result.targetTabId = newId;
  return result;
}

bool TabService::isChildOfRoot(const std::string &sourceId,
                               const std::string &rootSourceId) const {
  if (sourceId.empty() || rootSourceId.empty()) {
    return false;
  }
  if (sourceId == rootSourceId) {
    return true;
  }
  if (sourceId.rfind(rootSourceId, 0) == 0) {
    // Check path delimiter boundary
    if (rootSourceId.back() == '/' || rootSourceId.back() == '\\') {
      return true;
    }
    if (sourceId.size() > rootSourceId.size()) {
      char next = sourceId[rootSourceId.size()];
      return next == '/' || next == '\\';
    }
  }
  return false;
}

bool TabService::updateActiveSource(const std::string &sourceId,
                                    const std::string &title) {
  TabState *tab = getActiveTab();
  if (!tab)
    return false;
  tab->sourceId = sourceId;
  if (!title.empty()) {
    tab->title = title;
  }
  return true;
}

bool TabService::updateActiveView(const std::string &viewId) {
  TabState *tab = getActiveTab();
  if (!tab || viewId.empty())
    return false;
  tab->viewId = viewId;
  return true;
}

bool TabService::updateActiveSubFilter(const TabSubFilterState &filterState) {
  TabState *tab = getActiveTab();
  if (!tab)
    return false;
  tab->subFilterState = filterState;
  return true;
}

bool TabService::updateActiveSort(const std::vector<TabSortColumn> &sortOrder,
                                  const std::string &groupBy) {
  TabState *tab = getActiveTab();
  if (!tab)
    return false;
  tab->sortOrder = sortOrder;
  if (!groupBy.empty()) {
    tab->groupBy = groupBy;
  }
  return true;
}

bool TabService::updateActiveScrollOffset(double offset) {
  TabState *tab = getActiveTab();
  if (!tab)
    return false;
  tab->scrollOffset = offset;
  return true;
}

bool TabService::updateActiveLayoutOverride(const std::string &panelId,
                                            const std::string &state) {
  TabState *tab = getActiveTab();
  if (!tab || panelId.empty())
    return false;
  tab->layoutOverrides[panelId] = state;
  return true;
}

bool TabService::clearActiveLayoutOverrides() {
  TabState *tab = getActiveTab();
  if (!tab)
    return false;
  tab->layoutOverrides.clear();
  return true;
}

std::optional<std::string> TabService::reopenLastClosedTab() {
  if (m_closedTabsHistory.empty()) {
    return std::nullopt;
  }

  TabState restored = std::move(m_closedTabsHistory.front());
  m_closedTabsHistory.pop_front();

  std::string restoredId = restored.tabId;
  m_tabs.push_back(std::move(restored));
  m_activeTabIndex = m_tabs.size() - 1;

  return restoredId;
}

std::string TabService::generateUniqueTabId() {
  auto now = std::chrono::steady_clock::now().time_since_epoch();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
  std::stringstream ss;
  ss << "tab_" << ms << "_" << m_idCounter++;
  return ss.str();
}

std::string TabService::serializeToJson() const {
  std::stringstream ss;
  ss << "{\n";
  ss << "  \"activeTabIndex\": " << m_activeTabIndex << ",\n";
  ss << "  \"tabs\": [\n";

  for (size_t i = 0; i < m_tabs.size(); ++i) {
    const TabState &t = m_tabs[i];
    ss << "    {\n";
    ss << "      \"tabId\": \"" << escapeJsonString(t.tabId) << "\",\n";
    ss << "      \"title\": \"" << escapeJsonString(t.title) << "\",\n";
    ss << "      \"lockMode\": " << static_cast<int>(t.lockMode) << ",\n";
    ss << "      \"isPinned\": " << (t.isPinned ? "true" : "false") << ",\n";
    ss << "      \"rootSourceId\": \"" << escapeJsonString(t.rootSourceId)
       << "\",\n";
    ss << "      \"sourceId\": \"" << escapeJsonString(t.sourceId) << "\",\n";
    ss << "      \"viewId\": \"" << escapeJsonString(t.viewId) << "\",\n";
    ss << "      \"groupBy\": \"" << escapeJsonString(t.groupBy) << "\",\n";
    ss << "      \"scrollOffset\": " << t.scrollOffset << ",\n";

    // SubFilterState
    ss << "      \"subFilterState\": {\n";
    ss << "        \"jumpBarLetter\": \""
       << escapeJsonString(t.subFilterState.jumpBarLetter) << "\",\n";
    ss << "        \"thumbnailSelection\": \""
       << escapeJsonString(t.subFilterState.thumbnailSelection) << "\",\n";
    ss << "        \"columnBrowserSelections\": {";
    bool firstCol = true;
    for (const auto &[k, v] : t.subFilterState.columnBrowserSelections) {
      if (!firstCol)
        ss << ", ";
      ss << "\"" << escapeJsonString(k) << "\": \"" << escapeJsonString(v)
         << "\"";
      firstCol = false;
    }
    ss << "}\n";
    ss << "      },\n";

    // SortOrder
    ss << "      \"sortOrder\": [";
    for (size_t s = 0; s < t.sortOrder.size(); ++s) {
      if (s > 0)
        ss << ", ";
      ss << "{\"field\": \"" << escapeJsonString(t.sortOrder[s].field)
         << "\", \"ascending\": "
         << (t.sortOrder[s].ascending ? "true" : "false") << "}";
    }
    ss << "],\n";

    // LayoutOverrides
    ss << "      \"layoutOverrides\": {";
    bool firstOv = true;
    for (const auto &[k, v] : t.layoutOverrides) {
      if (!firstOv)
        ss << ", ";
      ss << "\"" << escapeJsonString(k) << "\": \"" << escapeJsonString(v)
         << "\"";
      firstOv = false;
    }
    ss << "}\n";

    ss << "    }" << (i + 1 < m_tabs.size() ? ",\n" : "\n");
  }

  ss << "  ]\n";
  ss << "}\n";
  return ss.str();
}

bool TabService::deserializeFromJson(const std::string &jsonStr) {
  if (jsonStr.empty()) {
    return false;
  }

  SimpleJsonValue root;
  SimpleJsonParser parser(jsonStr);
  if (!parser.parse(root) || root.type != SimpleJsonValue::Type::Object) {
    return false;
  }

  const SimpleJsonValue &tabsVal = root["tabs"];
  if (tabsVal.type != SimpleJsonValue::Type::Array || tabsVal.arrVal.empty()) {
    return false;
  }

  std::vector<TabState> loadedTabs;
  loadedTabs.reserve(tabsVal.arrVal.size());

  for (const auto &item : tabsVal.arrVal) {
    if (item.type != SimpleJsonValue::Type::Object)
      continue;

    TabState tab;
    tab.tabId = item["tabId"].strVal;
    tab.title = item["title"].strVal;
    tab.lockMode =
        static_cast<TabLockMode>(static_cast<int>(item["lockMode"].numVal));
    tab.isPinned = item["isPinned"].boolVal;
    tab.rootSourceId = item["rootSourceId"].strVal;
    tab.sourceId = item["sourceId"].strVal;
    tab.viewId = item["viewId"].strVal;
    tab.groupBy = item["groupBy"].strVal;
    tab.scrollOffset = item["scrollOffset"].numVal;

    // SubFilterState
    const SimpleJsonValue &subF = item["subFilterState"];
    if (subF.type == SimpleJsonValue::Type::Object) {
      tab.subFilterState.jumpBarLetter = subF["jumpBarLetter"].strVal;
      tab.subFilterState.thumbnailSelection = subF["thumbnailSelection"].strVal;
      const SimpleJsonValue &cols = subF["columnBrowserSelections"];
      if (cols.type == SimpleJsonValue::Type::Object) {
        for (const auto &[k, v] : cols.objVal) {
          tab.subFilterState.columnBrowserSelections[k] = v.strVal;
        }
      }
    }

    // SortOrder
    const SimpleJsonValue &sortArr = item["sortOrder"];
    if (sortArr.type == SimpleJsonValue::Type::Array) {
      for (const auto &sItem : sortArr.arrVal) {
        if (sItem.type == SimpleJsonValue::Type::Object) {
          TabSortColumn sc;
          sc.field = sItem["field"].strVal;
          sc.ascending = sItem["ascending"].boolVal;
          tab.sortOrder.push_back(sc);
        }
      }
    }

    // LayoutOverrides
    const SimpleJsonValue &loObj = item["layoutOverrides"];
    if (loObj.type == SimpleJsonValue::Type::Object) {
      for (const auto &[k, v] : loObj.objVal) {
        tab.layoutOverrides[k] = v.strVal;
      }
    }

    if (tab.tabId.empty()) {
      tab.tabId = generateUniqueTabId();
    }
    if (tab.title.empty()) {
      tab.title = "Music";
    }

    loadedTabs.push_back(std::move(tab));
  }

  if (loadedTabs.empty()) {
    return false;
  }

  m_tabs = std::move(loadedTabs);
  size_t activeIdx = static_cast<size_t>(root["activeTabIndex"].numVal);
  m_activeTabIndex = (activeIdx < m_tabs.size()) ? activeIdx : 0;

  return true;
}

bool TabService::saveToFile(const std::string &filePath) const {
  std::filesystem::path p(filePath);
  if (p.has_parent_path()) {
    std::filesystem::create_directories(p.parent_path());
  }

  std::ofstream out(p, std::ios::out | std::ios::trunc);
  if (!out.is_open()) {
    return false;
  }

  out << serializeToJson();
  return out.good();
}

bool TabService::loadFromFile(const std::string &filePath) {
  std::filesystem::path p(filePath);
  if (!std::filesystem::exists(p)) {
    return false;
  }

  std::ifstream in(p, std::ios::in);
  if (!in.is_open()) {
    return false;
  }

  std::stringstream ss;
  ss << in.rdbuf();
  return deserializeFromJson(ss.str());
}

} // namespace core
