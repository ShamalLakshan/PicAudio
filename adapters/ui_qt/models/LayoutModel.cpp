#include "LayoutModel.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

LayoutModel::LayoutModel(QObject *parent) : QObject(parent) {
  registerAllModularElements();
  if (!loadLayoutProfile("default")) {
    resetToDefaultLayout();
  }
}

void LayoutModel::registerAllModularElements() {
  m_allElements.clear();

  auto reg = [this](const QString &id, const QString &name,
                    const QString &defZone) {
    LayoutElement el;
    el.elementId = id;
    el.name = name;
    el.defaultZoneId = defZone;
    el.isStacked = false;
    el.stackMode = "sub_tabs";
    el.activeStackIndex = 0;
    el.splitWeight = 1.0;
    m_allElements[id] = el;
  };

  reg("player_controls", "Player Transport Controls", "bottomPanel");
  reg("tabs_bar", "Tabs Bar Strip", "headerBar");
  reg("search_box", "Search Box", "headerBar");
  reg("command_toolbar", "Command Actions Toolbar", "headerBar");
  reg("wavebar", "Waveform Seekbar", "topPanel");
  reg("spectrum_visualizer", "Spectrum Visualizer", "topPanel");
  reg("now_playing_bar", "Now Playing Bar", "topPanel");
  reg("navigator", "Library Navigator Tree", "leftSidebar");
  reg("album_cover", "Track Album Artwork", "rightSidebar");
  reg("track_info", "Track Information & Tags", "rightSidebar");
  reg("playing_tracks", "Playback Queue", "rightSidebar");
  reg("thumbnail_browser", "Thumbnail Browser", "leftMainPanel");
  reg("library_explorer", "Library Explorer Tree", "leftMainPanel");
  reg("artist_bio", "Artist Biography", "rightMainPanel");
  reg("artist_picture", "Artist Photographic Portrait", "rightMainPanel");
  reg("lyrics", "Synchronized Lyrics", "rightMainPanel");
  reg("sidebar_player", "Sidebar Compact Player", "rightSidebar");
  reg("status_bar", "Status Bar", "bottomPanel");
  reg("jump_bar", "A-Z Alphabetical Jump Bar", "mainPanel");
  reg("column_browser", "Cascading Column Browser", "mainPanel");
  reg("main_files_view", "Main Files Workspace View", "mainPanel");
}

void LayoutModel::initializeDefaultZones() {
  m_zones.clear();

  LayoutZone header;
  header.zoneId = "headerBar";
  header.name = "Header Bar";
  header.mode = "visible";
  header.dimension = 44.0;
  header.elements.append(m_allElements["tabs_bar"]);
  header.elements.append(m_allElements["search_box"]);
  header.elements.append(m_allElements["command_toolbar"]);
  m_zones["headerBar"] = header;

  LayoutZone top;
  top.zoneId = "topPanel";
  top.name = "Top Panel";
  top.mode = "hidden";
  top.dimension = 64.0;
  m_zones["topPanel"] = top;

  LayoutZone leftSide;
  leftSide.zoneId = "leftSidebar";
  leftSide.name = "Left Sidebar";
  leftSide.mode = "permanent";
  leftSide.dimension = 240.0;
  leftSide.elements.append(m_allElements["navigator"]);
  m_zones["leftSidebar"] = leftSide;

  LayoutZone leftMain;
  leftMain.zoneId = "leftMainPanel";
  leftMain.name = "Left Main Panel";
  leftMain.mode = "hidden";
  leftMain.dimension = 210.0;
  leftMain.elements.append(m_allElements["thumbnail_browser"]);
  m_zones["leftMainPanel"] = leftMain;

  LayoutZone main;
  main.zoneId = "mainPanel";
  main.name = "Main Panel";
  main.mode = "visible";
  main.dimension = 0.0; // Dynamic fill
  main.elements.append(m_allElements["main_files_view"]);
  main.elements.append(m_allElements["column_browser"]);
  main.elements.append(m_allElements["jump_bar"]);
  m_zones["mainPanel"] = main;

  LayoutZone rightMain;
  rightMain.zoneId = "rightMainPanel";
  rightMain.name = "Right Main Panel";
  rightMain.mode = "hidden";
  rightMain.dimension = 240.0;
  rightMain.elements.append(m_allElements["lyrics"]);
  m_zones["rightMainPanel"] = rightMain;

  LayoutZone rightSide;
  rightSide.zoneId = "rightSidebar";
  rightSide.name = "Right Sidebar";
  rightSide.mode = "permanent";
  rightSide.dimension = 280.0;
  LayoutElement pt = m_allElements["playing_tracks"];
  pt.isStacked = true;
  pt.stackMode = "sub_tabs";
  LayoutElement ti = m_allElements["track_info"];
  ti.isStacked = true;
  ti.stackMode = "sub_tabs";
  LayoutElement ac = m_allElements["album_cover"];
  ac.isStacked = true;
  ac.stackMode = "sub_tabs";
  rightSide.elements.append(pt);
  rightSide.elements.append(ti);
  rightSide.elements.append(ac);
  m_zones["rightSidebar"] = rightSide;

  LayoutZone bottom;
  bottom.zoneId = "bottomPanel";
  bottom.name = "Bottom Panel";
  bottom.mode = "visible";
  bottom.dimension = 94.0;
  bottom.elements.append(m_allElements["player_controls"]);
  bottom.elements.append(m_allElements["status_bar"]);
  m_zones["bottomPanel"] = bottom;

  LayoutZone floating;
  floating.zoneId = "floatingWindow";
  floating.name = "Floating Window";
  floating.mode = "hidden";
  floating.dimension = 400.0;
  m_zones["floatingWindow"] = floating;
}

