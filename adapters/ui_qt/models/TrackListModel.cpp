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
  m_selectedIndices.clear();
  applyFilter();
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
  setSearchQuery(filterText);
}

void TrackListModel::setSearchQuery(const QString &query) {
  if (m_searchQuery == query && m_filterText == query)
    return;
  m_searchQuery = query;
  m_filterText = query;
  beginResetModel();
  m_selectedIndices.clear();
  applyFilter();
  endResetModel();
  emit countChanged();
  emit selectionChanged();
}

void TrackListModel::setGenreFilter(const QString &genre) {
  if (m_genreFilter == genre)
    return;
  m_genreFilter = genre;
  beginResetModel();
  m_selectedIndices.clear();
  applyFilter();
  endResetModel();
  emit countChanged();
  emit selectionChanged();
}

void TrackListModel::setArtistFilter(const QString &artist) {
  if (m_artistFilter == artist)
    return;
  m_artistFilter = artist;
  beginResetModel();
  m_selectedIndices.clear();
  applyFilter();
  endResetModel();
  emit countChanged();
  emit selectionChanged();
}

void TrackListModel::setAlbumFilter(const QString &album) {
  if (m_albumFilter == album)
    return;
  m_albumFilter = album;
  beginResetModel();
  m_selectedIndices.clear();
  applyFilter();
  endResetModel();
  emit countChanged();
  emit selectionChanged();
}

void TrackListModel::setLetterFilter(const QString &letter) {
  if (m_letterFilter == letter)
    return;
  m_letterFilter = letter;
  beginResetModel();
  m_selectedIndices.clear();
  applyFilter();
  endResetModel();
  emit countChanged();
  emit selectionChanged();
}

void TrackListModel::setHiResOnly(bool hiResOnly) {
  if (m_hiResOnly == hiResOnly)
    return;
  m_hiResOnly = hiResOnly;
  beginResetModel();
  m_selectedIndices.clear();
  applyFilter();
  endResetModel();
  emit countChanged();
  emit selectionChanged();
}

void TrackListModel::resetAllFilters() {
  m_filterText.clear();
  m_searchQuery.clear();
  m_genreFilter.clear();
  m_artistFilter.clear();
  m_albumFilter.clear();
  m_letterFilter.clear();
  m_hiResOnly = false;
  beginResetModel();
  m_selectedIndices.clear();
  applyFilter();
  endResetModel();
  emit countChanged();
  emit selectionChanged();
}

