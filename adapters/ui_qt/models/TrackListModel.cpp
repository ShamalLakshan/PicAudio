#include "TrackListModel.hpp"
#include "core/ports/IMusicDatabasePort.hpp"
#include <QDateTime>
#include <QFileInfo>
#include <QSettings>
#include <algorithm>

TrackListModel::TrackListModel(core::IMusicDatabasePort *dbPort,
                               QObject *parent)
    : QAbstractListModel(parent), m_dbPort(dbPort) {
  if (m_dbPort) {
    loadAllTracks();
  }
}

void TrackListModel::setDatabasePort(core::IMusicDatabasePort *dbPort) {
  m_dbPort = dbPort;
}

void TrackListModel::setTracks(const std::vector<core::Track> &tracks) {
  beginResetModel();
  m_allTracks = tracks;
  m_filteredTracks = tracks;
  m_filterText.clear();
  m_selectedIndices.clear();
  if (!m_sortColumn.isEmpty()) {
    applySort();
  }
  endResetModel();
  emit countChanged();
  emit selectionChanged();
}

void TrackListModel::loadAllTracks() {
  beginResetModel();
  m_allTracks.clear();
  m_filteredTracks.clear();
  m_selectedIndices.clear();
  if (m_dbPort) {
    m_allTracks = m_dbPort->getAllTracks();
  }
  applyFilter();
  endResetModel();
  emit countChanged();
  emit selectionChanged();
}

void TrackListModel::loadAlbumTracks(const QString &albumTitle,
                                     const QString &albumArtist) {
  beginResetModel();
  m_allTracks.clear();
  m_filteredTracks.clear();
  m_selectedIndices.clear();
  if (m_dbPort) {
    m_allTracks = m_dbPort->getTracksByAlbum(albumTitle.toStdString(),
                                             albumArtist.toStdString());
  }
  applyFilter();
  endResetModel();
  emit countChanged();
  emit selectionChanged();
}

void TrackListModel::setFilter(const QString &filterText) {
  if (m_filterText == filterText)
    return;
  m_filterText = filterText;
  beginResetModel();
  m_selectedIndices.clear();
  applyFilter();
  endResetModel();
  emit countChanged();
  emit selectionChanged();
}

void TrackListModel::applyFilter() {
  if (m_filterText.trimmed().isEmpty()) {
    m_filteredTracks = m_allTracks;
  } else {
    m_filteredTracks.clear();
    QString lower = m_filterText.toLower();
    for (const auto &t : m_allTracks) {
      QString title = QString::fromStdString(t.title).toLower();
      QString artist = QString::fromStdString(t.artist).toLower();
      QString album = QString::fromStdString(t.album).toLower();
      QString genre = QString::fromStdString(t.genre).toLower();
      if (title.contains(lower) || artist.contains(lower) ||
          album.contains(lower) || genre.contains(lower)) {
        m_filteredTracks.push_back(t);
      }
    }
  }
  if (!m_sortColumn.isEmpty()) {
    applySort();
  }
}

int TrackListModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return static_cast<int>(m_filteredTracks.size());
}

