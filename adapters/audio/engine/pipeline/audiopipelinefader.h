/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <core/engine/enginedefs.h>

#include <cstdint>

namespace PicAudioEngine {
class AudioPipelineFader
{
public:
    virtual ~AudioPipelineFader() = default;

    virtual void setFaderCurve(Engine::FadeCurve curve)                           = 0;
    virtual void faderFadeIn(int durationMs, double targetGain, uint64_t token)   = 0;
    virtual void faderFadeOut(int durationMs, double currentGain, uint64_t token) = 0;
    virtual void faderStop()                                                      = 0;
    virtual void faderReset()                                                     = 0;
    [[nodiscard]] virtual bool faderIsFading() const                              = 0;
};
} // namespace PicAudioEngine
