/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

namespace PicAudioEngine::ResamplerSettings {
constexpr auto SerialisationVersion = 1;
constexpr auto DefaultTargetRate    = 48000;
constexpr auto DefaultUseSoxr       = true;
constexpr auto DefaultSoxrPrecision = 20.0;

enum class SampleRateFilterMode : uint8_t
{
    ExcludeListed = 0,
    OnlyListed
};

constexpr auto DefaultSampleRateFilterMode = SampleRateFilterMode::ExcludeListed;
constexpr auto DefaultFilteredRatesText    = "";
} // namespace PicAudioEngine::ResamplerSettings