QVariant TrackListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 ||
      index.row() >= static_cast<int>(m_filteredTracks.size())) {
    return QVariant();
  }

  const auto &track = m_filteredTracks[static_cast<size_t>(index.row())];

  switch (role) {
  case IdRole:
    return QString::fromStdString(track.id);
  case FilePathRole:
    return QString::fromStdString(track.filePath);
  case TitleRole:
    return QString::fromStdString(track.title);
  case ArtistRole:
    return QString::fromStdString(track.artist);
  case AlbumRole:
    return QString::fromStdString(track.album);
  case AlbumArtistRole:
    if (!track.albumArtist.empty() && track.albumArtist != "Unknown Artist") {
      return QString::fromStdString(track.albumArtist);
    }
    return QString::fromStdString(track.artist);
  case GenreRole:
    return QString::fromStdString(track.genre);
  case YearRole:
    return track.year > 0 ? QVariant(track.year) : QVariant();
  case TrackNumberRole:
    return track.trackNumber;
  case DiscNumberRole:
    return track.discNumber;
  case DurationMsRole:
    return static_cast<qint64>(track.durationMs);
  case DurationFormattedRole:
    return QString::fromStdString(track.durationFormatted());
  case SampleRateRole:
    return track.sampleRate;
  case BitDepthRole:
    return track.bitDepth;
  case ChannelsRole:
    return track.channels;
  case BitrateRole:
    return track.bitrate;
  case CodecRole:
    return QString::fromStdString(track.codec);
  case ArtHashRole:
    return QString::fromStdString(track.artHash);
  case AudiophileBadgeRole:
    return QString::fromStdString(track.audiophileBadge());
  case IsSelectedRole:
    return m_selectedIndices.find(index.row()) != m_selectedIndices.end();
  case FileSizeBytesRole:
    return static_cast<qint64>(track.fileSizeBytes);
  case FileSizeFormattedRole: {
    quint64 bytes = track.fileSizeBytes;
    if (bytes == 0 && !track.filePath.empty()) {
      QFileInfo fi(QString::fromStdString(track.filePath));
      if (fi.exists())
        bytes = static_cast<quint64>(fi.size());
    }
    if (bytes == 0)
      return QString("");
    if (bytes < 1024 * 1024) {
      return QString("%1 KB").arg(bytes / 1024);
    }
    double mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
    return QString("%1 MB").arg(QString::number(mb, 'f', 1));
  }
  case DateAddedRole:
    return static_cast<qint64>(track.dateAdded);
  case DateAddedFormattedRole: {
    if (track.dateAdded > 0) {
      return QDateTime::fromSecsSinceEpoch(track.dateAdded)
          .toString("yyyy-MM-dd");
    }
    if (!track.filePath.empty()) {
      QFileInfo fi(QString::fromStdString(track.filePath));
      if (fi.exists()) {
        return fi.lastModified().toString("yyyy-MM-dd");
      }
    }
    return QString("");
  }
  case PlayCountRole: {
    QSettings s("PicAudio", "PicAudioPlayer");
    return s.value("playcounts/" + QString::fromStdString(track.id), 0).toInt();
  }
  case RatingRole: {
    QSettings s("PicAudio", "PicAudioPlayer");
    return s.value("ratings/" + QString::fromStdString(track.id), 0).toInt();
  }
  default:
    return QVariant();
  }
}

QVariantMap TrackListModel::getTrackAt(int index) const {
  QVariantMap map;
  if (index < 0 || index >= static_cast<int>(m_filteredTracks.size())) {
    return map;
  }
  const auto &t = m_filteredTracks[static_cast<size_t>(index)];
  map["id"] = QString::fromStdString(t.id);
  map["filePath"] = QString::fromStdString(t.filePath);
  map["title"] = QString::fromStdString(t.title);
  map["artist"] = QString::fromStdString(t.artist);
  map["album"] = QString::fromStdString(t.album);
  map["albumArtist"] = QString::fromStdString(
      (!t.albumArtist.empty() && t.albumArtist != "Unknown Artist")
          ? t.albumArtist
          : t.artist);
  map["year"] = t.year;
  map["genre"] = QString::fromStdString(t.genre);
  map["trackNumber"] = t.trackNumber;
  map["durationFormatted"] = QString::fromStdString(t.durationFormatted());
  map["durationMs"] = static_cast<qint64>(t.durationMs);
  map["sampleRate"] = t.sampleRate;
  map["bitDepth"] = t.bitDepth;
  map["channels"] = t.channels;
  map["bitrate"] = t.bitrate;
  map["codec"] = QString::fromStdString(t.codec);
  map["artHash"] = QString::fromStdString(t.artHash);
  map["audiophileBadge"] = QString::fromStdString(t.audiophileBadge());
  map["isSelected"] = m_selectedIndices.find(index) != m_selectedIndices.end();
  map["fileSizeBytes"] = static_cast<qint64>(t.fileSizeBytes);

  quint64 bytes = t.fileSizeBytes;
  if (bytes == 0 && !t.filePath.empty()) {
    QFileInfo fi(QString::fromStdString(t.filePath));
    if (fi.exists())
      bytes = static_cast<quint64>(fi.size());
  }
  if (bytes > 0) {
    if (bytes < 1024 * 1024) {
      map["fileSizeFormatted"] = QString("%1 KB").arg(bytes / 1024);
    } else {
      double mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
      map["fileSizeFormatted"] =
          QString("%1 MB").arg(QString::number(mb, 'f', 1));
    }
  } else {
    map["fileSizeFormatted"] = QString("");
  }

  map["dateAdded"] = static_cast<qint64>(t.dateAdded);
  if (t.dateAdded > 0) {
    map["dateAddedFormatted"] =
        QDateTime::fromSecsSinceEpoch(t.dateAdded).toString("yyyy-MM-dd");
  } else if (!t.filePath.empty()) {
    QFileInfo fi(QString::fromStdString(t.filePath));
    if (fi.exists()) {
      map["dateAddedFormatted"] = fi.lastModified().toString("yyyy-MM-dd");
    } else {
      map["dateAddedFormatted"] = QString("");
    }
  } else {
    map["dateAddedFormatted"] = QString("");
  }

  QSettings s("PicAudio", "PicAudioPlayer");
  map["playCount"] =
      s.value("playcounts/" + QString::fromStdString(t.id), 0).toInt();
  map["rating"] = s.value("ratings/" + QString::fromStdString(t.id), 0).toInt();

  return map;
}

