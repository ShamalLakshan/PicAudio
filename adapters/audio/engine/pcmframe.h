/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <core/engine/audioformat.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <span>

namespace PicAudioEngine {
/*!
 * Fixed-hop interleaved PCM snapshot for visualisers and other analysis consumers.
 *
 * Samples are linear float PCM in interleaved channel order.
 */
struct PcmFrame
{
    static constexpr int MaxChannels = 8;
    static constexpr int MaxFrames   = 1024;
    static constexpr int MaxSamples  = MaxChannels * MaxFrames;

    std::array<float, MaxSamples> samples{};
    AudioFormat format{SampleFormat::F32, 0, 0};
    int frameCount{0};
    uint64_t streamTimeMs{0};
    uint32_t streamId{0};
    std::chrono::steady_clock::time_point presentationTime;
    bool discontinuityBefore{false};

    [[nodiscard]] size_t sampleCount() const
    {
        if(frameCount <= 0 || format.channelCount() <= 0) {
            return 0;
        }

        return static_cast<size_t>(frameCount) * static_cast<size_t>(format.channelCount());
    }

    [[nodiscard]] std::span<const float> interleavedSamples() const
    {
        return {samples.data(), sampleCount()};
    }
};
} // namespace PicAudioEngine
