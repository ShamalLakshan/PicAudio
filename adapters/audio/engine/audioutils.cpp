/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include "audioutils.h"

#include <utils/timeconstants.h>

#include <algorithm>
#include <limits>

constexpr auto MaxStreamBufferSamples = 96U * 1000U * 1000U;

namespace PicAudioEngine::Audio {
AudioFormat normaliseProcessingFormat(AudioFormat format)
{
    if(format.isValid()) {
        format.setSampleFormat(SampleFormat::F64);
    }
    return format;
}

size_t bufferSamplesFromMs(int ms, int sampleRate, int channels)
{
    if(ms <= 0 || sampleRate <= 0 || channels <= 0) {
        return 0;
    }

    const auto checkedMul = [](int64_t a, int64_t b) {
        if(a <= 0 || b <= 0) {
            return int64_t{0};
        }
        if(a > (std::numeric_limits<int64_t>::max() / b)) {
            return std::numeric_limits<int64_t>::max();
        }
        return a * b;
    };

    constexpr auto msPerSecond = static_cast<int64_t>(Time::MsPerSecond);
    const int64_t msRate       = checkedMul(ms, sampleRate);
    const int64_t samples      = checkedMul(msRate, channels);
    const int64_t numerator    = samples >= (std::numeric_limits<int64_t>::max() - (msPerSecond - 1))
                                   ? std::numeric_limits<int64_t>::max()
                                   : (samples + (msPerSecond - 1));

    const int64_t roundedSamples = numerator / msPerSecond;
    const auto maxSamples        = static_cast<int64_t>(MaxStreamBufferSamples);

    return static_cast<size_t>(std::clamp<int64_t>(roundedSamples, 0, maxSamples));
}

bool inputFormatsMatch(const AudioFormat& lhs, const AudioFormat& rhs)
{
    if(lhs.sampleRate() != rhs.sampleRate() || lhs.channelCount() != rhs.channelCount()) {
        return false;
    }

    if(lhs.hasChannelLayout() != rhs.hasChannelLayout()) {
        return false;
    }
    if(!lhs.hasChannelLayout()) {
        return true;
    }

    for(int i = 0; i < lhs.channelCount(); ++i) {
        if(lhs.channelPosition(i) != rhs.channelPosition(i)) {
            return false;
        }
    }
    return true;
}

bool outputFormatsMatch(const AudioFormat& lhs, const AudioFormat& rhs)
{
    return inputFormatsMatch(lhs, rhs) && lhs.sampleFormat() == rhs.sampleFormat();
}

bool shouldAutomaticallyDither(SampleFormat input, SampleFormat output)
{
    const bool integerOutput = output == SampleFormat::U8 || output == SampleFormat::S16
                            || output == SampleFormat::S24In32 || output == SampleFormat::S32;
    if(!integerOutput) {
        return false;
    }

    if(input == SampleFormat::F32 || input == SampleFormat::F64) {
        return true;
    }

    const AudioFormat inputFormat{input, 1, 1};
    const AudioFormat outputFormat{output, 1, 1};
    return inputFormat.isValid() && outputFormat.isValid()
        && outputFormat.bitsPerSample() < inputFormat.bitsPerSample();
}
} // namespace PicAudioEngine::Audio