LayoutZone *LayoutModel::findZone(const QString &zoneId) {
  auto it = m_zones.find(zoneId);
  return (it != m_zones.end()) ? &it.value() : nullptr;
}

const LayoutZone *LayoutModel::findZone(const QString &zoneId) const {
  auto it = m_zones.find(zoneId);
  return (it != m_zones.end()) ? &it.value() : nullptr;
}

LayoutElement *LayoutModel::findElement(const QString &elementId) {
  for (auto &zone : m_zones) {
    for (auto &el : zone.elements) {
      if (el.elementId == elementId) {
        return &el;
      }
    }
  }
  return nullptr;
}

QString LayoutModel::effectiveLeftSidebarMode() const {
  if (m_activeOverrides.contains("leftSidebar")) {
    return m_activeOverrides.value("leftSidebar").toString();
  }
  const auto *z = findZone("leftSidebar");
  return z ? z->mode : "permanent";
}

bool LayoutModel::effectiveLeftMainPanelVisible() const {
  if (m_activeOverrides.contains("leftMainPanel")) {
    QString ov = m_activeOverrides.value("leftMainPanel").toString();
    return ov == "visible" || ov == "permanent";
  }
  const auto *z = findZone("leftMainPanel");
  return z && (z->mode == "visible" || z->mode == "permanent") &&
         !z->elements.isEmpty();
}

bool LayoutModel::effectiveRightMainPanelVisible() const {
  if (m_activeOverrides.contains("rightMainPanel")) {
    QString ov = m_activeOverrides.value("rightMainPanel").toString();
    return ov == "visible" || ov == "permanent";
  }
  const auto *z = findZone("rightMainPanel");
  return z && (z->mode == "visible" || z->mode == "permanent") &&
         !z->elements.isEmpty();
}

QString LayoutModel::effectiveRightSidebarMode() const {
  if (m_activeOverrides.contains("rightSidebar")) {
    return m_activeOverrides.value("rightSidebar").toString();
  }
  const auto *z = findZone("rightSidebar");
  return z ? z->mode : "permanent";
}

bool LayoutModel::effectiveTopPanelVisible() const {
  if (m_activeOverrides.contains("topPanel")) {
    QString ov = m_activeOverrides.value("topPanel").toString();
    return ov == "visible" || ov == "permanent";
  }
  const auto *z = findZone("topPanel");
  return z && (z->mode == "visible" || z->mode == "permanent") &&
         !z->elements.isEmpty();
}

