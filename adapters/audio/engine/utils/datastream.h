/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fyutils_export.h"

#include <QDataStream>

#include <set>
#include <vector>

PICAUDIO_UTILS_EXPORT QDataStream& operator<<(QDataStream& stream, const std::vector<int>& vec);
PICAUDIO_UTILS_EXPORT QDataStream& operator>>(QDataStream& stream, std::vector<int>& vec);

PICAUDIO_UTILS_EXPORT QDataStream& operator<<(QDataStream& stream, const std::set<int>& vec);
PICAUDIO_UTILS_EXPORT QDataStream& operator>>(QDataStream& stream, std::set<int>& vec);

PICAUDIO_UTILS_EXPORT QDataStream& operator<<(QDataStream& stream, const std::vector<int16_t>& vec);
PICAUDIO_UTILS_EXPORT QDataStream& operator>>(QDataStream& stream, std::vector<int16_t>& vec);

PICAUDIO_UTILS_EXPORT QDataStream& operator<<(QDataStream& stream, const std::vector<uint64_t>& vec);
PICAUDIO_UTILS_EXPORT QDataStream& operator>>(QDataStream& stream, std::vector<uint64_t>& vec);

PICAUDIO_UTILS_EXPORT QDataStream& operator<<(QDataStream& stream, const std::vector<QByteArray>& vec);
PICAUDIO_UTILS_EXPORT QDataStream& operator>>(QDataStream& stream, std::vector<QByteArray>& vec);

namespace PicAudioEngine::DataStream {
template <class C>
using value_t = std::ranges::range_value_t<C>;

template <class C>
constexpr bool has_reserve_v = requires(C& c, size_t n) { c.reserve(n); };

template <class C, class V>
constexpr bool has_push_back_v = requires(C& c, V&& v) { c.push_back(std::forward<V>(v)); };

template <class To, class From>
To maybeCast(From&& v)
{
    if constexpr(std::is_same_v<std::remove_cvref_t<To>, std::remove_cvref_t<From>>) {
        return std::forward<From>(v);
    }
    else {
        return static_cast<To>(std::forward<From>(v));
    }
}

template <class C, class V>
void insertValue(C& c, V&& v)
{
    if constexpr(has_push_back_v<C, V>) {
        c.push_back(std::forward<V>(v));
    }
    else {
        c.insert(std::forward<V>(v));
    }
}

template <class Container, class Wire = value_t<Container>>
QDataStream& writeContainer(QDataStream& s, const Container& c)
{
    s << static_cast<quint32>(c.size());
    for(const auto& v : c) {
        s << maybeCast<Wire>(v);
    }
    return s;
}

template <class Container, class Wire = value_t<Container>>
QDataStream& readContainer(QDataStream& s, Container& c)
{
    quint32 n{0};
    s >> n;

    c.clear();
    if constexpr(has_reserve_v<Container>) {
        c.reserve(static_cast<size_t>(n));
    }

    for(quint32 i{0}; i < n; ++i) {
        Wire w{};
        s >> w;
        if(s.status() != QDataStream::Ok) {
            c.clear();
            return s;
        }
        insertValue(c, maybeCast<value_t<Container>>(w));
    }

    return s;
}
} // namespace PicAudioEngine::DataStream
