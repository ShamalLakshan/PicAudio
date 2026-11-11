#pragma once

#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

/**
 * @brief Definition of a main panel view (columns, sorting, grouping, thumbnail
 * size, layout overrides).
 */
struct ViewDefinition {
  QString viewId;
  QString name;
  QString baseRendererType; // "tracks", "album_and_tracks", "covers",
                            // "artists", "jukebox"
  QStringList displayedColumns;
  QVariantList sortOrder;
  QString groupBy;
  int thumbnailSize{160};
  QVariantMap panelOverrides;
  bool isBuiltin{false};
};

/**
 * @brief Registry of built-in and custom saved workspace views with dynamic
 * panel layout overrides.
 */
class ViewRegistryModel : public QObject {
  Q_OBJECT

public:
  explicit ViewRegistryModel(QObject *parent = nullptr);
  ~ViewRegistryModel() override = default;

  // QML invokable API
  Q_INVOKABLE QVariantList getAllViews() const;
  Q_INVOKABLE QVariantMap getView(const QString &viewId) const;
  Q_INVOKABLE QString createCustomView(const QString &name,
                                       const QString &baseRendererType,
                                       const QString &sourceViewId = "");
  Q_INVOKABLE bool updateCustomView(const QString &viewId, const QString &name,
                                    const QStringList &columns,
                                    const QString &groupBy, int thumbnailSize,
                                    const QVariantMap &panelOverrides);
  Q_INVOKABLE bool deleteCustomView(const QString &viewId);
  Q_INVOKABLE QString duplicateView(const QString &viewId,
                                    const QString &newName);
  Q_INVOKABLE QVariantMap getPanelOverrides(const QString &viewId) const;
  Q_INVOKABLE QString getBaseRendererType(const QString &viewId) const;

  // Persistence
  void saveViews();
  void loadViews();

signals:
  void viewsChanged();

private:
  void registerBuiltinViews();
  QMap<QString, ViewDefinition> m_views;
};
