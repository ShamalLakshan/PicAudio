/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fyutils_export.h"

#include <QModelIndexList>
#include <QString>
#include <QStringList>

#include <functional>
#include <vector>

class QTreeView;

namespace PicAudioEngine::Utils {
using IndexRangeList = std::vector<QModelIndexList>;

PICAUDIO_UTILS_EXPORT IndexRangeList getIndexRanges(const QModelIndexList& indexes);
PICAUDIO_UTILS_EXPORT bool sortModelIndexes(const QModelIndex& index1, const QModelIndex& index2);

PICAUDIO_UTILS_EXPORT void recursiveDataChanged(QAbstractItemModel* model, const QModelIndex& parent,
                                         const QList<int>& roles = {});

using ModelIndexKey = std::function<QString(const QModelIndex&)>;

PICAUDIO_UTILS_EXPORT QString modelIndexPath(const QModelIndex& index);
PICAUDIO_UTILS_EXPORT QStringList saveExpansionState(const QTreeView* view);
PICAUDIO_UTILS_EXPORT QStringList saveExpansionState(const QTreeView* view, const ModelIndexKey& keyForIndex);
PICAUDIO_UTILS_EXPORT QStringList updateExpansionState(const QTreeView* view, const QStringList& currentState);
PICAUDIO_UTILS_EXPORT QStringList updateExpansionState(const QTreeView* view, const QStringList& currentState,
                                                const ModelIndexKey& keyForIndex);
PICAUDIO_UTILS_EXPORT void restoreExpansionState(QTreeView* view, const QStringList& expandedIndexes);
PICAUDIO_UTILS_EXPORT void restoreExpansionState(QTreeView* view, const QStringList& expandedIndexes,
                                          const ModelIndexKey& keyForIndex);

PICAUDIO_UTILS_EXPORT QStringList saveCollapsedExpansionState(const QTreeView* view);
PICAUDIO_UTILS_EXPORT QStringList saveCollapsedExpansionState(const QTreeView* view, const ModelIndexKey& keyForIndex);
PICAUDIO_UTILS_EXPORT QStringList updateCollapsedExpansionState(const QTreeView* view, const QStringList& currentState);
PICAUDIO_UTILS_EXPORT QStringList updateCollapsedExpansionState(const QTreeView* view, const QStringList& currentState,
                                                         const ModelIndexKey& keyForIndex);
PICAUDIO_UTILS_EXPORT void restoreCollapsedExpansionState(QTreeView* view, const QStringList& collapsedIndexes);
PICAUDIO_UTILS_EXPORT void restoreCollapsedExpansionState(QTreeView* view, const QStringList& collapsedIndexes,
                                                   const ModelIndexKey& keyForIndex);
} // namespace PicAudioEngine::Utils
