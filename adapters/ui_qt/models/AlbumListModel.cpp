#include "AlbumListModel.hpp"
#include "core/entities/Track.hpp"
#include "core/ports/IMusicDatabasePort.hpp"
#include <algorithm>
#include <unordered_map>

AlbumListModel::AlbumListModel(core::IMusicDatabasePort *dbPort,
                               QObject *parent)
    : QAbstractListModel(parent), m_dbPort(dbPort) {
  if (m_dbPort) {
    reload();
  }
}

void AlbumListModel::setDatabasePort(core::IMusicDatabasePort *dbPort) {
  m_dbPort = dbPort;
  reload();
}

void AlbumListModel::reload() {
  beginResetModel();
  m_allAlbums.clear();
  m_filteredAlbums.clear();
  if (m_dbPort) {
    m_allAlbums = m_dbPort->getAllAlbums();
  }
  applyFilter();
  endResetModel();
  emit countChanged();
}

void AlbumListModel::setFilter(const QString &filterText) {
  setSearchQuery(filterText);
}

void AlbumListModel::setSearchQuery(const QString &query) {
  if (m_searchQuery == query && m_filterText == query)
    return;
  m_searchQuery = query;
  m_filterText = query;
  beginResetModel();
  applyFilter();
  endResetModel();
  emit countChanged();
}

void AlbumListModel::setGenreFilter(const QString &genre) {
  if (m_genreFilter == genre)
    return;
  m_genreFilter = genre;
  beginResetModel();
  applyFilter();
  endResetModel();
  emit countChanged();
}

void AlbumListModel::setArtistFilter(const QString &artist) {
  if (m_artistFilter == artist)
    return;
  m_artistFilter = artist;
  beginResetModel();
  applyFilter();
  endResetModel();
  emit countChanged();
}

void AlbumListModel::setLetterFilter(const QString &letter) {
  if (m_letterFilter == letter)
    return;
  m_letterFilter = letter;
  beginResetModel();
  applyFilter();
  endResetModel();
  emit countChanged();
}

void AlbumListModel::setHiResOnly(bool hiResOnly) {
  if (m_hiResOnly == hiResOnly)
    return;
  m_hiResOnly = hiResOnly;
  beginResetModel();
  applyFilter();
  endResetModel();
  emit countChanged();
}

void AlbumListModel::resetAllFilters() {
  m_filterText.clear();
  m_searchQuery.clear();
  m_genreFilter.clear();
  m_artistFilter.clear();
  m_letterFilter.clear();
  m_hiResOnly = false;
  beginResetModel();
  applyFilter();
  endResetModel();
  emit countChanged();
}

void AlbumListModel::setAlbumsFromTracks(
    const std::vector<core::Track> &tracks) {
  beginResetModel();
  m_allAlbums.clear();
  m_filteredAlbums.clear();

  struct AlbumAccumulator {
    std::string title;
    std::string artist;
    uint32_t year{0};
    std::string genre;
    uint32_t trackCount{0};
    uint64_t totalDurationMs{0};
    std::string artHash;
    uint32_t maxSampleRate{0};
    uint8_t maxBitDepth{0};
    std::string primaryCodec;
  };

  std::unordered_map<std::string, AlbumAccumulator> accMap;
  for (const auto &t : tracks) {
    std::string albTitle = t.album.empty() ? "Unknown Album" : t.album;
    std::string albArtist =
        (!t.albumArtist.empty() && t.albumArtist != "Unknown Artist")
            ? t.albumArtist
            : (t.artist.empty() ? "Unknown Artist" : t.artist);
    std::string key = albTitle + "_" + albArtist;

    auto &acc = accMap[key];
    if (acc.trackCount == 0) {
      acc.title = albTitle;
      acc.artist = albArtist;
      acc.year = t.year;
      acc.genre = t.genre;
      acc.artHash = t.artHash;
      acc.primaryCodec = t.codec;
    }
    acc.trackCount++;
    acc.totalDurationMs += t.durationMs;
    if (t.sampleRate > acc.maxSampleRate)
      acc.maxSampleRate = t.sampleRate;
    if (t.bitDepth > acc.maxBitDepth)
      acc.maxBitDepth = t.bitDepth;
    if (!t.artHash.empty() && acc.artHash.empty())
      acc.artHash = t.artHash;
  }

  for (auto &[_, acc] : accMap) {
    core::Album a;
    a.title = acc.title;
    a.artist = acc.artist;
    a.id = "alb_" + a.title + "_" + a.artist;
    a.year = acc.year;
    a.genre = acc.genre;
    a.trackCount = acc.trackCount;
    a.totalDurationMs = acc.totalDurationMs;
    a.artHash = acc.artHash;
    a.maxSampleRate = acc.maxSampleRate;
    a.maxBitDepth = acc.maxBitDepth;
    a.primaryCodec = acc.primaryCodec;

    if (a.maxSampleRate >= 88200 || a.maxBitDepth >= 24) {
      a.qualityBadge = "Hi-Res Lossless";
    } else if (a.primaryCodec == "DSD") {
      a.qualityBadge = "DSD";
    } else if (a.primaryCodec == "FLAC" || a.primaryCodec == "WAV" ||
               a.primaryCodec == "ALAC" || a.primaryCodec == "AIFF") {
      a.qualityBadge = "Lossless";
    } else {
      a.qualityBadge = "Standard";
    }
    m_allAlbums.push_back(std::move(a));
  }

  std::sort(m_allAlbums.begin(), m_allAlbums.end(),
            [](const core::Album &a, const core::Album &b) {
              return QString::compare(QString::fromStdString(a.title),
                                      QString::fromStdString(b.title),
                                      Qt::CaseInsensitive) < 0;
            });

  applyFilter();
  endResetModel();
  emit countChanged();
}

