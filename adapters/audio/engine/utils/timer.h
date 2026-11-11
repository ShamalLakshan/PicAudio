/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fyutils_export.h"

#include <QString>

#include <chrono>

namespace PicAudioEngine {
class PICAUDIO_UTILS_EXPORT Timer
{
public:
    Timer();

    void reset();

    [[nodiscard]] std::chrono::milliseconds elapsed() const;
    [[nodiscard]] std::chrono::nanoseconds elapsedNanoseconds() const;
    [[nodiscard]] QString elapsedFormatted() const;

private:
    std::chrono::steady_clock::time_point m_start;
};
} // namespace PicAudioEngine
