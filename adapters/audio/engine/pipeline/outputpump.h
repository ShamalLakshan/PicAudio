/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include "buffereddspstage.h"

#include <core/engine/audioformat.h>
#include <core/engine/audiooutput.h>

#include <chrono>
#include <utility>

namespace PicAudioEngine {
/*!
 * Output-side helper for pending-buffer drain and backoff decisions.
 */
class PICAUDIO_ENGINE_EXPORT OutputPump
{
public:
    struct Config
    {
        int minWaitMs{3};
        int maxWaitMs{40};
        int queuedDrainDivisor{2};
    };

    struct PendingDrainResult
    {
        int writtenFrames{0};
        int queueOffsetStartFrames{0};
        int queueOffsetEndFrames{0};
    };

    OutputPump();
    explicit OutputPump(const Config& config);

    [[nodiscard]] static bool shouldHoldForPreroll(int queuedPendingFrames, int minBufferedFramesToDrain);
    [[nodiscard]] static bool shouldWaitAfterDrain(int freeFrames, int writtenFrames);

    [[nodiscard]] std::chrono::milliseconds writeBackoff(const AudioFormat& outputFormat,
                                                         const OutputState& outputState) const;

    template <typename Fn>
    [[nodiscard]] PendingDrainResult drainPending(BufferedDspStage& stage, Fn&& writer) const
    {
        const auto writeResult = stage.writePendingOutput(std::forward<Fn>(writer));
        return {
            .writtenFrames          = writeResult.writtenFrames,
            .queueOffsetStartFrames = writeResult.queueOffsetStartFrames,
            .queueOffsetEndFrames   = writeResult.queueOffsetEndFrames,
        };
    }

private:
    Config m_config;
};
} // namespace PicAudioEngine