bool LayoutModel::effectiveBottomPanelVisible() const {
  if (m_activeOverrides.contains("bottomPanel")) {
    QString ov = m_activeOverrides.value("bottomPanel").toString();
    return ov == "visible" || ov == "permanent";
  }
  const auto *z = findZone("bottomPanel");
  return z && (z->mode == "visible" || z->mode == "permanent") &&
         !z->elements.isEmpty();
}

bool LayoutModel::effectiveHeaderBarVisible() const {
  const auto *z = findZone("headerBar");
  return z && z->mode != "hidden";
}

bool LayoutModel::isFloatingWindowActive() const {
  const auto *z = findZone("floatingWindow");
  return z && !z->elements.isEmpty();
}

double LayoutModel::leftSidebarWidth() const {
  const auto *z = findZone("leftSidebar");
  return z ? z->dimension : 240.0;
}

double LayoutModel::leftMainPanelWidth() const {
  const auto *z = findZone("leftMainPanel");
  return z ? z->dimension : 210.0;
}

double LayoutModel::rightMainPanelWidth() const {
  const auto *z = findZone("rightMainPanel");
  return z ? z->dimension : 240.0;
}

double LayoutModel::rightSidebarWidth() const {
  const auto *z = findZone("rightSidebar");
  return z ? z->dimension : 280.0;
}

double LayoutModel::topPanelHeight() const {
  const auto *z = findZone("topPanel");
  return z ? z->dimension : 64.0;
}

double LayoutModel::bottomPanelHeight() const {
  const auto *z = findZone("bottomPanel");
  return z ? z->dimension : 94.0;
}

bool LayoutModel::columnBrowserVisible() const {
  if (m_activeOverrides.contains("columnBrowser")) {
    QString ov = m_activeOverrides.value("columnBrowser").toString();
    return ov == "visible" || ov == "true";
  }
  return hasElement("mainPanel", "column_browser");
}

QString LayoutModel::currentPreset() const { return m_currentPreset; }

void LayoutModel::setColumnBrowserVisible(bool visible) {
  if (visible) {
    if (!hasElement("mainPanel", "column_browser")) {
      assignElement("column_browser", "mainPanel");
    }
    m_activeOverrides["columnBrowser"] = "visible";
  } else {
    removeElement("column_browser");
    m_activeOverrides["columnBrowser"] = "hidden";
  }
  emit layoutChanged();
}

bool LayoutModel::jumpBarVisible() const {
  if (m_activeOverrides.contains("jumpBar")) {
    QString ov = m_activeOverrides.value("jumpBar").toString();
    return ov == "visible" || ov == "true";
  }
  return hasElement("mainPanel", "jump_bar");
}

void LayoutModel::setJumpBarVisible(bool visible) {
  if (visible) {
    if (!hasElement("mainPanel", "jump_bar")) {
      assignElement("jump_bar", "mainPanel");
    }
    m_activeOverrides["jumpBar"] = "visible";
  } else {
    removeElement("jump_bar");
    m_activeOverrides["jumpBar"] = "hidden";
  }
  emit layoutChanged();
}

void LayoutModel::setLeftSidebarWidth(double width) {
  setZoneDimension("leftSidebar", width);
}

void LayoutModel::setLeftMainPanelWidth(double width) {
  setZoneDimension("leftMainPanel", width);
}

void LayoutModel::setRightMainPanelWidth(double width) {
  setZoneDimension("rightMainPanel", width);
}

void LayoutModel::setRightSidebarWidth(double width) {
  setZoneDimension("rightSidebar", width);
}

void LayoutModel::setTopPanelHeight(double height) {
  setZoneDimension("topPanel", height);
}

void LayoutModel::setBottomPanelHeight(double height) {
  setZoneDimension("bottomPanel", height);
}

QVariantList LayoutModel::getZones() const {
  QVariantList list;
  for (const auto &z : m_zones) {
    QVariantMap map;
    map["zoneId"] = z.zoneId;
    map["name"] = z.name;
    map["mode"] = z.mode;
    map["dimension"] = z.dimension;
    map["elementCount"] = z.elements.size();
    list.append(map);
  }
  return list;
}

