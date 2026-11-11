/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "treemodel.h"

namespace PicAudioEngine {
template <class Item>
class TableModel : public TreeModel<Item>
{
public:
    using TreeModel<Item>::rootItem;

    explicit TableModel(QObject* parent = nullptr)
        : TreeModel<Item>{parent}
    { }

    [[nodiscard]] QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override
    {
        if(!QAbstractItemModel::hasIndex(row, column, parent)) {
            return {};
        }

        Item* childItem = rootItem()->child(row);
        if(childItem) {
            return QAbstractItemModel::createIndex(row, column, childItem);
        }
        return {};
    }

    [[nodiscard]] QModelIndex parent(const QModelIndex& /*index*/) const override
    {
        return {};
    }
};
} // namespace PicAudioEngine
