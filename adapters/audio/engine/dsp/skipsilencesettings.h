/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <cstdint>

namespace PicAudioEngine::SkipSilenceSettings {
inline constexpr int MinSilenceMinMs                = 200;
inline constexpr int MinSilenceMaxMs                = 20000;
inline constexpr int ThresholdMinDb                 = -100;
inline constexpr int ThresholdMaxDb                 = -20;
inline constexpr int DefaultMinSilenceDurationMs    = 5000;
inline constexpr int DefaultThresholdDb             = -60;
inline constexpr bool DefaultKeepInitialPeriod      = true;
inline constexpr bool DefaultIncludeMiddleSilence   = false;
inline constexpr std::uint32_t SerializationVersion = 1;
} // namespace PicAudioEngine::SkipSilenceSettings