void TrackListModel::applyFilter() {
  const bool hasSearch = !m_searchQuery.trimmed().isEmpty();
  const bool hasLegacyFilter = !m_filterText.trimmed().isEmpty() && !hasSearch;
  const bool hasGenre =
      !m_genreFilter.trimmed().isEmpty() && m_genreFilter != "All";
  const bool hasArtist =
      !m_artistFilter.trimmed().isEmpty() && m_artistFilter != "All";
  const bool hasAlbum =
      !m_albumFilter.trimmed().isEmpty() && m_albumFilter != "All";
  const bool hasLetter =
      !m_letterFilter.trimmed().isEmpty() && m_letterFilter != "All";

  if (!hasSearch && !hasLegacyFilter && !hasGenre && !hasArtist && !hasAlbum &&
      !hasLetter && !m_hiResOnly) {
    m_filteredTracks = m_allTracks;
  } else {
    m_filteredTracks.clear();
    const QString searchLower =
        (hasSearch ? m_searchQuery : m_filterText).trimmed().toLower();
    const QString genreLower = m_genreFilter.trimmed().toLower();
    const QString artistLower = m_artistFilter.trimmed().toLower();
    const QString albumLower = m_albumFilter.trimmed().toLower();
    const QString letterLower = m_letterFilter.trimmed().toLower();

    for (const auto &t : m_allTracks) {
      if (m_hiResOnly) {
        if (!(t.bitDepth >= 24 || t.sampleRate >= 88200 || t.codec == "DSD")) {
          continue;
        }
      }

      if (hasLetter) {
        bool letterMatch = false;
        QString titleStr = QString::fromStdString(t.title).trimmed();
        QString artistStr = QString::fromStdString(t.artist).trimmed();
        if (letterLower == "#") {
          QChar c1 = titleStr.isEmpty() ? QChar() : titleStr.at(0);
          QChar c2 = artistStr.isEmpty() ? QChar() : artistStr.at(0);
          if ((!c1.isNull() && (c1.isDigit() || !c1.isLetter())) ||
              (!c2.isNull() && (c2.isDigit() || !c2.isLetter()))) {
            letterMatch = true;
          }
        } else {
          if ((!titleStr.isEmpty() &&
               titleStr.startsWith(letterLower, Qt::CaseInsensitive)) ||
              (!artistStr.isEmpty() &&
               artistStr.startsWith(letterLower, Qt::CaseInsensitive))) {
            letterMatch = true;
          }
        }
        if (!letterMatch) {
          continue;
        }
      }

      if (hasGenre) {
        QString trackGenre =
            QString::fromStdString(t.genre).trimmed().toLower();
        if (trackGenre != genreLower && !trackGenre.contains(genreLower)) {
          continue;
        }
      }

      if (hasArtist) {
        QString trackArtist =
            QString::fromStdString(t.artist).trimmed().toLower();
        QString albumArtist =
            QString::fromStdString(t.albumArtist).trimmed().toLower();
        bool artistMatch =
            (trackArtist == artistLower || albumArtist == artistLower ||
             trackArtist.contains(artistLower) ||
             albumArtist.contains(artistLower));
        if (!artistMatch) {
          continue;
        }
      }

      if (hasAlbum) {
        QString trackAlbum =
            QString::fromStdString(t.album).trimmed().toLower();
        if (trackAlbum != albumLower && !trackAlbum.contains(albumLower)) {
          continue;
        }
      }

      if (hasSearch || hasLegacyFilter) {
        QString title = QString::fromStdString(t.title).toLower();
        QString artist = QString::fromStdString(t.artist).toLower();
        QString albumArtist = QString::fromStdString(t.albumArtist).toLower();
        QString album = QString::fromStdString(t.album).toLower();
        QString genre = QString::fromStdString(t.genre).toLower();
        QString comment = QString::fromStdString(t.comment).toLower();
        QString filePath = QString::fromStdString(t.filePath).toLower();
        if (!title.contains(searchLower) && !artist.contains(searchLower) &&
            !albumArtist.contains(searchLower) &&
            !album.contains(searchLower) && !genre.contains(searchLower) &&
            !comment.contains(searchLower) && !filePath.contains(searchLower)) {
          continue;
        }
      }

      m_filteredTracks.push_back(t);
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
    return static_cast<int>(track.playCount);
  }
  case RatingRole: {
    return static_cast<int>(track.rating);
  }
  case IsFavoriteRole: {
    return track.isFavorite;
  }
  case SkipCountRole: {
    return static_cast<int>(track.skipCount);
  }
  case LastPlayedRole: {
    return static_cast<qint64>(track.lastPlayed);
  }
  case LastPlayedFormattedRole: {
    if (track.lastPlayed > 0) {
      QDateTime dt = QDateTime::fromSecsSinceEpoch(track.lastPlayed);
      return dt.toString("yyyy-MM-dd hh:mm");
    }
    return QStringLiteral("Never");
  }
  case CommentRole: {
    return QString::fromStdString(track.comment);
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

  map["playCount"] = static_cast<int>(t.playCount);
  map["skipCount"] = static_cast<int>(t.skipCount);
  map["rating"] = static_cast<int>(t.rating);
  map["isFavorite"] = t.isFavorite;
  map["lastPlayed"] = static_cast<qint64>(t.lastPlayed);
  map["comment"] = QString::fromStdString(t.comment);

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
  roles[IsFavoriteRole] = "isFavorite";
  roles[SkipCountRole] = "skipCount";
  roles[LastPlayedRole] = "lastPlayed";
  roles[LastPlayedFormattedRole] = "lastPlayedFormatted";
  roles[CommentRole] = "comment";
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

void TrackListModel::setTrackRating(int index, int rating) {
  if (index < 0 || index >= static_cast<int>(m_filteredTracks.size()))
    return;
  m_filteredTracks[static_cast<size_t>(index)].rating =
      static_cast<uint8_t>(std::clamp(rating, 0, 5));
  emit dataChanged(createIndex(index, 0), createIndex(index, 0), {RatingRole});
}

void TrackListModel::setTrackFavorite(int index, bool isFavorite) {
  if (index < 0 || index >= static_cast<int>(m_filteredTracks.size()))
    return;
  m_filteredTracks[static_cast<size_t>(index)].isFavorite = isFavorite;
  emit dataChanged(createIndex(index, 0), createIndex(index, 0),
                   {IsFavoriteRole});
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
      if (a.rating != b.rating)
        cmp = a.rating < b.rating ? -1 : 1;
    } else if (m_sortColumn == "isFavorite") {
      if (a.isFavorite != b.isFavorite)
        cmp = a.isFavorite ? 1 : -1;
    } else if (m_sortColumn == "playCount") {
      if (a.playCount != b.playCount)
        cmp = a.playCount < b.playCount ? -1 : 1;
    } else if (m_sortColumn == "skipCount") {
      if (a.skipCount != b.skipCount)
        cmp = a.skipCount < b.skipCount ? -1 : 1;
    } else if (m_sortColumn == "lastPlayed") {
      if (a.lastPlayed != b.lastPlayed)
        cmp = a.lastPlayed < b.lastPlayed ? -1 : 1;
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