QHash<int, QByteArray> TrackListModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[IdRole] = "id";
  roles[FilePathRole] = "filePath";
  roles[TitleRole] = "title";
  roles[ArtistRole] = "artist";
  roles[AlbumRole] = "album";
  roles[AlbumArtistRole] = "albumArtist";
  roles[GenreRole] = "genre";
  roles[YearRole] = "year";
  roles[TrackNumberRole] = "trackNumber";
  roles[DiscNumberRole] = "discNumber";
  roles[DurationMsRole] = "durationMs";
  roles[DurationFormattedRole] = "durationFormatted";
  roles[SampleRateRole] = "sampleRate";
  roles[BitDepthRole] = "bitDepth";
  roles[ChannelsRole] = "channels";
  roles[BitrateRole] = "bitrate";
  roles[CodecRole] = "codec";
  roles[ArtHashRole] = "artHash";
  roles[AudiophileBadgeRole] = "audiophileBadge";
  roles[IsSelectedRole] = "isSelected";
  roles[FileSizeBytesRole] = "fileSizeBytes";
  roles[FileSizeFormattedRole] = "fileSizeFormatted";
  roles[DateAddedRole] = "dateAdded";
  roles[DateAddedFormattedRole] = "dateAddedFormatted";
  roles[PlayCountRole] = "playCount";
  roles[RatingRole] = "rating";
  return roles;
}

void TrackListModel::selectAll() {
  if (m_filteredTracks.empty())
    return;
  m_selectedIndices.clear();
  for (size_t i = 0; i < m_filteredTracks.size(); ++i) {
    m_selectedIndices.insert(static_cast<int>(i));
  }
  emit dataChanged(
      createIndex(0, 0),
      createIndex(static_cast<int>(m_filteredTracks.size()) - 1, 0),
      {IsSelectedRole});
  emit selectionChanged();
}

void TrackListModel::invertSelection() {
  if (m_filteredTracks.empty())
    return;
  std::unordered_set<int> inverted;
  for (size_t i = 0; i < m_filteredTracks.size(); ++i) {
    int idx = static_cast<int>(i);
    if (m_selectedIndices.find(idx) == m_selectedIndices.end()) {
      inverted.insert(idx);
    }
  }
  m_selectedIndices = std::move(inverted);
  emit dataChanged(
      createIndex(0, 0),
      createIndex(static_cast<int>(m_filteredTracks.size()) - 1, 0),
      {IsSelectedRole});
  emit selectionChanged();
}

void TrackListModel::clearSelection() {
  if (m_selectedIndices.empty())
    return;
  m_selectedIndices.clear();
  emit dataChanged(
      createIndex(0, 0),
      createIndex(static_cast<int>(m_filteredTracks.size()) - 1, 0),
      {IsSelectedRole});
  emit selectionChanged();
}