QVariantList LayoutModel::getElementsForZone(const QString &zoneId) const {
  QVariantList list;
  const auto *z = findZone(zoneId);
  if (!z)
    return list;

  for (int i = 0; i < z->elements.size(); ++i) {
    const auto &el = z->elements[i];
    QVariantMap map;
    map["elementId"] = el.elementId;
    map["name"] = el.name;
    map["isStacked"] = el.isStacked;
    map["stackMode"] = el.stackMode;
    map["activeStackIndex"] = el.activeStackIndex;
    map["splitWeight"] = el.splitWeight;
    map["zoneIndex"] = i;
    list.append(map);
  }
  return list;
}

QVariantList LayoutModel::getAllAvailableElements() const {
  QVariantList list;
  for (const auto &el : m_allElements) {
    QVariantMap map;
    map["elementId"] = el.elementId;
    map["name"] = el.name;
    map["defaultZoneId"] = el.defaultZoneId;

    // Determine current assigned zone
    QString currentZone = "";
    for (const auto &z : m_zones) {
      for (const auto &zel : z.elements) {
        if (zel.elementId == el.elementId) {
          currentZone = z.zoneId;
          break;
        }
      }
      if (!currentZone.isEmpty())
        break;
    }
    map["currentZoneId"] = currentZone;
    list.append(map);
  }
  return list;
}

bool LayoutModel::hasElement(const QString &zoneId,
                             const QString &elementId) const {
  const auto *z = findZone(zoneId);
  if (!z)
    return false;
  for (const auto &el : z->elements) {
    if (el.elementId == elementId)
      return true;
  }
  return false;
}

void LayoutModel::assignElement(const QString &elementId, const QString &zoneId,
                                int index, bool stacked) {
  removeElement(elementId); // First remove from any existing zone

  auto *z = findZone(zoneId);
  if (!z)
    return;

  LayoutElement el;
  if (m_allElements.contains(elementId)) {
    el = m_allElements[elementId];
  } else {
    el.elementId = elementId;
    el.name = elementId;
  }
  el.isStacked = stacked;

  if (index >= 0 && index < z->elements.size()) {
    z->elements.insert(index, el);
  } else {
    z->elements.append(el);
  }

  emit layoutChanged();
}

void LayoutModel::removeElement(const QString &elementId) {
  for (auto &z : m_zones) {
    for (int i = 0; i < z.elements.size(); ++i) {
      if (z.elements[i].elementId == elementId) {
        z.elements.removeAt(i);
        emit layoutChanged();
        return;
      }
    }
  }
}

void LayoutModel::moveElement(const QString &elementId,
                              const QString &targetZoneId, int targetIndex) {
  LayoutElement el;
  bool found = false;

  for (auto &z : m_zones) {
    for (int i = 0; i < z.elements.size(); ++i) {
      if (z.elements[i].elementId == elementId) {
        el = z.elements.takeAt(i);
        found = true;
        break;
      }
    }
    if (found)
      break;
  }

  if (!found)
    return;

  auto *targetZ = findZone(targetZoneId);
  if (!targetZ)
    return;

  if (targetIndex >= 0 && targetIndex < targetZ->elements.size()) {
    targetZ->elements.insert(targetIndex, el);
  } else {
    targetZ->elements.append(el);
  }

  emit layoutChanged();
}

void LayoutModel::setElementStacked(const QString &elementId, bool stacked,
                                    const QString &stackMode) {
  auto *el = findElement(elementId);
  if (!el)
    return;
  el->isStacked = stacked;
  if (!stackMode.isEmpty()) {
    el->stackMode = stackMode;
  }
  emit layoutChanged();
}

void LayoutModel::setZoneMode(const QString &zoneId, const QString &mode) {
  auto *z = findZone(zoneId);
  if (!z || z->mode == mode)
    return;
  z->mode = mode;
  emit layoutChanged();
}