void AlbumListModel::applyFilter() {
  const bool hasSearch = !m_searchQuery.trimmed().isEmpty();
  const bool hasLegacyFilter = !m_filterText.trimmed().isEmpty() && !hasSearch;
  const bool hasGenre =
      !m_genreFilter.trimmed().isEmpty() && m_genreFilter != "All";
  const bool hasArtist =
      !m_artistFilter.trimmed().isEmpty() && m_artistFilter != "All";
  const bool hasLetter =
      !m_letterFilter.trimmed().isEmpty() && m_letterFilter != "All";

  if (!hasSearch && !hasLegacyFilter && !hasGenre && !hasArtist && !hasLetter &&
      !m_hiResOnly) {
    m_filteredAlbums = m_allAlbums;
    return;
  }

  m_filteredAlbums.clear();
  const QString searchLower =
      (hasSearch ? m_searchQuery : m_filterText).trimmed().toLower();
  const QString genreLower = m_genreFilter.trimmed().toLower();
  const QString artistLower = m_artistFilter.trimmed().toLower();
  const QString letterLower = m_letterFilter.trimmed().toLower();

  for (const auto &a : m_allAlbums) {
    if (m_hiResOnly) {
      if (!(a.maxBitDepth >= 24 || a.maxSampleRate >= 88200 ||
            a.primaryCodec == "DSD" || a.qualityBadge == "Hi-Res Lossless" ||
            a.qualityBadge == "DSD")) {
        continue;
      }
    }

    if (hasLetter) {
      bool letterMatch = false;
      QString titleStr = QString::fromStdString(a.title).trimmed();
      QString artistStr = QString::fromStdString(a.artist).trimmed();
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
      QString albumGenre = QString::fromStdString(a.genre).trimmed().toLower();
      if (albumGenre != genreLower && !albumGenre.contains(genreLower)) {
        continue;
      }
    }

    if (hasArtist) {
      QString albumArtist =
          QString::fromStdString(a.artist).trimmed().toLower();
      if (albumArtist != artistLower && !albumArtist.contains(artistLower)) {
        continue;
      }
    }

    if (hasSearch || hasLegacyFilter) {
      QString title = QString::fromStdString(a.title).toLower();
      QString artist = QString::fromStdString(a.artist).toLower();
      QString genre = QString::fromStdString(a.genre).toLower();
      if (!title.contains(searchLower) && !artist.contains(searchLower) &&
          !genre.contains(searchLower)) {
        continue;
      }
    }

    m_filteredAlbums.push_back(a);
  }
}

int AlbumListModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return static_cast<int>(m_filteredAlbums.size());
}

QVariant AlbumListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 ||
      index.row() >= static_cast<int>(m_filteredAlbums.size())) {
    return QVariant();
  }

  const auto &album = m_filteredAlbums[static_cast<size_t>(index.row())];

  switch (role) {
  case IdRole:
    return QString::fromStdString(album.id);
  case TitleRole:
    return QString::fromStdString(album.title);
  case ArtistRole:
    return QString::fromStdString(album.artist);
  case YearRole:
    return album.year > 0 ? QVariant(album.year) : QVariant();
  case GenreRole:
    return QString::fromStdString(album.genre);
  case TrackCountRole:
    return album.trackCount;
  case DurationRole:
    return QString::fromStdString(album.durationFormatted());
  case ArtHashRole:
    return QString::fromStdString(album.artHash);
  case CodecRole:
    return QString::fromStdString(album.primaryCodec);
  case SampleRateRole:
    return album.maxSampleRate;
  case BitDepthRole:
    return album.maxBitDepth;
  case QualityBadgeRole:
    return QString::fromStdString(album.qualityBadge);
  case AudiophileSummaryRole:
    return QString::fromStdString(album.audiophileSummary());
  default:
    return QVariant();
  }
}

QHash<int, QByteArray> AlbumListModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[IdRole] = "id";
  roles[TitleRole] = "title";
  roles[ArtistRole] = "artist";
  roles[YearRole] = "year";
  roles[GenreRole] = "genre";
  roles[TrackCountRole] = "trackCount";
  roles[DurationRole] = "durationFormatted";
  roles[ArtHashRole] = "artHash";
  roles[CodecRole] = "codec";
  roles[SampleRateRole] = "sampleRate";
  roles[BitDepthRole] = "bitDepth";
  roles[QualityBadgeRole] = "qualityBadge";
  roles[AudiophileSummaryRole] = "audiophileSummary";
  return roles;
}

QVariantMap AlbumListModel::getAlbumAt(int index) const {
  QVariantMap map;
  if (index < 0 || index >= static_cast<int>(m_filteredAlbums.size())) {
    return map;
  }
  const auto &album = m_filteredAlbums[static_cast<size_t>(index)];
  map["id"] = QString::fromStdString(album.id);
  map["title"] = QString::fromStdString(album.title);
  map["artist"] = QString::fromStdString(album.artist);
  map["year"] = album.year;
  map["genre"] = QString::fromStdString(album.genre);
  map["trackCount"] = album.trackCount;
  map["durationFormatted"] = QString::fromStdString(album.durationFormatted());
  map["artHash"] = QString::fromStdString(album.artHash);
  map["codec"] = QString::fromStdString(album.primaryCodec);
  map["sampleRate"] = album.maxSampleRate;
  map["bitDepth"] = album.maxBitDepth;
  map["qualityBadge"] = QString::fromStdString(album.qualityBadge);
  map["audiophileSummary"] = QString::fromStdString(album.audiophileSummary());
  return map;
}
