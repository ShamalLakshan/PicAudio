/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <QString>

#include <chrono>
#include <optional>
#include <stop_token>

namespace PicAudioEngine {
struct NetworkStreamMetadata
{
    QString streamName;
    QString streamGenre;
    QString streamTitle;
    QString streamUrl;
    QString title;
    QString artist;
    QString album;
    int bitrateKbps{0};
    quint64 revision{0};
};

struct RemoteFileInfo
{
    qint64 size{-1};
    //! Modification time in milliseconds since epoch, or zero when unknown.
    quint64 modifiedTime{0};
};

//! Optional file properties
// TODO: Add to RemoteStreamDevice for 1.0
class PICAUDIO_ENGINE_EXPORT RemoteFileDevice
{
public:
    virtual ~RemoteFileDevice() = default;

    [[nodiscard]] virtual RemoteFileInfo remoteFileInfo() const = 0;
};

class PICAUDIO_ENGINE_EXPORT RemoteStreamDevice
{
public:
    static constexpr auto ReadWaitTimeout = std::chrono::milliseconds{50};

    virtual ~RemoteStreamDevice() = default;

    [[nodiscard]] virtual bool readWouldBlock() const         = 0;
    [[nodiscard]] virtual qsizetype bufferedByteCount() const = 0;
    virtual void setNonBlockingReadsEnabled(bool enabled)     = 0;
    virtual void setReconnectOnFinishedEnabled(bool enabled);

    virtual void setReadCancellationToken(std::stop_token token) = 0;

    [[nodiscard]] virtual bool shouldExposePathToDecoder() const;
    [[nodiscard]] virtual std::optional<NetworkStreamMetadata> remoteStreamMetadata() const;
};
} // namespace PicAudioEngine
