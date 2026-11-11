/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include "decodercontext.h"

#include <core/engine/audioinput.h>
#include <core/engine/audioloader.h>
#include <core/engine/enginedefs.h>
#include <core/engine/pipeline/audiostream.h>
#include <core/track.h>

#include <optional>

class QObject;

namespace PicAudioEngine {
class AudioFormat;
class DecodingControllerPrivate;

/*!
 * Worker-thread decode controller.
 *
 * Decoder/source state and calls into AudioDecoder are owned by a dedicated
 * worker. AudioEngine interacts through commands and thread-safe snapshots;
 * decoded PCM is written directly to AudioStream's ring buffer.
 */
class PICAUDIO_ENGINE_EXPORT DecodingController
{
public:
    struct DecodeResult
    {
        bool stopDecodeTimer{false};
        bool inputNeedsMoreData{false};
    };

    explicit DecodingController(QObject* timerHost);
    ~DecodingController();

    DecodingController(const DecodingController&)            = delete;
    DecodingController& operator=(const DecodingController&) = delete;
    DecodingController(DecodingController&&)                 = delete;
    DecodingController& operator=(DecodingController&&)      = delete;

    //! Start decoding and arm periodic decode timer if needed.
    void startDecoding();
    //! Stop decode timer and stop decode state.
    void stopDecoding();
    //! Thread-safe cancellation of a blocking decoder operation.
    void requestAbort();
    //! Stop decode timer only (keep decoder context intact).
    void stopDecodeTimer();
    //! Ensure decode timer is active.
    void ensureDecodeTimerRunning();
    [[nodiscard]] bool isDecodeTimerActive() const;
    //! Configure decode hysteresis watermarks (milliseconds).
    void setBufferWatermarksMs(int lowWatermarkMs, int highWatermarkMs);
    //! Temporary reserve target for burst operations (for e.g. seek fade-out).
    void requestDecodeReserveMs(int reserveMs);
    //! Clear temporary reserve target.
    void clearDecodeReserve();
    [[nodiscard]] int lowWatermarkMs() const;
    [[nodiscard]] int highWatermarkMs() const;

    [[nodiscard]] bool isValid() const;
    [[nodiscard]] bool isDecoding() const;
    [[nodiscard]] bool isSeekable() const;
    [[nodiscard]] bool allowsConcurrentDecoding() const;
    [[nodiscard]] int playbackPrebufferMs() const;
    [[nodiscard]] AudioDecoder::RepeatHandling repeatHandling() const;

    [[nodiscard]] AudioStreamPtr activeStream() const;
    [[nodiscard]] StreamId activeStreamId() const;
    [[nodiscard]] Track track() const;
    [[nodiscard]] AudioFormat format() const;
    [[nodiscard]] uint64_t currentPosition() const;
    [[nodiscard]] uint64_t startPosition() const;
    [[nodiscard]] bool lastDecodeNeededMoreInput() const;

    [[nodiscard]] AudioDecoder::PlaybackHints playbackHints() const;
    void setPlaybackHints(AudioDecoder::PlaybackHints hints);

    bool init(LoadedDecoder decoder, const Track& track);
    bool adoptPreparedDecoder(LoadedDecoder decoder, const Track& track);
    void setPreparedDecodePosition(uint64_t positionMs);
    [[nodiscard]] AudioStreamPtr createStream(size_t bufferSamples,
                                              Engine::FadeCurve fadeCurve = Engine::FadeCurve::Linear);
    void setActiveStream(AudioStreamPtr stream);
    [[nodiscard]] AudioStreamPtr detachStream();
    void start();
    void stop();
    bool seek(uint64_t positionMs);

    [[nodiscard]] bool switchContiguousTrack(const Track& track);
    void setEndPolicy(DecoderContext::EndPolicy policy, std::optional<uint64_t> windowEndMs = {});
    [[nodiscard]] DecoderContext::EndPolicy endPolicy() const;
    void syncStreamPosition();
    [[nodiscard]] int prefillActiveStream(size_t targetSamples, int maxChunks = 0, size_t maxFramesPerChunk = 4096);
    [[nodiscard]] int prefillActiveStreamMs(uint64_t targetMs, int maxChunks = 0, size_t maxFramesPerChunk = 4096);
    [[nodiscard]] bool refreshTrackMetadata();
    [[nodiscard]] int bitrate() const;
    [[nodiscard]] LoadedDecoder takeLoadedDecoder();
    void reset();

    //! Prepare a stream for an incoming crossfade track using current decoder context.
    //! Assumes decoder is already initialised for the new track.
    [[nodiscard]] AudioStreamPtr setupCrossfadeStream(int bufferLengthMs, Engine::FadeCurve curve);
    //! Prepare a stream for seek crossfade at the given absolute position.
    [[nodiscard]] AudioStreamPtr prepareSeekStream(uint64_t seekPosMs, int bufferLengthMs, Engine::FadeCurve curve,
                                                   const Track& track);

    //! Perform one decode iteration and return requested follow-up actions.
    [[nodiscard]] DecodeResult decodeLoop();
    //! Handle Qt timer event id; returns nullopt for unrelated timer ids.
    [[nodiscard]] std::optional<DecodeResult> handleTimer(int timerId, bool seekInProgress);
    [[nodiscard]] bool inputNeedsMoreData() const;

private:
    std::unique_ptr<DecodingControllerPrivate> p;
};
} // namespace PicAudioEngine
