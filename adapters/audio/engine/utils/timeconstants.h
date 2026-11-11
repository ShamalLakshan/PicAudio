/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <cstdint>
#include <ratio>

namespace PicAudioEngine::Time {
constexpr uint64_t MsPerSecond = std::milli::den;
constexpr uint64_t UsPerSecond = std::micro::den;
constexpr uint64_t UsPerMs     = UsPerSecond / MsPerSecond;
constexpr uint64_t NsPerSecond = std::nano::den;
constexpr uint64_t NsPerMs     = NsPerSecond / MsPerSecond;
} // namespace PicAudioEngine::Time