QString LayoutModel::getZoneMode(const QString &zoneId) const {
  const auto *z = findZone(zoneId);
  return z ? z->mode : "";
}

void LayoutModel::setZoneDimension(const QString &zoneId, double dimension) {
  auto *z = findZone(zoneId);
  if (!z || qAbs(z->dimension - dimension) < 0.5)
    return;
  z->dimension = dimension;
  emit layoutChanged();
}

double LayoutModel::getZoneDimension(const QString &zoneId,
                                     double defaultDim) const {
  const auto *z = findZone(zoneId);
  return z ? z->dimension : defaultDim;
}

void LayoutModel::setElementActiveStackIndex(const QString &elementId,
                                             int stackIndex) {
  auto *el = findElement(elementId);
  if (!el || el->activeStackIndex == stackIndex)
    return;
  el->activeStackIndex = stackIndex;
  emit layoutChanged();
}

int LayoutModel::getElementActiveStackIndex(const QString &elementId) const {
  for (const auto &z : m_zones) {
    for (const auto &el : z.elements) {
      if (el.elementId == elementId) {
        return el.activeStackIndex;
      }
    }
  }
  return 0;
}

void LayoutModel::applyLayoutOverrides(const QVariantMap &overrides) {
  m_activeOverrides = overrides;
  emit overridesApplied();
  emit layoutChanged();
}

void LayoutModel::clearLayoutOverrides() {
  m_activeOverrides.clear();
  emit overridesApplied();
  emit layoutChanged();
}

void LayoutModel::resetToDefaultLayout() {
  initializeDefaultZones();
  m_activeOverrides.clear();
  emit layoutChanged();
}

void LayoutModel::saveLayoutProfile(const QString &profileName) {
  QSettings s("PicAudio", "PicAudio");
  QString prefix = QString("layout_%1/").arg(profileName);

  for (const auto &z : m_zones) {
    s.setValue(prefix + z.zoneId + "_mode", z.mode);
    s.setValue(prefix + z.zoneId + "_dimension", z.dimension);

    QJsonArray arr;
    for (const auto &el : z.elements) {
      QJsonObject obj;
      obj["elementId"] = el.elementId;
      obj["isStacked"] = el.isStacked;
      obj["stackMode"] = el.stackMode;
      obj["activeStackIndex"] = el.activeStackIndex;
      obj["splitWeight"] = el.splitWeight;
      arr.append(obj);
    }
    s.setValue(prefix + z.zoneId + "_elements",
               QJsonDocument(arr).toJson(QJsonDocument::Compact));
  }
}

bool LayoutModel::loadLayoutProfile(const QString &profileName) {
  QSettings s("PicAudio", "PicAudio");
  QString prefix = QString("layout_%1/").arg(profileName);

  if (!s.contains(prefix + "headerBar_mode") &&
      !s.contains(prefix + "mainPanel_mode")) {
    return false;
  }

  initializeDefaultZones();

  for (auto &z : m_zones) {
    if (s.contains(prefix + z.zoneId + "_mode")) {
      z.mode = s.value(prefix + z.zoneId + "_mode").toString();
    }
    if (s.contains(prefix + z.zoneId + "_dimension")) {
      z.dimension = s.value(prefix + z.zoneId + "_dimension").toDouble();
    }
    if (s.contains(prefix + z.zoneId + "_elements")) {
      QByteArray data = s.value(prefix + z.zoneId + "_elements").toByteArray();
      QJsonDocument doc = QJsonDocument::fromJson(data);
      if (doc.isArray()) {
        z.elements.clear();
        for (const auto &val : doc.array()) {
          QJsonObject obj = val.toObject();
          QString elId = obj["elementId"].toString();
          LayoutElement el;
          if (m_allElements.contains(elId)) {
            el = m_allElements[elId];
          } else {
            el.elementId = elId;
            el.name = elId;
          }
          el.isStacked = obj["isStacked"].toBool();
          el.stackMode = obj["stackMode"].toString("sub_tabs");
          el.activeStackIndex = obj["activeStackIndex"].toInt();
          el.splitWeight = obj["splitWeight"].toDouble(1.0);
          z.elements.append(el);
        }
      }
    }
  }

  emit layoutChanged();
  return true;
}

