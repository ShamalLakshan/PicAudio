/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once
#ifndef PICAUDIO_ENGINE_DOWNMIXTOSTEREODSP_H
#define PICAUDIO_ENGINE_DOWNMIXTOSTEREODSP_H

#include "dspnode.h"

namespace PicAudioEngine {
class DownmixToStereoDsp : public DspNode
{
public:
    QString name() const override;
    QString id() const override;

    AudioFormat outputFormat(const AudioFormat& input) const override;
    void prepare(const AudioFormat& format) override;
    void process(ProcessingBufferList& chunks) override;

private:
    AudioFormat m_format;
};
} // namespace PicAudioEngine
#endif // PICAUDIO_ENGINE_DOWNMIXTOSTEREODSP_H
