/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once
#ifndef PICAUDIO_ENGINE_DSPPLUGIN_H
#define PICAUDIO_ENGINE_DSPPLUGIN_H

#include "dspnode.h"

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
#endif // PICAUDIO_ENGINE_DSPPLUGIN_H
