/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <QString>
#include <QStringList>

#include <vector>

namespace PicAudioEngine {
template <typename Value>
class FlatStringMap
{
public:
    using Entry         = std::pair<QString, Value>;
    using Container     = std::vector<Entry>;
    using Iterator      = Container::iterator;
    using ConstIterator = Container::const_iterator;

    void reserve(std::size_t n)
    {
        m_data.reserve(n);
    }

    void clear()
    {
        m_data.clear();
    }

    [[nodiscard]] std::size_t size() const
    {
        return m_data.size();
    }

    [[nodiscard]] bool empty() const
    {
        return m_data.empty();
    }

    [[nodiscard]] Value* find(const QString& key)
    {
        auto it = lowerBound(key);
        if(it != m_data.end() && it->first == key) {
            return &it->second;
        }
        return nullptr;
    }

    [[nodiscard]] const Value* find(const QString& key) const
    {
        auto it = lowerBound(key);
        if(it != m_data.end() && it->first == key) {
            return &it->second;
        }
        return nullptr;
    }

    [[nodiscard]] bool contains(const QString& key) const
    {
        auto it = lowerBound(key);
        return it != m_data.end() && it->first == key;
    }

    [[nodiscard]] Value value(const QString& key) const
    {
        auto it = lowerBound(key);
        if(it != m_data.end() && it->first == key) {
            return it->second;
        }
        return {};
    }

    void insertOrAssign(QString key, Value value)
    {
        auto it = lowerBound(key);
        if(it != m_data.end() && it->first == key) {
            it->second = std::move(value);
        }
        else {
            m_data.insert(it, {std::move(key), std::move(value)});
        }
    }

    bool erase(const QString& key)
    {
        auto it = lowerBound(key);
        if(it != m_data.end() && it->first == key) {
            m_data.erase(it);
            return true;
        }
        return false;
    }

    [[nodiscard]] const std::vector<Entry>& entries() const
    {
        return m_data;
    }

    [[nodiscard]] ConstIterator begin() const
    {
        return m_data.begin();
    }

    [[nodiscard]] ConstIterator end() const
    {
        return m_data.end();
    }

    [[nodiscard]] ConstIterator cbegin() const
    {
        return m_data.cbegin();
    }

    [[nodiscard]] ConstIterator cend() const
    {
        return m_data.cend();
    }

private:
    Iterator lowerBound(const QString& key)
    {
        return std::lower_bound(m_data.begin(), m_data.end(), key,
                                [](const Entry& entry, const QString& k) { return entry.first < k; });
    }

    [[nodiscard]] ConstIterator lowerBound(const QString& key) const
    {
        return std::lower_bound(m_data.begin(), m_data.end(), key,
                                [](const Entry& entry, const QString& k) { return entry.first < k; });
    }

    Container m_data;
};
} // namespace PicAudioEngine