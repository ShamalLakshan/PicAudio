/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <utils/treeitem.h>

#include <QFont>

namespace PicAudioEngine {
template <class Item>
class TreeStatusItem : public TreeItem<Item>
{
public:
    enum ItemStatus
    {
        None = Qt::UserRole + 50,
        Added,
        Removed,
        Changed
    };

    explicit TreeStatusItem(Item* parent)
        : TreeItem<Item>{parent}
    { }

    [[nodiscard]] ItemStatus status() const
    {
        return m_status;
    };

    void setStatus(ItemStatus status)
    {
        m_status = status;
    };

    [[nodiscard]] QFont font() const
    {
        QFont font;
        switch(m_status) {
            case(Added):
                font.setItalic(true);
                break;
            case(Removed):
                font.setStrikeOut(true);
                break;
            case(Changed):
                font.setBold(true);
                break;
            case(None):
                break;
        }
        return font;
    }

private:
    ItemStatus m_status{None};
};
} // namespace PicAudioEngine
