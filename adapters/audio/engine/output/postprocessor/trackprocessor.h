/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <core/engine/audioformat.h>

#include <cstddef>

namespace PicAudioEngine {
class Track;

/*!
 * Per-stream track-processing interface applied inside AudioMixer.
 *
 * TrackProcessors run before per-track DSP nodes for each stream. Instances are
 * stream-local and invoked on the audio thread.
 */
class PICAUDIO_ENGINE_EXPORT TrackProcessor
{
public:
    virtual ~TrackProcessor() = default;

    //! Initialize for a new track
    virtual void init(const Track& track, const AudioFormat& format) = 0;

    //! True if this processor needs to run for the current track.
    [[nodiscard]] virtual bool isActive() const = 0;

    //! Process interleaved F64 samples in-place.
    virtual void process(double* samples, size_t sampleCount) = 0;

    //! Called at end of track before switching to next track.
    virtual void endOfTrack() = 0;

    //! Reset internal state (e.g. after seek).
    virtual void reset() = 0;
};
} // namespace PicAudioEngine
