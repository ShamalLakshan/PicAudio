/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <core/engine/audioencoder.h>

#include <optional>
#include <vector>

namespace PicAudioEngine {
class AudioEncoderRegistryPrivate;

class PICAUDIO_ENGINE_EXPORT AudioEncoderRegistry
{
public:
    enum class BackendPriority : int8_t
    {
        Fallback = -100,
        Normal   = 0,
    };

    AudioEncoderRegistry();
    ~AudioEncoderRegistry();

    void addEncoderBackend(const QString& id, const QString& name, EncoderCreator creator,
                           BackendPriority priority = BackendPriority::Normal);

    [[nodiscard]] std::vector<AudioEncoderInfo> availableEncoders() const;
    [[nodiscard]] std::vector<AudioEncoderInfo> preferredEncoders() const;
    [[nodiscard]] std::optional<AudioEncoderInfo> encoderInfo(const QString& encoderId) const;
    [[nodiscard]] std::unique_ptr<AudioEncoder> createEncoder(const QString& encoderId) const;

    void reset();

private:
    std::unique_ptr<AudioEncoderRegistryPrivate> p;
};
} // namespace PicAudioEngine
