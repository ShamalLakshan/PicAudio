#include "ViewRegistryModel.hpp"
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

ViewRegistryModel::ViewRegistryModel(QObject *parent) : QObject(parent) {
  registerBuiltinViews();
  loadViews();
}

void ViewRegistryModel::registerBuiltinViews() {
  m_views.clear();

  // 1. Tracks Spreadsheet Table
  ViewDefinition tracks;
  tracks.viewId = "tracks_default";
  tracks.name = "Tracks Table";
  tracks.baseRendererType = "tracks";
  tracks.displayedColumns = {"trackNumber", "title",   "artist", "album",
                             "duration",    "bitrate", "rating"};
  tracks.groupBy = "album_artist";
  tracks.thumbnailSize = 120;
  tracks.isBuiltin = true;
  m_views["tracks_default"] = tracks;

  // 2. Album and Tracks Split View
  ViewDefinition albumTracks;
  albumTracks.viewId = "album_and_tracks_default";
  albumTracks.name = "Album & Tracks";
  albumTracks.baseRendererType = "album_and_tracks";
  albumTracks.displayedColumns = {"trackNumber", "title", "duration", "rating"};
  albumTracks.groupBy = "album";
  albumTracks.thumbnailSize = 180;
  albumTracks.isBuiltin = true;
  m_views["album_and_tracks_default"] = albumTracks;

  // 3. Album Covers Responsive Grid
  ViewDefinition covers;
  covers.viewId = "covers_default";
  covers.name = "Album Covers Grid";
  covers.baseRendererType = "covers";
  covers.thumbnailSize = 160;
  covers.isBuiltin = true;
  m_views["covers_default"] = covers;

  // 4. Artists Visual Grid
  ViewDefinition artists;
  artists.viewId = "artists_default";
  artists.name = "Artists Grid";
  artists.baseRendererType = "artists";
  artists.thumbnailSize = 150;
  artists.isBuiltin = true;
  m_views["artists_default"] = artists;

  // 5. Jukebox Presentation View
  ViewDefinition jukebox;
  jukebox.viewId = "jukebox_default";
  jukebox.name = "Jukebox";
  jukebox.baseRendererType = "jukebox";
  jukebox.thumbnailSize = 240;
  jukebox.panelOverrides["leftSidebar"] = "hidden";
  jukebox.panelOverrides["leftMainPanel"] = "hidden";
  jukebox.panelOverrides["rightMainPanel"] = "hidden";
  jukebox.panelOverrides["columnBrowser"] = "hidden";
  jukebox.panelOverrides["jumpBar"] = "hidden";
  jukebox.isBuiltin = true;
  m_views["jukebox_default"] = jukebox;

  // 6. Now Playing View
  ViewDefinition nowPlaying;
  nowPlaying.viewId = "now_playing_default";
  nowPlaying.name = "Now Playing";
  nowPlaying.baseRendererType = "now_playing";
  nowPlaying.panelOverrides["leftSidebar"] = "hidden";
  nowPlaying.panelOverrides["leftMainPanel"] = "hidden";
  nowPlaying.panelOverrides["rightMainPanel"] = "hidden";
  nowPlaying.panelOverrides["columnBrowser"] = "hidden";
  nowPlaying.isBuiltin = true;
  m_views["now_playing_default"] = nowPlaying;

  // 7. Listening History View
  ViewDefinition history;
  history.viewId = "history_default";
  history.name = "History";
  history.baseRendererType = "history";
  history.isBuiltin = true;
  m_views["history_default"] = history;
}

QVariantList ViewRegistryModel::getAllViews() const {
  QVariantList list;
  for (const auto &v : m_views) {
    QVariantMap map;
    map["viewId"] = v.viewId;
    map["name"] = v.name;
    map["baseRendererType"] = v.baseRendererType;
    map["displayedColumns"] = v.displayedColumns;
    map["groupBy"] = v.groupBy;
    map["thumbnailSize"] = v.thumbnailSize;
    map["panelOverrides"] = v.panelOverrides;
    map["isBuiltin"] = v.isBuiltin;
    list.append(map);
  }
  return list;
}

QVariantMap ViewRegistryModel::getView(const QString &viewId) const {
  auto it = m_views.find(viewId);
  if (it != m_views.end()) {
    const auto &v = it.value();
    QVariantMap map;
    map["viewId"] = v.viewId;
    map["name"] = v.name;
    map["baseRendererType"] = v.baseRendererType;
    map["displayedColumns"] = v.displayedColumns;
    map["groupBy"] = v.groupBy;
    map["thumbnailSize"] = v.thumbnailSize;
    map["panelOverrides"] = v.panelOverrides;
    map["isBuiltin"] = v.isBuiltin;
    return map;
  }
  // Return default tracks view if not found
  return getView("tracks_default");
}

