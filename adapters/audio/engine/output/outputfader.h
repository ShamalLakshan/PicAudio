/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <core/engine/dsp/processingbufferlist.h>
#include <core/engine/enginedefs.h>

#include <cstdint>
#include <optional>

namespace PicAudioEngine {
/*!
 * Final-stage output fade for transport transitions (play/pause/stop/manual change).
 * Owned and processed on the audio thread.
 */
class PICAUDIO_ENGINE_EXPORT OutputFader
{
public:
    OutputFader();

    enum class CompletionType : uint8_t
    {
        FadeInComplete,
        FadeOutComplete,
    };

    struct Completion
    {
        CompletionType type{CompletionType::FadeInComplete};
        uint64_t token{0};
    };

    void process(ProcessingBufferList& chunks);

    void reset();
    void fadeIn(int durationMs, double targetGain, int sampleRate, uint64_t token = 0);
    void fadeOut(int durationMs, double currentGain, int sampleRate, uint64_t token = 0);
    void stopFade();

    [[nodiscard]] bool isFading() const;
    [[nodiscard]] std::optional<Completion> takeCompletion();

    void setFadeCurve(Engine::FadeCurve curve);

private:
    void processBuffer(ProcessingBuffer& buffer);

    bool m_fading;
    bool m_fadingOut;
    uint64_t m_activeToken;
    std::optional<Completion> m_pendingCompletion;
    double m_currentGain;
    double m_fadeStartGain;
    double m_targetGain;
    int64_t m_fadeFramesTotal;
    int64_t m_fadeFramesDone;
    Engine::FadeCurve m_curve;
};
} // namespace PicAudioEngine
