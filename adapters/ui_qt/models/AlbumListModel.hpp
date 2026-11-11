#pragma once

#include "core/entities/Album.hpp"
#include <QAbstractListModel>
#include <memory>
#include <vector>

namespace core {
class IMusicDatabasePort;
struct Track;
} // namespace core

/**
 * @brief Qt List Model for displaying and filtering album cards in QML
 * GridView.
 */
class AlbumListModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
  enum AlbumRoles {
    IdRole = Qt::UserRole + 1,
    TitleRole,
    ArtistRole,
    YearRole,
    GenreRole,
    TrackCountRole,
    DurationRole,
    ArtHashRole,
    CodecRole,
    SampleRateRole,
    BitDepthRole,
    QualityBadgeRole,
    AudiophileSummaryRole
  };

  explicit AlbumListModel(core::IMusicDatabasePort *dbPort = nullptr,
                          QObject *parent = nullptr);
  ~AlbumListModel() override = default;

  void setDatabasePort(core::IMusicDatabasePort *dbPort);

  [[nodiscard]] int
  rowCount(const QModelIndex &parent = QModelIndex()) const override;
  [[nodiscard]] QVariant data(const QModelIndex &index,
                              int role = Qt::DisplayRole) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

  Q_INVOKABLE void reload();
  Q_INVOKABLE void setFilter(const QString &filterText);
  Q_INVOKABLE void setSearchQuery(const QString &query);
  Q_INVOKABLE void setGenreFilter(const QString &genre);
  Q_INVOKABLE void setArtistFilter(const QString &artist);
  Q_INVOKABLE void setLetterFilter(const QString &letter);
  Q_INVOKABLE void setHiResOnly(bool hiResOnly);
  Q_INVOKABLE void resetAllFilters();

  void setAlbumsFromTracks(const std::vector<core::Track> &tracks);

  [[nodiscard]] QString searchQuery() const { return m_searchQuery; }
  [[nodiscard]] QString genreFilter() const { return m_genreFilter; }
  [[nodiscard]] QString artistFilter() const { return m_artistFilter; }
  [[nodiscard]] QString letterFilter() const { return m_letterFilter; }
  [[nodiscard]] bool hiResOnly() const { return m_hiResOnly; }
  [[nodiscard]] Q_INVOKABLE QVariantMap getAlbumAt(int index) const;

signals:
  void countChanged();

private:
  void applyFilter();

  core::IMusicDatabasePort *m_dbPort{nullptr};
  std::vector<core::Album> m_allAlbums;
  std::vector<core::Album> m_filteredAlbums;
  QString m_filterText;
  QString m_searchQuery;
  QString m_genreFilter;
  QString m_artistFilter;
  QString m_letterFilter;
  bool m_hiResOnly{false};
};
