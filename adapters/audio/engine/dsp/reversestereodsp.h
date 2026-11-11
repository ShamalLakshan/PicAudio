/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <core/engine/dsp/dspnode.h>

namespace PicAudioEngine {
class ReverseStereoDsp : public DspNode
{
public:
    [[nodiscard]] QString name() const override;
    [[nodiscard]] QString id() const override;

    void prepare(const AudioFormat& format) override;
    void process(ProcessingBufferList& chunks) override;

private:
    AudioFormat m_format;
};
} // namespace PicAudioEngine