void TrackListModel::selectRange(int fromIndex, int toIndex) {
  if (m_filteredTracks.empty())
    return;
  int maxIdx = static_cast<int>(m_filteredTracks.size()) - 1;
  int start = std::clamp(std::min(fromIndex, toIndex), 0, maxIdx);
  int end = std::clamp(std::max(fromIndex, toIndex), 0, maxIdx);
  for (int i = start; i <= end; ++i) {
    m_selectedIndices.insert(i);
  }
  emit dataChanged(createIndex(start, 0), createIndex(end, 0),
                   {IsSelectedRole});
  emit selectionChanged();
}

void TrackListModel::toggleSelection(int index) {
  if (index < 0 || index >= static_cast<int>(m_filteredTracks.size()))
    return;
  auto it = m_selectedIndices.find(index);
  if (it != m_selectedIndices.end()) {
    m_selectedIndices.erase(it);
  } else {
    m_selectedIndices.insert(index);
  }
  emit dataChanged(createIndex(index, 0), createIndex(index, 0),
                   {IsSelectedRole});
  emit selectionChanged();
}

void TrackListModel::setRowSelected(int index, bool selected) {
  if (index < 0 || index >= static_cast<int>(m_filteredTracks.size()))
    return;
  if (selected) {
    m_selectedIndices.insert(index);
  } else {
    m_selectedIndices.erase(index);
  }
  emit dataChanged(createIndex(index, 0), createIndex(index, 0),
                   {IsSelectedRole});
  emit selectionChanged();
}

bool TrackListModel::isSelected(int index) const {
  return m_selectedIndices.find(index) != m_selectedIndices.end();
}

int TrackListModel::selectedCount() const {
  return static_cast<int>(m_selectedIndices.size());
}

QVariantList TrackListModel::getSelectedTrackIds() const {
  std::vector<int> sortedIndices(m_selectedIndices.begin(),
                                 m_selectedIndices.end());
  std::sort(sortedIndices.begin(), sortedIndices.end());
  QVariantList list;
  for (int idx : sortedIndices) {
    if (idx >= 0 && idx < static_cast<int>(m_filteredTracks.size())) {
      list.append(QString::fromStdString(
          m_filteredTracks[static_cast<size_t>(idx)].id));
    }
  }
  return list;
}

QVariantList TrackListModel::getSelectedTracks() const {
  std::vector<int> sortedIndices(m_selectedIndices.begin(),
                                 m_selectedIndices.end());
  std::sort(sortedIndices.begin(), sortedIndices.end());
  QVariantList list;
  for (int idx : sortedIndices) {
    if (idx >= 0 && idx < static_cast<int>(m_filteredTracks.size())) {
      list.append(getTrackAt(idx));
    }
  }
  return list;
}

void TrackListModel::removeSelected() {
  if (m_selectedIndices.empty())
    return;

  std::vector<core::Track> remainingTracks;
  remainingTracks.reserve(m_filteredTracks.size());
  for (size_t i = 0; i < m_filteredTracks.size(); ++i) {
    if (m_selectedIndices.find(static_cast<int>(i)) ==
        m_selectedIndices.end()) {
      remainingTracks.push_back(m_filteredTracks[i]);
    }
  }
  setTracks(remainingTracks);
}

void TrackListModel::sortBy(const QString &columnKey, bool ascending) {
  m_sortColumn = columnKey;
  m_sortAscending = ascending;
  beginResetModel();
  m_selectedIndices.clear();
  applySort();
  endResetModel();
  emit selectionChanged();
  emit sortChanged();
}

void TrackListModel::toggleSort(const QString &columnKey) {
  if (m_sortColumn == columnKey) {
    sortBy(columnKey, !m_sortAscending);
  } else {
    sortBy(columnKey, true);
  }
}

void TrackListModel::clearSort() {
  m_sortColumn.clear();
  m_sortAscending = true;
  beginResetModel();
  m_selectedIndices.clear();
  applyFilter();
  endResetModel();
  emit selectionChanged();
  emit sortChanged();
}

