/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <core/engine/audiobuffer.h>

namespace PicAudioEngine {
class AudioFormat;

namespace Audio {
/*!
 * Convert an AudioBuffer to @p outputFormat.
 *
 * Handles sample-format conversion and channel-layout remap/mix as needed.
 * When @p dither is true, TPDF dither is applied for float->S16 conversion.
 * Returns an invalid buffer on conversion failure.
 */
PICAUDIO_ENGINE_EXPORT AudioBuffer convert(const AudioBuffer& buffer, const AudioFormat& outputFormat, bool dither = false);

/*!
 * Convert interleaved raw samples between formats.
 *
 * When @p dither is true, TPDF dither is applied for float->S16 conversion.
 * @p output must have enough space for the converted sample count.
 */
PICAUDIO_ENGINE_EXPORT bool convert(const AudioFormat& inputFormat, const std::byte* input, const AudioFormat& outputFormat,
                           std::byte* output, int frameCount, bool dither = false);

} // namespace Audio
} // namespace PicAudioEngine
