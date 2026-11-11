/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <chrono>
#include <memory>

class QIODevice;
class QUrl;

namespace PicAudioEngine {
class RemoteStreamDevice;

struct RemoteStreamSource
{
    std::unique_ptr<QIODevice> device;
    RemoteStreamDevice* remoteDevice{nullptr};
    std::chrono::milliseconds openTimeout{std::chrono::milliseconds{0}};
};

// TODO: For 1.0, expose stream preparation through the provider interface.
class PICAUDIO_ENGINE_EXPORT RemoteSourceProvider
{
public:
    virtual ~RemoteSourceProvider() = default;

    //! Create an unopened source. Access capabilities may remain unknown until response headers arrive.
    [[nodiscard]] virtual RemoteStreamSource createStreamSource(const QUrl& url) const = 0;
};
} // namespace PicAudioEngine
