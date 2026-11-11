/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <core/engine/audioinput.h>

#include <QtPlugin>

namespace PicAudioEngine {
struct InputCreator
{
    DecoderCreator decoder;
    ReaderCreator reader;
    ArchiveReaderCreator archiveReader;
    //! Default priority for every backend supplied by this plugin.
    int priority{DefaultInputPriority};
};

/*!
 * Plugin interface for registering decoder/reader/archive backends.
 */
class InputPlugin
{
public:
    virtual ~InputPlugin() = default;

    //! Unique plugin/backend name shown in settings/UI.
    [[nodiscard]] virtual QString inputName() const = 0;
    //! Factory bundle for decoder/reader/archive reader implementations.
    [[nodiscard]] virtual InputCreator inputCreator() const = 0;
};
} // namespace PicAudioEngine

Q_DECLARE_INTERFACE(PicAudioEngine::InputPlugin, "com.picaudio.engine.plugin.input")
