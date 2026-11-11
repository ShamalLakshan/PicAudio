/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include <utils/datastream.h>

#include <QByteArray>
#include <QDataStream>
#include <vector>

QDataStream& operator<<(QDataStream& stream, const std::vector<int>& vec)
{
    return PicAudioEngine::DataStream::writeContainer<std::vector<int>, qint32>(stream, vec);
}

QDataStream& operator>>(QDataStream& stream, std::vector<int>& vec)
{
    return PicAudioEngine::DataStream::readContainer<std::vector<int>, qint32>(stream, vec);
}

QDataStream& operator<<(QDataStream& stream, const std::set<int>& vec)
{
    return PicAudioEngine::DataStream::writeContainer<std::set<int>, qint32>(stream, vec);
}

QDataStream& operator>>(QDataStream& stream, std::set<int>& vec)
{
    return PicAudioEngine::DataStream::readContainer<std::set<int>, qint32>(stream, vec);
}

QDataStream& operator<<(QDataStream& stream, const std::vector<int16_t>& vec)
{
    return PicAudioEngine::DataStream::writeContainer<std::vector<int16_t>, qint16>(stream, vec);
}

QDataStream& operator>>(QDataStream& stream, std::vector<int16_t>& vec)
{
    return PicAudioEngine::DataStream::readContainer<std::vector<int16_t>, qint16>(stream, vec);
}

QDataStream& operator<<(QDataStream& stream, const std::vector<uint64_t>& vec)
{
    return PicAudioEngine::DataStream::writeContainer<std::vector<uint64_t>, quint64>(stream, vec);
}

QDataStream& operator>>(QDataStream& stream, std::vector<uint64_t>& vec)
{
    return PicAudioEngine::DataStream::readContainer<std::vector<uint64_t>, quint64>(stream, vec);
}

QDataStream& operator<<(QDataStream& stream, const std::vector<QByteArray>& vec)
{
    return PicAudioEngine::DataStream::writeContainer(stream, vec);
}

QDataStream& operator>>(QDataStream& stream, std::vector<QByteArray>& vec)
{
    return PicAudioEngine::DataStream::readContainer(stream, vec);
}
