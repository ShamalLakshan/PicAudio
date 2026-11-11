/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <array>
#include <chrono>
#include <cstdint>

namespace PicAudioEngine {
/*!
 * Fixed-size level analysis snapshot for consumers such as visualisers/meters.
 *
 * Values are linear (not dBFS).
 */
struct LevelFrame
{
    static constexpr int MaxChannels = 8;

    std::array<float, MaxChannels> peak{};
    std::array<float, MaxChannels> rms{};

    int channelCount{0};
    uint64_t streamTimeMs{0};
    std::chrono::steady_clock::time_point presentationTime;
};
} // namespace PicAudioEngine
