#include "TabListModel.hpp"

TabListModel::TabListModel(core::TabService *tabService, QObject *parent)
    : QAbstractListModel(parent), m_tabService(tabService) {}

int TabListModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid() || !m_tabService) {
    return 0;
  }
  return static_cast<int>(m_tabService->getTabCount());
}

QVariant TabListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || !m_tabService) {
    return {};
  }

  int row = index.row();
  const auto *tab = m_tabService->getTab(static_cast<size_t>(row));
  if (!tab) {
    return {};
  }

  switch (role) {
  case TabIdRole:
    return QString::fromStdString(tab->tabId);
  case TitleRole:
  case Qt::DisplayRole:
    return QString::fromStdString(tab->title);
  case SourceIdRole:
    return QString::fromStdString(tab->sourceId);
  case ViewIdRole:
    return QString::fromStdString(tab->viewId);
  case LockModeRole:
    return static_cast<int>(tab->lockMode);
  case IsPinnedRole:
    return tab->isPinned;
  case IsActiveRole:
    return row == static_cast<int>(m_tabService->getActiveTabIndex());
  case RootSourceIdRole:
    return QString::fromStdString(tab->rootSourceId);
  case JumpBarLetterRole:
    return QString::fromStdString(tab->subFilterState.jumpBarLetter);
  default:
    return {};
  }
}

QHash<int, QByteArray> TabListModel::roleNames() const {
  return {{TabIdRole, "tabId"},
          {TitleRole, "title"},
          {SourceIdRole, "sourceId"},
          {ViewIdRole, "viewId"},
          {LockModeRole, "lockMode"},
          {IsPinnedRole, "isPinned"},
          {IsActiveRole, "isActive"},
          {RootSourceIdRole, "rootSourceId"},
          {JumpBarLetterRole, "jumpBarLetter"}};
}

int TabListModel::activeTabIndex() const {
  return m_tabService ? static_cast<int>(m_tabService->getActiveTabIndex()) : 0;
}

int TabListModel::tabCount() const {
  return m_tabService ? static_cast<int>(m_tabService->getTabCount()) : 0;
}

bool TabListModel::canReopenClosedTab() const {
  return m_tabService && m_tabService->canReopenClosedTab();
}

QString TabListModel::activeTabId() const {
  if (m_tabService && m_tabService->getActiveTab()) {
    return QString::fromStdString(m_tabService->getActiveTab()->tabId);
  }
  return {};
}

QString TabListModel::activeSourceId() const {
  if (m_tabService && m_tabService->getActiveTab()) {
    return QString::fromStdString(m_tabService->getActiveTab()->sourceId);
  }
  return {};
}

QString TabListModel::activeViewId() const {
  if (m_tabService && m_tabService->getActiveTab()) {
    return QString::fromStdString(m_tabService->getActiveTab()->viewId);
  }
  return {};
}

QString TabListModel::activeTitle() const {
  if (m_tabService && m_tabService->getActiveTab()) {
    return QString::fromStdString(m_tabService->getActiveTab()->title);
  }
  return {};
}

int TabListModel::activeLockMode() const {
  if (m_tabService && m_tabService->getActiveTab()) {
    return static_cast<int>(m_tabService->getActiveTab()->lockMode);
  }
  return 0;
}

bool TabListModel::isActiveTabLocked() const { return activeLockMode() != 0; }

bool TabListModel::activeIsPinned() const {
  if (m_tabService && m_tabService->getActiveTab()) {
    return m_tabService->getActiveTab()->isPinned;
  }
  return false;
}

bool TabListModel::setActiveTabIndex(int index) {
  if (!m_tabService || index < 0 ||
      static_cast<size_t>(index) >= m_tabService->getTabCount()) {
    return false;
  }
  int oldIndex = static_cast<int>(m_tabService->getActiveTabIndex());
  if (oldIndex == index) {
    return true;
  }

  if (m_tabService->setActiveTabIndex(static_cast<size_t>(index))) {
    emit dataChanged(this->index(oldIndex), this->index(oldIndex),
                     {IsActiveRole});
    emit dataChanged(this->index(index), this->index(index), {IsActiveRole});
    emit activeTabIndexChanged();
    emit activeTabStateChanged();

    const auto *cur = m_tabService->getActiveTab();
    if (cur) {
      emit tabSwitched(oldIndex, index, QString::fromStdString(cur->sourceId),
                       QString::fromStdString(cur->viewId));
    }
    return true;
  }
  return false;
}

