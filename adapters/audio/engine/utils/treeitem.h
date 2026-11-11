/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "helpers.h"

#include <vector>

namespace PicAudioEngine {
template <class Item>
class TreeItem
{
public:
    explicit TreeItem(Item* parent = nullptr)
        : m_parent{parent}
    { }

    virtual ~TreeItem()
    {
        m_children.clear();
    }

    virtual bool hasChild(Item* child) const
    {
        return Utils::contains(m_children, child);
    }

    virtual std::vector<Item*> children() const
    {
        return m_children;
    }

    virtual void appendChild(Item* child)
    {
        child->m_row = static_cast<int>(m_children.size());
        m_children.emplace_back(child);
        child->m_parent = static_cast<Item*>(this);
    }

    virtual void insertChild(int row, Item* child)
    {
        row = std::min(row, static_cast<int>(m_children.size()));
        m_children.insert(m_children.begin() + row, child);
        child->m_parent = static_cast<Item*>(this);
        resetChildRows(row);
    }

    virtual void moveChild(int oldRow, int newRow)
    {
        if(oldRow < newRow) {
            Utils::move(m_children, oldRow, newRow - 1);
        }
        else {
            Utils::move(m_children, oldRow, newRow);
        }
        resetChildRows(std::min(oldRow, newRow));
    }

    virtual bool removeChild(int index)
    {
        if(index < 0 || index >= childCount()) {
            return false;
        }
        m_children.erase(m_children.cbegin() + index);
        resetChildRows(index);
        return true;
    }

    virtual void clearChildren()
    {
        m_children.clear();
    }

    virtual Item* child(int index) const
    {
        if(index < 0 || index >= childCount()) {
            return nullptr;
        }
        return m_children.at(index);
    }

    [[nodiscard]] virtual int childCount() const
    {
        return static_cast<int>(m_children.size());
    }

    [[nodiscard]] virtual int row() const
    {
        if(m_row < 0 && m_parent) {
            m_row = Utils::findIndex(m_parent->m_children, this);
        }
        return m_row;
    }

    [[nodiscard]] virtual Item* parent() const
    {
        return m_parent;
    }

    virtual void resetRow()
    {
        m_row = -1;
    }

    virtual void resetChildren()
    {
        for(Item* child : m_children) {
            if(child) {
                child->resetChildren();
            }
        }
        resetChildRows();
    }

    template <typename SortFunc>
    void sortChildren(SortFunc&& func)
    {
        for(auto* child : m_children) {
            child->sortChildren(std::forward<SortFunc>(func));
            child->resetRow();
        }

        std::ranges::sort(m_children, std::forward<SortFunc>(func));
        resetChildRows();
    }

    void sortChildren()
    {
        sortChildren([](const Item* a, const Item* b) {
            if(!a || !b) {
                return false;
            }
            return *a < *b;
        });
    }

private:
    friend Item;

    void resetChildRows(int start = 0)
    {
        const int count = childCount();
        for(int row{std::max(start, 0)}; row < count; ++row) {
            if(Item* child = m_children.at(row)) {
                child->m_row = row;
            }
        }
    }

    Item* m_parent;                // Not owned
    std::vector<Item*> m_children; // Not owned
    mutable int m_row{-1};
};
} // namespace PicAudioEngine