void TrackListModel::applySort() {
  if (m_sortColumn.isEmpty() || m_filteredTracks.empty())
    return;

  auto compareTracks = [this](const core::Track &a,
                              const core::Track &b) -> bool {
    int cmp = 0;
    if (m_sortColumn == "trackNumber") {
      if (a.trackNumber != b.trackNumber)
        cmp = a.trackNumber < b.trackNumber ? -1 : 1;
    } else if (m_sortColumn == "title") {
      cmp = QString::compare(QString::fromStdString(a.title),
                             QString::fromStdString(b.title),
                             Qt::CaseInsensitive);
    } else if (m_sortColumn == "artist") {
      cmp = QString::compare(QString::fromStdString(a.artist),
                             QString::fromStdString(b.artist),
                             Qt::CaseInsensitive);
    } else if (m_sortColumn == "album") {
      cmp = QString::compare(QString::fromStdString(a.album),
                             QString::fromStdString(b.album),
                             Qt::CaseInsensitive);
    } else if (m_sortColumn == "albumArtist") {
      cmp = QString::compare(QString::fromStdString(a.albumArtist),
                             QString::fromStdString(b.albumArtist),
                             Qt::CaseInsensitive);
    } else if (m_sortColumn == "genre") {
      cmp = QString::compare(QString::fromStdString(a.genre),
                             QString::fromStdString(b.genre),
                             Qt::CaseInsensitive);
    } else if (m_sortColumn == "year") {
      if (a.year != b.year)
        cmp = a.year < b.year ? -1 : 1;
    } else if (m_sortColumn == "duration" || m_sortColumn == "durationMs") {
      if (a.durationMs != b.durationMs)
        cmp = a.durationMs < b.durationMs ? -1 : 1;
    } else if (m_sortColumn == "bitrate") {
      if (a.bitrate != b.bitrate)
        cmp = a.bitrate < b.bitrate ? -1 : 1;
    } else if (m_sortColumn == "sampleRate") {
      if (a.sampleRate != b.sampleRate)
        cmp = a.sampleRate < b.sampleRate ? -1 : 1;
    } else if (m_sortColumn == "bitDepth") {
      if (a.bitDepth != b.bitDepth)
        cmp = a.bitDepth < b.bitDepth ? -1 : 1;
    } else if (m_sortColumn == "codec") {
      cmp = QString::compare(QString::fromStdString(a.codec),
                             QString::fromStdString(b.codec),
                             Qt::CaseInsensitive);
    } else if (m_sortColumn == "filePath") {
      cmp = QString::compare(QString::fromStdString(a.filePath),
                             QString::fromStdString(b.filePath),
                             Qt::CaseInsensitive);
    } else if (m_sortColumn == "fileSize" || m_sortColumn == "fileSizeBytes") {
      if (a.fileSizeBytes != b.fileSizeBytes)
        cmp = a.fileSizeBytes < b.fileSizeBytes ? -1 : 1;
    } else if (m_sortColumn == "dateAdded") {
      if (a.dateAdded != b.dateAdded)
        cmp = a.dateAdded < b.dateAdded ? -1 : 1;
    } else if (m_sortColumn == "rating") {
      QSettings s("PicAudio", "PicAudioPlayer");
      int ra = s.value("ratings/" + QString::fromStdString(a.id), 0).toInt();
      int rb = s.value("ratings/" + QString::fromStdString(b.id), 0).toInt();
      if (ra != rb)
        cmp = ra < rb ? -1 : 1;
    } else if (m_sortColumn == "playCount") {
      QSettings s("PicAudio", "PicAudioPlayer");
      int pa = s.value("playcounts/" + QString::fromStdString(a.id), 0).toInt();
      int pb = s.value("playcounts/" + QString::fromStdString(b.id), 0).toInt();
      if (pa != pb)
        cmp = pa < pb ? -1 : 1;
    }

    if (cmp == 0) {
      int albumCmp = QString::compare(QString::fromStdString(a.album),
                                      QString::fromStdString(b.album),
                                      Qt::CaseInsensitive);
      if (albumCmp != 0)
        return albumCmp < 0;
      if (a.discNumber != b.discNumber)
        return a.discNumber < b.discNumber;
      if (a.trackNumber != b.trackNumber)
        return a.trackNumber < b.trackNumber;
      return QString::compare(QString::fromStdString(a.title),
                              QString::fromStdString(b.title),
                              Qt::CaseInsensitive) < 0;
    }

    return m_sortAscending ? (cmp < 0) : (cmp > 0);
  };

  std::sort(m_filteredTracks.begin(), m_filteredTracks.end(), compareTracks);
}
