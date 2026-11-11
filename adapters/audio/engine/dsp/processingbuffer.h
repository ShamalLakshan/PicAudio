/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <core/engine/audiobuffer.h>
#include <core/engine/audioformat.h>

#include <span>
#include <vector>

namespace PicAudioEngine {
/*!
 * Processing-stage audio buffer storing interleaved F64 samples.
 *
 * Used for intermediate engine processing before conversion back to
 * output format.
 */
class PICAUDIO_ENGINE_EXPORT ProcessingBuffer
{
public:
    ProcessingBuffer();
    ProcessingBuffer(const AudioFormat& format, uint64_t startTimeNs);
    ProcessingBuffer(std::span<const double> samples, const AudioFormat& format, uint64_t startTimeNs);

    [[nodiscard]] bool isValid() const;

    void reset();

    [[nodiscard]] AudioFormat format() const;
    [[nodiscard]] int frameCount() const;
    [[nodiscard]] int sampleCount() const;
    [[nodiscard]] uint64_t startTimeNs() const;
    [[nodiscard]] uint64_t sourceFrameDurationNs() const;
    [[nodiscard]] uint64_t endTimeNs() const;
    [[nodiscard]] uint64_t durationNs() const;

    void setStartTimeNs(uint64_t startTimeNs);
    void setSourceFrameDurationNs(uint64_t sourceFrameDurationNs);

    void reserveSamples(size_t samples);
    void resizeSamples(size_t samples);

    void clear();

    void append(std::span<const double> samples);

    [[nodiscard]] std::span<const double> constData() const;
    [[nodiscard]] std::span<double> data();
    [[nodiscard]] const std::vector<double>& samples() const;
    [[nodiscard]] std::vector<double>& samples();

    [[nodiscard]] AudioBuffer toAudioBuffer() const;

private:
    std::vector<double> m_samples;
    AudioFormat m_format;
    uint64_t m_startTimeNs;
    uint64_t m_sourceFrameDurationNs;
};
} // namespace PicAudioEngine