QVariantList LayoutModel::getAvailablePresets() const {
  QVariantList list;

  QVariantMap p1;
  p1["id"] = "studio";
  p1["name"] = "Studio";
  p1["description"] =
      "Balanced pro workspace with navigation sidebar, cascading filters, "
      "track table, and lyrics/queue side inspector.";
  p1["icon"] = "table";
  p1["recommendedViewId"] = "tracks_default";
  list.append(p1);

  QVariantMap p2;
  p2["id"] = "showcase";
  p2["name"] = "Showcase";
  p2["description"] = "Artwork-first presentation with album covers grid, "
                      "inline track drawer, and large metadata inspector.";
  p2["icon"] = "grid";
  p2["recommendedViewId"] = "covers_default";
  list.append(p2);

  QVariantMap p3;
  p3["id"] = "catalog";
  p3["name"] = "Catalog";
  p3["description"] = "Deep library browsing with split navigator, thumbnail "
                      "browser, 3-column tag hierarchy, and album tracklists.";
  p3["icon"] = "split";
  p3["recommendedViewId"] = "album_and_tracks_default";
  list.append(p3);

  QVariantMap p4;
  p4["id"] = "stage";
  p4["name"] = "Stage";
  p4["description"] =
      "Immersive playback mode centered on high-resolution artwork, "
      "synchronized lyrics, and real-time spectrum analysis.";
  p4["icon"] = "now_playing";
  p4["recommendedViewId"] = "now_playing_default";
  list.append(p4);

  QVariantMap p5;
  p5["id"] = "compact";
  p5["name"] = "Streamlined";
  p5["description"] = "Clean and distraction-free layout with slim navigation "
                      "and maximum screen space dedicated to your music list.";
  p5["icon"] = "compact";
  p5["recommendedViewId"] = "tracks_default";
  list.append(p5);

  return list;
}

