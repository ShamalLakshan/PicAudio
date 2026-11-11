#pragma once

#include "core/services/TabService.hpp"
#include <QAbstractListModel>
#include <memory>

class TabListModel : public QAbstractListModel {
  Q_OBJECT

  Q_PROPERTY(int activeTabIndex READ activeTabIndex WRITE setActiveTabIndex
                 NOTIFY activeTabIndexChanged)
  Q_PROPERTY(int tabCount READ tabCount NOTIFY tabCountChanged)
  Q_PROPERTY(bool canReopenClosedTab READ canReopenClosedTab NOTIFY
                 closedTabsHistoryChanged)
  Q_PROPERTY(QString activeTabId READ activeTabId NOTIFY activeTabIndexChanged)
  Q_PROPERTY(
      QString activeSourceId READ activeSourceId NOTIFY activeTabStateChanged)
  Q_PROPERTY(
      QString activeViewId READ activeViewId NOTIFY activeTabStateChanged)
  Q_PROPERTY(QString activeTitle READ activeTitle NOTIFY activeTabStateChanged)
  Q_PROPERTY(
      int activeLockMode READ activeLockMode NOTIFY activeTabStateChanged)
  Q_PROPERTY(
      bool activeIsPinned READ activeIsPinned NOTIFY activeTabStateChanged)

public:
  enum TabRoles {
    TabIdRole = Qt::UserRole + 1,
    TitleRole,
    SourceIdRole,
    ViewIdRole,
    LockModeRole,
    IsPinnedRole,
    IsActiveRole,
    RootSourceIdRole,
    JumpBarLetterRole
  };
  Q_ENUM(TabRoles)

  explicit TabListModel(core::TabService *tabService,
                        QObject *parent = nullptr);
  ~TabListModel() override = default;

  [[nodiscard]] int
  rowCount(const QModelIndex &parent = QModelIndex()) const override;
  [[nodiscard]] QVariant data(const QModelIndex &index,
                              int role = Qt::DisplayRole) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

  [[nodiscard]] int activeTabIndex() const;
  [[nodiscard]] int tabCount() const;
  [[nodiscard]] bool canReopenClosedTab() const;
  [[nodiscard]] QString activeTabId() const;
  [[nodiscard]] QString activeSourceId() const;
  [[nodiscard]] QString activeViewId() const;
  [[nodiscard]] QString activeTitle() const;
  [[nodiscard]] int activeLockMode() const;
  [[nodiscard]] bool isActiveTabLocked() const;
  [[nodiscard]] bool activeIsPinned() const;

  // QML invokables
  Q_INVOKABLE bool setActiveTabIndex(int index);
  Q_INVOKABLE QString openTab(const QString &sourceId = "library://music",
                              const QString &title = "Music",
                              const QString &viewId = "tracks_default",
                              int insertIndex = -1);
  Q_INVOKABLE bool closeTab(int index, bool force = false);
  Q_INVOKABLE void closeOtherTabs(int keepIndex);
  Q_INVOKABLE QString duplicateTab(int index);
  Q_INVOKABLE bool renameTab(int index, const QString &newTitle);
  Q_INVOKABLE bool moveTab(int fromIndex, int toIndex);

  Q_INVOKABLE bool setTabLockMode(int index, int mode);
  Q_INVOKABLE bool setTabPinned(int index, bool pinned);
  Q_INVOKABLE bool setTabRootLock(int index, const QString &rootSourceId);
  Q_INVOKABLE bool unlockTab(int index);
  Q_INVOKABLE bool resetTabToRoot(int index);

  Q_INVOKABLE QVariantMap routeNavigatorClick(const QString &targetSourceId,
                                              const QString &defaultTitle,
                                              bool forceNewTab = false);

  Q_INVOKABLE bool updateActiveView(const QString &viewId);
  Q_INVOKABLE bool updateActiveSource(const QString &sourceId,
                                      const QString &title = "");
  Q_INVOKABLE bool updateActiveJumpBarLetter(const QString &letter);
  Q_INVOKABLE bool updateActiveScrollOffset(double offset);
  Q_INVOKABLE double getActiveScrollOffset() const;
  Q_INVOKABLE QString reopenLastClosedTab();

  void refreshAll();

signals:
  void activeTabIndexChanged();
  void tabCountChanged();
  void closedTabsHistoryChanged();
  void activeTabStateChanged();
  void tabSwitched(int oldIndex, int newIndex, const QString &sourceId,
                   const QString &viewId);

private:
  core::TabService *m_tabService{nullptr};
};
