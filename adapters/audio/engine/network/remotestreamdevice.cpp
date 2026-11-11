/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include <core/network/remotestreamdevice.h>

namespace PicAudioEngine {
void RemoteStreamDevice::setReconnectOnFinishedEnabled(bool /*enabled*/) { }

bool RemoteStreamDevice::shouldExposePathToDecoder() const
{
    return true;
}

std::optional<NetworkStreamMetadata> RemoteStreamDevice::remoteStreamMetadata() const
{
    return {};
}
} // namespace PicAudioEngine
