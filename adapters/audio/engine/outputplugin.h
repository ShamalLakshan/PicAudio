/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <core/engine/audiooutput.h>

#include <QtPlugin>

namespace PicAudioEngine {
/*!
 * Plugin interface for registering audio output backends.
 */
class OutputPlugin
{
public:
    virtual ~OutputPlugin() = default;

    //! Unique backend name shown in settings/UI.
    [[nodiscard]] virtual QString name() const = 0;
    //! Factory that creates backend instances.
    [[nodiscard]] virtual OutputCreator creator() const = 0;
};
} // namespace PicAudioEngine

Q_DECLARE_INTERFACE(PicAudioEngine::OutputPlugin, "com.picaudio.engine.plugin.output")
