#pragma once

#include <QAbstractListModel>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

/**
 * @brief Representation of an individual modular UI element assigned to a zone.
 */
struct LayoutElement {
  QString elementId;
  QString name;
  QString defaultZoneId;
  bool isStacked{false};
  QString stackMode{"sub_tabs"}; // "sub_tabs" | "header_cycle"
  int activeStackIndex{0};
  double splitWeight{1.0};
};

/**
 * @brief Representation of one of the 9 structural docking zones.
 */
struct LayoutZone {
  QString zoneId;
  QString name;
  QString mode{"permanent"}; // "permanent" | "hidden" | "auto_open" (for
                             // sidebars) or "visible" | "hidden" (for panels)
  double dimension{
      240.0}; // Width for vertical zones, Height for horizontal zones
  QList<LayoutElement> elements;
};

/**
 * @brief Manages 9-zone layout docking, element assignment, stacking,
 * per-view layout overrides, auto-open edge hover, and profile persistence.
 */
class LayoutModel : public QObject {
  Q_OBJECT

  // Effective Zone Visibilities / Modes (including per-tab/view overrides)
  Q_PROPERTY(QString effectiveLeftSidebarMode READ effectiveLeftSidebarMode
                 NOTIFY layoutChanged)
  Q_PROPERTY(bool effectiveLeftMainPanelVisible READ
                 effectiveLeftMainPanelVisible NOTIFY layoutChanged)
  Q_PROPERTY(bool effectiveRightMainPanelVisible READ
                 effectiveRightMainPanelVisible NOTIFY layoutChanged)
  Q_PROPERTY(QString effectiveRightSidebarMode READ effectiveRightSidebarMode
                 NOTIFY layoutChanged)
  Q_PROPERTY(bool effectiveTopPanelVisible READ effectiveTopPanelVisible NOTIFY
                 layoutChanged)
  Q_PROPERTY(bool effectiveBottomPanelVisible READ effectiveBottomPanelVisible
                 NOTIFY layoutChanged)
  Q_PROPERTY(bool effectiveHeaderBarVisible READ effectiveHeaderBarVisible
                 NOTIFY layoutChanged)
  Q_PROPERTY(bool isFloatingWindowActive READ isFloatingWindowActive NOTIFY
                 layoutChanged)

  // Dimensions
  Q_PROPERTY(double leftSidebarWidth READ leftSidebarWidth WRITE
                 setLeftSidebarWidth NOTIFY layoutChanged)
  Q_PROPERTY(double leftMainPanelWidth READ leftMainPanelWidth WRITE
                 setLeftMainPanelWidth NOTIFY layoutChanged)
  Q_PROPERTY(double rightMainPanelWidth READ rightMainPanelWidth WRITE
                 setRightMainPanelWidth NOTIFY layoutChanged)
  Q_PROPERTY(double rightSidebarWidth READ rightSidebarWidth WRITE
                 setRightSidebarWidth NOTIFY layoutChanged)
  Q_PROPERTY(double topPanelHeight READ topPanelHeight WRITE setTopPanelHeight
                 NOTIFY layoutChanged)
  Q_PROPERTY(double bottomPanelHeight READ bottomPanelHeight WRITE
                 setBottomPanelHeight NOTIFY layoutChanged)

  // In-Main-Panel component visibility overrides
  Q_PROPERTY(bool columnBrowserVisible READ columnBrowserVisible WRITE
                 setColumnBrowserVisible NOTIFY layoutChanged)
  Q_PROPERTY(bool jumpBarVisible READ jumpBarVisible WRITE setJumpBarVisible
                 NOTIFY layoutChanged)

  // Current Active Layout Preset
  Q_PROPERTY(QString currentPreset READ currentPreset NOTIFY layoutChanged)

public:
  explicit LayoutModel(QObject *parent = nullptr);
  ~LayoutModel() override = default;

