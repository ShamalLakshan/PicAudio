/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <atomic>
#include <cstdint>

class QObject;

namespace PicAudioEngine {
/*!
 * Lock-free signal mailbox from audio thread to engine thread.
 *
 * The audio thread marks bit flags and posts at most one wake event until the
 * engine thread drains the pending mask.
 */
class PICAUDIO_ENGINE_EXPORT PipelineSignalMailbox
{
public:
    PipelineSignalMailbox();

    //! Configure wake-event target object/type for engine-thread notifications.
    void setWakeTarget(QObject* target, int wakeEventType);
    //! Atomically fetch-and-clear pending signal bits.
    [[nodiscard]] uint32_t drain();
    //! Clear specific bits from pending mask.
    void clear(uint32_t signalMask);
    //! Set specific bits and schedule wake event if needed.
    void mark(uint32_t signalMask);
    //! Clear pending state and posted-wake latch.
    void reset();
    //! Default wake event type used when caller does not provide one.
    [[nodiscard]] static int signalWakeEventType();

private:
    void postWakeIfNeeded();

    std::atomic<uint32_t> m_pendingSignalBits;
    std::atomic<bool> m_signalWakePosted;
    std::atomic<QObject*> m_signalWakeTarget;
    std::atomic<int> m_signalWakeEventType;
};
} // namespace PicAudioEngine
