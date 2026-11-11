/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include "outputpump.h"

#include <utils/timeconstants.h>

#include <algorithm>
#include <cmath>

namespace PicAudioEngine {
OutputPump::OutputPump()
    : OutputPump{{}}
{ }

OutputPump::OutputPump(const Config& config)
    : m_config{config}
{ }

bool OutputPump::shouldHoldForPreroll(int queuedPendingFrames, int minBufferedFramesToDrain)
{
    return queuedPendingFrames > 0 && queuedPendingFrames < std::max(0, minBufferedFramesToDrain);
}

bool OutputPump::shouldWaitAfterDrain(int freeFrames, int writtenFrames)
{
    return freeFrames <= 0 || writtenFrames <= 0;
}

std::chrono::milliseconds OutputPump::writeBackoff(const AudioFormat& outputFormat,
                                                   const OutputState& outputState) const
{
    const int minWaitMs = std::max(1, m_config.minWaitMs);
    const int maxWaitMs = std::max(minWaitMs, m_config.maxWaitMs);
    const int divisor   = std::max(1, m_config.queuedDrainDivisor);

    if(!outputFormat.isValid() || outputFormat.sampleRate() <= 0) {
        return std::chrono::milliseconds{minWaitMs};
    }

    const int queuedFrames = std::max(1, outputState.queuedFrames);
    const int drainFrames  = std::max(1, queuedFrames / divisor);
    const int waitMs
        = static_cast<int>(std::llround(static_cast<double>(drainFrames) * static_cast<double>(Time::MsPerSecond)
                                        / static_cast<double>(outputFormat.sampleRate())));

    return std::chrono::milliseconds{std::clamp(waitMs, minWaitMs, maxWaitMs)};
}
} // namespace PicAudioEngine
