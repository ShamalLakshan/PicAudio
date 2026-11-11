/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fyutils_export.h"

#include <utils/id.h>

#include <QCryptographicHash>
#include <QString>

namespace PicAudioEngine {
using Md5Hash = QByteArray;

namespace Utils {
namespace Detail {
inline QCryptographicHash& md5Hash()
{
    thread_local QCryptographicHash hash{QCryptographicHash::Md5};
    hash.reset();
    return hash;
}
} // namespace Detail

template <typename T>
void addDataToHash(QCryptographicHash& hash, const T& arg)
{
    hash.addData(arg.toUtf8());
}

template <>
inline void addDataToHash(QCryptographicHash& hash, const QByteArray& arg)
{
    hash.addData(arg);
}

template <typename... Args>
QString generateHash(const Args&... args)
{
    QCryptographicHash& hash = Detail::md5Hash();
    (addDataToHash(hash, args), ...);

    return QString::fromUtf8(hash.result().toHex());
}

template <typename... Args>
Md5Hash generateMd5Hash(const Args&... args)
{
    QCryptographicHash& hash = Detail::md5Hash();
    (addDataToHash(hash, args), ...);
    return hash.result();
}

PICAUDIO_UTILS_EXPORT QString generateUniqueHash();
} // namespace Utils
} // namespace PicAudioEngine