void LayoutModel::applyPreset(const QString &presetId) {
  QString id = presetId.toLower().trimmed();
  if (id.isEmpty())
    id = "studio";

  m_currentPreset = id;
  m_activeOverrides.clear();

  if (id == "showcase") {
    m_zones["leftSidebar"].mode = "permanent";
    m_zones["leftSidebar"].dimension = 200.0;
    m_zones["leftSidebar"].elements.clear();
    m_zones["leftSidebar"].elements.append(m_allElements["navigator"]);

    m_zones["leftMainPanel"].mode = "hidden";

    m_zones["mainPanel"].mode = "visible";
    m_zones["mainPanel"].elements.clear();
    m_zones["mainPanel"].elements.append(m_allElements["main_files_view"]);
    m_zones["mainPanel"].elements.append(m_allElements["jump_bar"]);
    m_activeOverrides["columnBrowser"] = "hidden";
    m_activeOverrides["jumpBar"] = "visible";

    m_zones["rightMainPanel"].mode = "hidden";

    m_zones["rightSidebar"].mode = "permanent";
    m_zones["rightSidebar"].dimension = 280.0;
    m_zones["rightSidebar"].elements.clear();
    auto elArt = m_allElements["album_cover"];
    elArt.isStacked = true;
    elArt.stackMode = "sub_tabs";
    auto elInfo = m_allElements["track_info"];
    elInfo.isStacked = true;
    elInfo.stackMode = "sub_tabs";
    m_zones["rightSidebar"].elements.append(elArt);
    m_zones["rightSidebar"].elements.append(elInfo);

    m_zones["topPanel"].mode = "hidden";
    m_zones["bottomPanel"].mode = "visible";
    m_zones["bottomPanel"].dimension = 72.0;

    emit presetApplied("showcase", "covers_default");
  } else if (id == "catalog") {
    m_zones["leftSidebar"].mode = "permanent";
    m_zones["leftSidebar"].dimension = 220.0;
    m_zones["leftSidebar"].elements.clear();
    m_zones["leftSidebar"].elements.append(m_allElements["navigator"]);

    m_zones["leftMainPanel"].mode = "visible";
    m_zones["leftMainPanel"].dimension = 220.0;
    m_zones["leftMainPanel"].elements.clear();
    m_zones["leftMainPanel"].elements.append(
        m_allElements["thumbnail_browser"]);

    m_zones["mainPanel"].mode = "visible";
    m_zones["mainPanel"].elements.clear();
    m_zones["mainPanel"].elements.append(m_allElements["main_files_view"]);
    m_zones["mainPanel"].elements.append(m_allElements["column_browser"]);
    m_zones["mainPanel"].elements.append(m_allElements["jump_bar"]);
    m_activeOverrides["columnBrowser"] = "visible";
    m_activeOverrides["jumpBar"] = "visible";

    m_zones["rightMainPanel"].mode = "hidden";

    m_zones["rightSidebar"].mode = "auto_open";
    m_zones["rightSidebar"].dimension = 260.0;
    m_zones["rightSidebar"].elements.clear();
    m_zones["rightSidebar"].elements.append(m_allElements["playing_tracks"]);

    m_zones["topPanel"].mode = "hidden";
    m_zones["bottomPanel"].mode = "visible";
    m_zones["bottomPanel"].dimension = 72.0;

    emit presetApplied("catalog", "album_and_tracks_default");
  } else if (id == "stage") {
    m_zones["leftSidebar"].mode = "auto_open";
    m_zones["leftSidebar"].dimension = 220.0;
    m_zones["leftSidebar"].elements.clear();
    m_zones["leftSidebar"].elements.append(m_allElements["navigator"]);

    m_zones["leftMainPanel"].mode = "hidden";

    m_zones["mainPanel"].mode = "visible";
    m_zones["mainPanel"].elements.clear();
    m_zones["mainPanel"].elements.append(m_allElements["main_files_view"]);
    m_activeOverrides["columnBrowser"] = "hidden";
    m_activeOverrides["jumpBar"] = "hidden";

    m_zones["rightMainPanel"].mode = "hidden";

    m_zones["rightSidebar"].mode = "auto_open";
    m_zones["rightSidebar"].dimension = 260.0;
    m_zones["rightSidebar"].elements.clear();
    auto queueEl = m_allElements["playing_tracks"];
    queueEl.isStacked = true;
    queueEl.stackMode = "sub_tabs";
    auto lyricsEl = m_allElements["lyrics"];
    lyricsEl.isStacked = true;
    lyricsEl.stackMode = "sub_tabs";
    m_zones["rightSidebar"].elements.append(queueEl);
    m_zones["rightSidebar"].elements.append(lyricsEl);

    m_zones["topPanel"].mode = "hidden";
    m_zones["bottomPanel"].mode = "visible";
    m_zones["bottomPanel"].dimension = 72.0;

    emit presetApplied("stage", "now_playing_default");
  } else if (id == "compact") {
    m_zones["leftSidebar"].mode = "permanent";
    m_zones["leftSidebar"].dimension = 180.0;
    m_zones["leftSidebar"].elements.clear();
    m_zones["leftSidebar"].elements.append(m_allElements["navigator"]);

    m_zones["leftMainPanel"].mode = "hidden";

    m_zones["mainPanel"].mode = "visible";
    m_zones["mainPanel"].elements.clear();
    m_zones["mainPanel"].elements.append(m_allElements["main_files_view"]);
    m_activeOverrides["columnBrowser"] = "hidden";
    m_activeOverrides["jumpBar"] = "hidden";

    m_zones["rightMainPanel"].mode = "hidden";
    m_zones["rightSidebar"].mode = "hidden";

    m_zones["topPanel"].mode = "hidden";
    m_zones["bottomPanel"].mode = "visible";
    m_zones["bottomPanel"].dimension = 64.0;

    emit presetApplied("compact", "tracks_default");
  } else {
    m_currentPreset = "studio";
    initializeDefaultZones();
    m_activeOverrides["columnBrowser"] = "visible";
    m_activeOverrides["jumpBar"] = "visible";
    emit presetApplied("studio", "tracks_default");
  }

  emit layoutChanged();
}
