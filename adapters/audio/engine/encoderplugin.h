/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <core/engine/audioencoder.h>

#include <QtPlugin>

namespace PicAudioEngine {
class EncoderPlugin
{
public:
    virtual ~EncoderPlugin() = default;

    [[nodiscard]] virtual QString encoderName() const           = 0;
    [[nodiscard]] virtual EncoderCreator encoderCreator() const = 0;
};
} // namespace PicAudioEngine

Q_DECLARE_INTERFACE(PicAudioEngine::EncoderPlugin, "com.picaudio.engine.plugin.encoder")
