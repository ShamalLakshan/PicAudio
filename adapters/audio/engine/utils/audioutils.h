/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fyutils_export.h"

#include <cstdint>

namespace PicAudioEngine::Audio {
enum class VolumeCurve : uint8_t
{
    LinearDb = 0,
    LinearAmplitude,
    Quadratic,
    Cubic,
    LogarithmicDb,
};

PICAUDIO_UTILS_EXPORT double dbToVolume(double db);
PICAUDIO_UTILS_EXPORT double volumeToDb(double volume);
// Map between linear amplitude and a normalised control position in [0, 1].
// LinearDb spans -40 dB to 0 dB, with position 0 reserved for mute.
PICAUDIO_UTILS_EXPORT double volumeToPosition(double volume, VolumeCurve curve);
PICAUDIO_UTILS_EXPORT double volumeFromPosition(double position, VolumeCurve curve);
} // namespace PicAudioEngine::Audio
