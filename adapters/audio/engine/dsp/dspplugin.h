/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <core/engine/dsp/dspnode.h>

#include <QtPlugin>

#include <vector>

namespace PicAudioEngine {
/*!
 * Plugin interface for registering DSP node factories.
 */
class DspPlugin
{
public:
    virtual ~DspPlugin() = default;

    //! Factory entries exposed by this plugin.
    [[nodiscard]] virtual std::vector<DspNode::Entry> dspCreators() const = 0;
};
} // namespace PicAudioEngine

Q_DECLARE_INTERFACE(PicAudioEngine::DspPlugin, "com.picaudio.engine.plugin.dsp")
