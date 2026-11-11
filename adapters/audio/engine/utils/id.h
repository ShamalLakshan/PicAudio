/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fyutils_export.h"

#include <QMetaType>
#include <QString>
#include <QUuid>

#include <set>
#include <vector>

namespace PicAudioEngine {
class PICAUDIO_UTILS_EXPORT UId : public QUuid
{
public:
    struct UIdHash
    {
        std::size_t operator()(const UId& id) const
        {
            return static_cast<std::size_t>(qHash(id));
        }
    };

    UId() = default;
    explicit UId(const QUuid& id);

    static UId create();

    [[nodiscard]] bool isValid() const;
};

class PICAUDIO_UTILS_EXPORT Id
{
public:
    struct IdHash
    {
        size_t operator()(const Id& id) const
        {
            return (std::hash<unsigned int>{}(id.m_id)) ^ (std::hash<QString>{}(id.m_name) << 1);
        }
    };

    Id() = default;
    explicit Id(const QString& name);
    Id(const char* name);
    ~Id() = default;

    std::strong_ordering operator<=>(const Id& id) const = default;

    [[nodiscard]] bool isValid() const;

    [[nodiscard]] uint32_t id() const;
    [[nodiscard]] QString name() const;

    Id append(const Id& id);
    Id append(const QString& str);
    Id append(const char* str);
    Id append(int num);
    Id append(uintptr_t addr);

private:
    friend PICAUDIO_UTILS_EXPORT QDataStream& operator<<(QDataStream& stream, const Id& id);
    friend PICAUDIO_UTILS_EXPORT QDataStream& operator>>(QDataStream& stream, Id& id);

    uint32_t m_id;
    QString m_name;
};
PICAUDIO_UTILS_EXPORT size_t qHash(const Id& id);

using IdList = std::vector<Id>;
using IdSet  = std::set<Id>;

PICAUDIO_UTILS_EXPORT QDataStream& operator<<(QDataStream& stream, const IdSet& ids);
PICAUDIO_UTILS_EXPORT QDataStream& operator>>(QDataStream& stream, IdSet& ids);
} // namespace PicAudioEngine
