/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include <utils/crypto.h>

#include <QDateTime>
#include <QRandomGenerator>
#include <QUuid>

namespace PicAudioEngine::Utils {
QString generateUniqueHash()
{
    return QUuid::createUuid().toString(QUuid::Id128);
}
} // namespace PicAudioEngine::Utils