  // Effective state queries
  [[nodiscard]] QString effectiveLeftSidebarMode() const;
  [[nodiscard]] bool effectiveLeftMainPanelVisible() const;
  [[nodiscard]] bool effectiveRightMainPanelVisible() const;
  [[nodiscard]] QString effectiveRightSidebarMode() const;
  [[nodiscard]] bool effectiveTopPanelVisible() const;
  [[nodiscard]] bool effectiveBottomPanelVisible() const;
  [[nodiscard]] bool effectiveHeaderBarVisible() const;
  [[nodiscard]] bool isFloatingWindowActive() const;

  [[nodiscard]] double leftSidebarWidth() const;
  [[nodiscard]] double leftMainPanelWidth() const;
  [[nodiscard]] double rightMainPanelWidth() const;
  [[nodiscard]] double rightSidebarWidth() const;
  [[nodiscard]] double topPanelHeight() const;
  [[nodiscard]] double bottomPanelHeight() const;

  [[nodiscard]] bool columnBrowserVisible() const;
  [[nodiscard]] bool jumpBarVisible() const;
  [[nodiscard]] QString currentPreset() const;

  void setLeftSidebarWidth(double width);
  void setLeftMainPanelWidth(double width);
  void setRightMainPanelWidth(double width);
  void setRightSidebarWidth(double width);
  void setTopPanelHeight(double height);
  void setBottomPanelHeight(double height);
  void setColumnBrowserVisible(bool visible);
  void setJumpBarVisible(bool visible);

  // QML invokable API for Arrange Panels configuration
  Q_INVOKABLE QVariantList getZones() const;
  Q_INVOKABLE QVariantList getElementsForZone(const QString &zoneId) const;
  Q_INVOKABLE QVariantList getAllAvailableElements() const;
  Q_INVOKABLE bool hasElement(const QString &zoneId,
                              const QString &elementId) const;

  Q_INVOKABLE void assignElement(const QString &elementId,
                                 const QString &zoneId, int index = -1,
                                 bool stacked = false);
  Q_INVOKABLE void removeElement(const QString &elementId);
  Q_INVOKABLE void moveElement(const QString &elementId,
                               const QString &targetZoneId, int targetIndex);
  Q_INVOKABLE void setElementStacked(const QString &elementId, bool stacked,
                                     const QString &stackMode = "sub_tabs");
  Q_INVOKABLE void setZoneMode(const QString &zoneId, const QString &mode);
  Q_INVOKABLE QString getZoneMode(const QString &zoneId) const;
  Q_INVOKABLE void setZoneDimension(const QString &zoneId, double dimension);
  Q_INVOKABLE double getZoneDimension(const QString &zoneId,
                                      double defaultDim = 240.0) const;

  // Stacking active item selection
  Q_INVOKABLE void setElementActiveStackIndex(const QString &elementId,
                                              int stackIndex);
  Q_INVOKABLE int getElementActiveStackIndex(const QString &elementId) const;

  // Per-Tab / Per-View Layout Overrides
  Q_INVOKABLE void applyLayoutOverrides(const QVariantMap &overrides);
  Q_INVOKABLE void clearLayoutOverrides();

  // Preset Layout Configurations
  Q_INVOKABLE QVariantList getAvailablePresets() const;
  Q_INVOKABLE void applyPreset(const QString &presetId);

  // Reset & Persistence
  Q_INVOKABLE void resetToDefaultLayout();
  Q_INVOKABLE void saveLayoutProfile(const QString &profileName = "default");
  Q_INVOKABLE bool loadLayoutProfile(const QString &profileName = "default");

signals:
  void layoutChanged();
  void overridesApplied();
  void presetApplied(const QString &presetId, const QString &recommendedViewId);

private:
  void initializeDefaultZones();
  void registerAllModularElements();
  [[nodiscard]] LayoutZone *findZone(const QString &zoneId);
  [[nodiscard]] const LayoutZone *findZone(const QString &zoneId) const;
  [[nodiscard]] LayoutElement *findElement(const QString &elementId);

  QMap<QString, LayoutZone> m_zones;
  QMap<QString, LayoutElement> m_allElements;
  QVariantMap m_activeOverrides;
  QString m_currentPreset{"studio"};
};
