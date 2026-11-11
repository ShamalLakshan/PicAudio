/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <QDebug>
#include <QLoggingCategory>
#include <QMetaEnum>
#include <QString>

namespace PicAudioEngine::Utils::Enum {
template <typename Value>
concept IsEnum = std::is_enum_v<Value>;

template <typename E>
    requires IsEnum<E>
std::optional<E> fromString(const QString& text)
{
    bool ok;
    auto result = static_cast<E>(QMetaEnum::fromType<E>().keyToValue(text.toUtf8().constData(), &ok));
    if(!ok) {
        QLoggingCategory log{"Utils"};
        qCWarning(log) << "Failed to convert enum" << text;
        return {};
    }
    return result;
}

template <typename E>
    requires IsEnum<E>
QString toString(E value)
{
    const int intValue = static_cast<int>(value);
    return QString::fromUtf8(QMetaEnum::fromType<E>().valueToKey(intValue));
}
} // namespace PicAudioEngine::Utils::Enum