QString TabListModel::openTab(const QString &sourceId, const QString &title,
                              const QString &viewId, int insertIndex) {
  if (!m_tabService) {
    return {};
  }

  size_t idx = (insertIndex < 0) ? m_tabService->getTabCount()
                                 : static_cast<size_t>(insertIndex);
  beginInsertRows(QModelIndex(), static_cast<int>(idx), static_cast<int>(idx));
  std::string newId = m_tabService->openTab(
      sourceId.toStdString(), title.toStdString(), viewId.toStdString(), idx);
  endInsertRows();

  emit tabCountChanged();
  emit activeTabIndexChanged();
  emit activeTabStateChanged();

  const auto *cur = m_tabService->getActiveTab();
  if (cur) {
    emit tabSwitched(-1, activeTabIndex(),
                     QString::fromStdString(cur->sourceId),
                     QString::fromStdString(cur->viewId));
  }

  return QString::fromStdString(newId);
}

bool TabListModel::closeTab(int index, bool force) {
  if (!m_tabService || index < 0 ||
      static_cast<size_t>(index) >= m_tabService->getTabCount()) {
    return false;
  }
  if (m_tabService->getTabCount() <= 1) {
    return false;
  }

  beginRemoveRows(QModelIndex(), index, index);
  bool ok = m_tabService->closeTab(static_cast<size_t>(index), force);
  endRemoveRows();

  if (ok) {
    emit tabCountChanged();
    emit closedTabsHistoryChanged();
    emit activeTabIndexChanged();
    emit activeTabStateChanged();
    const auto *cur = m_tabService->getActiveTab();
    if (cur) {
      emit tabSwitched(-1, activeTabIndex(),
                       QString::fromStdString(cur->sourceId),
                       QString::fromStdString(cur->viewId));
    }
  }
  return ok;
}

void TabListModel::closeOtherTabs(int keepIndex) {
  if (!m_tabService || keepIndex < 0 ||
      static_cast<size_t>(keepIndex) >= m_tabService->getTabCount()) {
    return;
  }
  beginResetModel();
  m_tabService->closeOtherTabs(static_cast<size_t>(keepIndex));
  endResetModel();

  emit tabCountChanged();
  emit closedTabsHistoryChanged();
  emit activeTabIndexChanged();
  emit activeTabStateChanged();

  const auto *cur = m_tabService->getActiveTab();
  if (cur) {
    emit tabSwitched(-1, activeTabIndex(),
                     QString::fromStdString(cur->sourceId),
                     QString::fromStdString(cur->viewId));
  }
}

QString TabListModel::duplicateTab(int index) {
  if (!m_tabService || index < 0 ||
      static_cast<size_t>(index) >= m_tabService->getTabCount()) {
    return {};
  }
  int insertPos = index + 1;
  beginInsertRows(QModelIndex(), insertPos, insertPos);
  std::string dupId = m_tabService->duplicateTab(static_cast<size_t>(index));
  endInsertRows();

  emit tabCountChanged();
  emit activeTabIndexChanged();
  emit activeTabStateChanged();

  return QString::fromStdString(dupId);
}

bool TabListModel::renameTab(int index, const QString &newTitle) {
  if (!m_tabService || index < 0 ||
      static_cast<size_t>(index) >= m_tabService->getTabCount()) {
    return false;
  }
  if (m_tabService->renameTab(static_cast<size_t>(index),
                              newTitle.toStdString())) {
    emit dataChanged(this->index(index), this->index(index),
                     {TitleRole, Qt::DisplayRole});
    if (index == activeTabIndex()) {
      emit activeTabStateChanged();
    }
    return true;
  }
  return false;
}

bool TabListModel::moveTab(int fromIndex, int toIndex) {
  if (!m_tabService || fromIndex < 0 || toIndex < 0 ||
      static_cast<size_t>(fromIndex) >= m_tabService->getTabCount() ||
      static_cast<size_t>(toIndex) >= m_tabService->getTabCount() ||
      fromIndex == toIndex) {
    return false;
  }

  int destRow = (toIndex > fromIndex) ? toIndex + 1 : toIndex;
  if (!beginMoveRows(QModelIndex(), fromIndex, fromIndex, QModelIndex(),
                     destRow)) {
    return false;
  }
  bool ok = m_tabService->moveTab(static_cast<size_t>(fromIndex),
                                  static_cast<size_t>(toIndex));
  endMoveRows();

  if (ok) {
    emit activeTabIndexChanged();
  }
  return ok;
}

