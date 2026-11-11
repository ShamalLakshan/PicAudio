#pragma once

#include "core/entities/Track.hpp"
#include <QAbstractListModel>
#include <memory>
#include <unordered_set>
#include <vector>

namespace core {
class IMusicDatabasePort;
}

/**
 * @brief Qt List Model for displaying tracks in library views and album detail
 * popups.
 */
class TrackListModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
  Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectionChanged)
  Q_PROPERTY(QString sortColumn READ sortColumn NOTIFY sortChanged)
  Q_PROPERTY(bool sortAscending READ sortAscending NOTIFY sortChanged)

public:
  enum TrackRoles {
    IdRole = Qt::UserRole + 1,
    FilePathRole,
    TitleRole,
    ArtistRole,
    AlbumRole,
    AlbumArtistRole,
    GenreRole,
    YearRole,
    TrackNumberRole,
    DiscNumberRole,
    DurationMsRole,
    DurationFormattedRole,
    SampleRateRole,
    BitDepthRole,
    ChannelsRole,
    BitrateRole,
    CodecRole,
    ArtHashRole,
    AudiophileBadgeRole,
    IsSelectedRole,
    FileSizeBytesRole,
    FileSizeFormattedRole,
    DateAddedRole,
    DateAddedFormattedRole,
    PlayCountRole,
    RatingRole,
    IsFavoriteRole,
    SkipCountRole,
    LastPlayedRole,
    LastPlayedFormattedRole,
    CommentRole
  };

  explicit TrackListModel(core::IMusicDatabasePort *dbPort = nullptr,
                          QObject *parent = nullptr);
  ~TrackListModel() override = default;

  void setDatabasePort(core::IMusicDatabasePort *dbPort);
  void setTracks(const std::vector<core::Track> &tracks);
  [[nodiscard]] const std::vector<core::Track> &tracks() const {
    return m_filteredTracks;
  }

  [[nodiscard]] int
  rowCount(const QModelIndex &parent = QModelIndex()) const override;
  [[nodiscard]] QVariant data(const QModelIndex &index,
                              int role = Qt::DisplayRole) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

  Q_INVOKABLE void loadAllTracks();
  Q_INVOKABLE void loadAlbumTracks(const QString &albumTitle,
                                   const QString &albumArtist);
  Q_INVOKABLE void setFilter(const QString &filterText);
  Q_INVOKABLE void setSearchQuery(const QString &query);
  Q_INVOKABLE void setGenreFilter(const QString &genre);
  Q_INVOKABLE void setArtistFilter(const QString &artist);
  Q_INVOKABLE void setAlbumFilter(const QString &album);
  Q_INVOKABLE void setLetterFilter(const QString &letter);
  Q_INVOKABLE void setHiResOnly(bool hiResOnly);
  Q_INVOKABLE void resetAllFilters();

  [[nodiscard]] QString searchQuery() const { return m_searchQuery; }
  [[nodiscard]] QString genreFilter() const { return m_genreFilter; }
  [[nodiscard]] QString artistFilter() const { return m_artistFilter; }
  [[nodiscard]] QString albumFilter() const { return m_albumFilter; }
  [[nodiscard]] QString letterFilter() const { return m_letterFilter; }
  [[nodiscard]] bool hiResOnly() const { return m_hiResOnly; }
  [[nodiscard]] Q_INVOKABLE QVariantMap getTrackAt(int index) const;

  // Selection management
  Q_INVOKABLE void selectAll();
  Q_INVOKABLE void invertSelection();
  Q_INVOKABLE void clearSelection();
  Q_INVOKABLE void selectRange(int fromIndex, int toIndex);
  Q_INVOKABLE void toggleSelection(int index);
  Q_INVOKABLE void setRowSelected(int index, bool selected);
  [[nodiscard]] Q_INVOKABLE bool isSelected(int index) const;
  [[nodiscard]] Q_INVOKABLE int selectedCount() const;
  [[nodiscard]] Q_INVOKABLE QVariantList getSelectedTrackIds() const;
  [[nodiscard]] Q_INVOKABLE QVariantList getSelectedTracks() const;
  Q_INVOKABLE void removeSelected();
  Q_INVOKABLE void setTrackRating(int index, int rating);
  Q_INVOKABLE void setTrackFavorite(int index, bool isFavorite);

  // Sorting
  [[nodiscard]] QString sortColumn() const { return m_sortColumn; }
  [[nodiscard]] bool sortAscending() const { return m_sortAscending; }
  Q_INVOKABLE void sortBy(const QString &columnKey, bool ascending = true);
  Q_INVOKABLE void toggleSort(const QString &columnKey);
  Q_INVOKABLE void clearSort();

signals:
  void countChanged();
  void selectionChanged();
  void sortChanged();

private:
  void applyFilter();
  void applySort();

  core::IMusicDatabasePort *m_dbPort{nullptr};
  std::vector<core::Track> m_allTracks;
  std::vector<core::Track> m_filteredTracks;
  std::unordered_set<int> m_selectedIndices;
  QString m_filterText;
  QString m_searchQuery;
  QString m_genreFilter;
  QString m_artistFilter;
  QString m_albumFilter;
  QString m_letterFilter;
  bool m_hiResOnly{false};
  QString m_sortColumn;
  bool m_sortAscending{true};
};