QString ViewRegistryModel::createCustomView(const QString &name,
                                            const QString &baseRendererType,
                                            const QString &sourceViewId) {
  QString newId = QString("custom_%1").arg(QDateTime::currentMSecsSinceEpoch());
  ViewDefinition v;
  if (!sourceViewId.isEmpty() && m_views.contains(sourceViewId)) {
    v = m_views[sourceViewId];
  } else {
    v.baseRendererType =
        baseRendererType.isEmpty() ? "tracks" : baseRendererType;
    v.displayedColumns = {"trackNumber", "title", "artist", "album",
                          "duration"};
    v.groupBy = "album_artist";
    v.thumbnailSize = 160;
  }
  v.viewId = newId;
  v.name = name.isEmpty() ? "New Custom View" : name;
  v.isBuiltin = false;

  m_views[newId] = v;
  saveViews();
  emit viewsChanged();
  return newId;
}

bool ViewRegistryModel::updateCustomView(const QString &viewId,
                                         const QString &name,
                                         const QStringList &columns,
                                         const QString &groupBy,
                                         int thumbnailSize,
                                         const QVariantMap &panelOverrides) {
  auto it = m_views.find(viewId);
  if (it == m_views.end() || it->isBuiltin) {
    return false;
  }
  if (!name.isEmpty())
    it->name = name;
  if (!columns.isEmpty())
    it->displayedColumns = columns;
  it->groupBy = groupBy;
  if (thumbnailSize > 0)
    it->thumbnailSize = thumbnailSize;
  it->panelOverrides = panelOverrides;

  saveViews();
  emit viewsChanged();
  return true;
}

bool ViewRegistryModel::deleteCustomView(const QString &viewId) {
  auto it = m_views.find(viewId);
  if (it == m_views.end() || it->isBuiltin) {
    return false;
  }
  m_views.erase(it);
  saveViews();
  emit viewsChanged();
  return true;
}

QString ViewRegistryModel::duplicateView(const QString &viewId,
                                         const QString &newName) {
  if (!m_views.contains(viewId)) {
    return {};
  }
  return createCustomView(newName, m_views[viewId].baseRendererType, viewId);
}

QVariantMap ViewRegistryModel::getPanelOverrides(const QString &viewId) const {
  auto it = m_views.find(viewId);
  if (it != m_views.end()) {
    return it->panelOverrides;
  }
  return {};
}

QString ViewRegistryModel::getBaseRendererType(const QString &viewId) const {
  auto it = m_views.find(viewId);
  if (it != m_views.end()) {
    return it->baseRendererType;
  }
  return "tracks";
}

void ViewRegistryModel::saveViews() {
  QSettings s("PicAudio", "PicAudio");
  QJsonArray arr;
  for (const auto &v : m_views) {
    if (v.isBuiltin)
      continue; // Only persist custom views
    QJsonObject obj;
    obj["viewId"] = v.viewId;
    obj["name"] = v.name;
    obj["baseRendererType"] = v.baseRendererType;
    obj["displayedColumns"] = QJsonArray::fromStringList(v.displayedColumns);
    obj["groupBy"] = v.groupBy;
    obj["thumbnailSize"] = v.thumbnailSize;
    obj["panelOverrides"] = QJsonObject::fromVariantMap(v.panelOverrides);
    arr.append(obj);
  }
  s.setValue("custom_views", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

void ViewRegistryModel::loadViews() {
  QSettings s("PicAudio", "PicAudio");
  if (!s.contains("custom_views"))
    return;

  QByteArray data = s.value("custom_views").toByteArray();
  QJsonDocument doc = QJsonDocument::fromJson(data);
  if (!doc.isArray())
    return;

  for (const auto &val : doc.array()) {
    QJsonObject obj = val.toObject();
    ViewDefinition v;
    v.viewId = obj["viewId"].toString();
    v.name = obj["name"].toString();
    v.baseRendererType = obj["baseRendererType"].toString("tracks");
    QJsonArray cols = obj["displayedColumns"].toArray();
    for (const auto &c : cols)
      v.displayedColumns.append(c.toString());
    v.groupBy = obj["groupBy"].toString();
    v.thumbnailSize = obj["thumbnailSize"].toInt(160);
    v.panelOverrides = obj["panelOverrides"].toObject().toVariantMap();
    v.isBuiltin = false;

    if (!v.viewId.isEmpty()) {
      m_views[v.viewId] = v;
    }
  }
  emit viewsChanged();
}