bool TabListModel::setTabLockMode(int index, int mode) {
  if (!m_tabService || index < 0 ||
      static_cast<size_t>(index) >= m_tabService->getTabCount()) {
    return false;
  }
  if (m_tabService->setTabLockMode(static_cast<size_t>(index),
                                   static_cast<core::TabLockMode>(mode))) {
    emit dataChanged(this->index(index), this->index(index), {LockModeRole});
    if (index == activeTabIndex()) {
      emit activeTabStateChanged();
    }
    return true;
  }
  return false;
}

bool TabListModel::setTabPinned(int index, bool pinned) {
  if (!m_tabService || index < 0 ||
      static_cast<size_t>(index) >= m_tabService->getTabCount()) {
    return false;
  }
  if (m_tabService->setTabPinned(static_cast<size_t>(index), pinned)) {
    emit dataChanged(this->index(index), this->index(index), {IsPinnedRole});
    if (index == activeTabIndex()) {
      emit activeTabStateChanged();
    }
    return true;
  }
  return false;
}

bool TabListModel::setTabRootLock(int index, const QString &rootSourceId) {
  if (!m_tabService || index < 0 ||
      static_cast<size_t>(index) >= m_tabService->getTabCount()) {
    return false;
  }
  if (m_tabService->setTabRootLock(static_cast<size_t>(index),
                                   rootSourceId.toStdString())) {
    emit dataChanged(this->index(index), this->index(index),
                     {LockModeRole, RootSourceIdRole});
    if (index == activeTabIndex()) {
      emit activeTabStateChanged();
    }
    return true;
  }
  return false;
}

bool TabListModel::unlockTab(int index) {
  if (!m_tabService || index < 0 ||
      static_cast<size_t>(index) >= m_tabService->getTabCount()) {
    return false;
  }
  if (m_tabService->unlockTab(static_cast<size_t>(index))) {
    emit dataChanged(this->index(index), this->index(index),
                     {LockModeRole, IsPinnedRole, RootSourceIdRole});
    if (index == activeTabIndex()) {
      emit activeTabStateChanged();
    }
    return true;
  }
  return false;
}

bool TabListModel::resetTabToRoot(int index) {
  if (!m_tabService || index < 0 ||
      static_cast<size_t>(index) >= m_tabService->getTabCount()) {
    return false;
  }
  if (m_tabService->resetTabToRoot(static_cast<size_t>(index))) {
    emit dataChanged(this->index(index), this->index(index), {SourceIdRole});
    if (index == activeTabIndex()) {
      emit activeTabStateChanged();
      const auto *cur = m_tabService->getActiveTab();
      if (cur) {
        emit tabSwitched(index, index, QString::fromStdString(cur->sourceId),
                         QString::fromStdString(cur->viewId));
      }
    }
    return true;
  }
  return false;
}

QVariantMap TabListModel::routeNavigatorClick(const QString &targetSourceId,
                                              const QString &defaultTitle,
                                              bool forceNewTab) {
  QVariantMap res;
  if (!m_tabService) {
    res["type"] = "Denied";
    return res;
  }

  size_t countBefore = m_tabService->getTabCount();
  int oldActiveIndex = static_cast<int>(m_tabService->getActiveTabIndex());

  auto routeRes = m_tabService->routeNavigatorClick(
      targetSourceId.toStdString(), defaultTitle.toStdString(), forceNewTab);

  size_t countAfter = m_tabService->getTabCount();
  int newActiveIndex = static_cast<int>(m_tabService->getActiveTabIndex());

  if (countAfter > countBefore) {
    beginInsertRows(QModelIndex(), static_cast<int>(countAfter - 1),
                    static_cast<int>(countAfter - 1));
    endInsertRows();
    emit tabCountChanged();
  }

  emit activeTabIndexChanged();
  emit activeTabStateChanged();

  if (oldActiveIndex != newActiveIndex) {
    emit dataChanged(this->index(oldActiveIndex), this->index(oldActiveIndex),
                     {IsActiveRole});
    emit dataChanged(this->index(newActiveIndex), this->index(newActiveIndex),
                     {IsActiveRole, SourceIdRole, TitleRole});
  } else {
    emit dataChanged(this->index(newActiveIndex), this->index(newActiveIndex),
                     {SourceIdRole, TitleRole});
  }

  const auto *cur = m_tabService->getActiveTab();
  if (cur) {
    emit tabSwitched(oldActiveIndex, newActiveIndex,
                     QString::fromStdString(cur->sourceId),
                     QString::fromStdString(cur->viewId));
  }

  switch (routeRes.type) {
  case core::NavigationResultType::CurrentTabUpdated:
    res["type"] = "CurrentTabUpdated";
    break;
  case core::NavigationResultType::NavigatedToUnlockedTab:
    res["type"] = "NavigatedToUnlockedTab";
    break;
  case core::NavigationResultType::NewTabSpawned:
    res["type"] = "NewTabSpawned";
    break;
  default:
    res["type"] = "Denied";
    break;
  }

  res["targetTabIndex"] = static_cast<int>(routeRes.targetTabIndex);
  res["targetTabId"] = QString::fromStdString(routeRes.targetTabId);
  return res;
}

