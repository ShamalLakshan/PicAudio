/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <core/engine/audioformat.h>

#include <chrono>

namespace PicAudioEngine::Audio {
constexpr auto MaxPresentationDelay = std::chrono::seconds{60};

AudioFormat normaliseProcessingFormat(AudioFormat format);
size_t bufferSamplesFromMs(int ms, int sampleRate, int channels);
bool inputFormatsMatch(const AudioFormat& lhs, const AudioFormat& rhs);
bool outputFormatsMatch(const AudioFormat& lhs, const AudioFormat& rhs);
bool shouldAutomaticallyDither(SampleFormat input, SampleFormat output);
} // namespace PicAudioEngine::Audio
