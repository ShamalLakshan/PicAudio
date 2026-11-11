/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <memory>
#include <span>

namespace PicAudioEngine::Dsp {
class PICAUDIO_ENGINE_EXPORT RealFft
{
public:
    RealFft();
    explicit RealFft(int fftSize);
    ~RealFft();

    RealFft(RealFft&& other) noexcept;
    RealFft& operator=(RealFft&& other) noexcept;
    RealFft(const RealFft&)            = delete;
    RealFft& operator=(const RealFft&) = delete;

    [[nodiscard]] bool reset(int fftSize);
    void clear();

    [[nodiscard]] bool isValid() const;
    [[nodiscard]] int fftSize() const;
    [[nodiscard]] int binCount() const;
    //! Return the supported real-transform size closest to fftSize, or 0 for an invalid request.
    [[nodiscard]] static int nearestValidSize(int fftSize);

    // Computes magnitude bins for a real-valued input signal. Output must provide fftSize()/2 + 1 bins.
    [[nodiscard]] bool transformMagnitudes(std::span<const float> input, std::span<float> output) const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
} // namespace PicAudioEngine::Dsp