bool TabListModel::updateActiveView(const QString &viewId) {
  if (!m_tabService)
    return false;
  if (m_tabService->updateActiveView(viewId.toStdString())) {
    emit dataChanged(this->index(activeTabIndex()),
                     this->index(activeTabIndex()), {ViewIdRole});
    emit activeTabStateChanged();
    const auto *cur = m_tabService->getActiveTab();
    if (cur) {
      emit tabSwitched(activeTabIndex(), activeTabIndex(),
                       QString::fromStdString(cur->sourceId),
                       QString::fromStdString(cur->viewId));
    }
    return true;
  }
  return false;
}

bool TabListModel::updateActiveSource(const QString &sourceId,
                                      const QString &title) {
  if (!m_tabService)
    return false;
  if (m_tabService->updateActiveSource(sourceId.toStdString(),
                                       title.toStdString())) {
    emit dataChanged(this->index(activeTabIndex()),
                     this->index(activeTabIndex()), {SourceIdRole, TitleRole});
    emit activeTabStateChanged();
    const auto *cur = m_tabService->getActiveTab();
    if (cur) {
      emit tabSwitched(activeTabIndex(), activeTabIndex(),
                       QString::fromStdString(cur->sourceId),
                       QString::fromStdString(cur->viewId));
    }
    return true;
  }
  return false;
}

bool TabListModel::updateActiveJumpBarLetter(const QString &letter) {
  if (!m_tabService)
    return false;
  auto *tab = m_tabService->getActiveTab();
  if (!tab)
    return false;
  tab->subFilterState.jumpBarLetter = letter.toStdString();
  emit dataChanged(this->index(activeTabIndex()), this->index(activeTabIndex()),
                   {JumpBarLetterRole});
  return true;
}

bool TabListModel::updateActiveScrollOffset(double offset) {
  if (!m_tabService)
    return false;
  return m_tabService->updateActiveScrollOffset(offset);
}

double TabListModel::getActiveScrollOffset() const {
  if (!m_tabService || !m_tabService->getActiveTab())
    return 0.0;
  return m_tabService->getActiveTab()->scrollOffset;
}

QString TabListModel::reopenLastClosedTab() {
  if (!m_tabService || !m_tabService->canReopenClosedTab()) {
    return {};
  }
  size_t targetIndex = m_tabService->getTabCount();
  beginInsertRows(QModelIndex(), static_cast<int>(targetIndex),
                  static_cast<int>(targetIndex));
  auto optId = m_tabService->reopenLastClosedTab();
  endInsertRows();

  if (optId.has_value()) {
    emit tabCountChanged();
    emit closedTabsHistoryChanged();
    emit activeTabIndexChanged();
    emit activeTabStateChanged();
    const auto *cur = m_tabService->getActiveTab();
    if (cur) {
      emit tabSwitched(-1, activeTabIndex(),
                       QString::fromStdString(cur->sourceId),
                       QString::fromStdString(cur->viewId));
    }
    return QString::fromStdString(*optId);
  }
  return {};
}

void TabListModel::refreshAll() {
  beginResetModel();
  endResetModel();
  emit tabCountChanged();
  emit activeTabIndexChanged();
  emit activeTabStateChanged();
}
