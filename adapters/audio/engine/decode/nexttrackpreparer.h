/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include "core/engine/pipeline/audiostream.h"

#include <core/engine/audioformat.h>
#include <core/engine/audioinput.h>
#include <core/engine/audioloader.h>
#include <core/engine/enginedefs.h>
#include <core/track.h>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>

namespace PicAudioEngine {
class AudioLoader;

struct PICAUDIO_ENGINE_EXPORT NextTrackPreparationState
{
    Engine::PlaybackItem item;
    LoadedDecoder loadedDecoder;
    AudioFormat format;
    AudioStreamPtr preparedStream;
    uint64_t preparedDecodePositionMs{0};
    bool allowsConcurrentDecoding{true};

    [[nodiscard]] bool isValid() const
    {
        return item.isValid() && loadedDecoder.decoder != nullptr && format.isValid();
    }
};

/*!
 * Helper that prepares next-track decode state.
 *
 * Called by `NextTrackPrepareWorker` to pre-open decoder resources and
 * optionally pre-create/prefill a stream for transition handoff.
 */
class PICAUDIO_ENGINE_EXPORT NextTrackPreparer
{
public:
    struct Context
    {
        std::shared_ptr<AudioLoader> audioLoader;
        Track currentTrack; // Used to check for same-file transition
        Engine::PlaybackState playbackState{Engine::PlaybackState::Stopped};
        AudioDecoder::PlaybackHints playbackHints{AudioDecoder::NoHints};
        bool currentAllowsConcurrentDecoding{true};
        uint64_t bufferLengthMs{0};     // Internal prepared-stream reserve target
        uint64_t preferredPrefillMs{0}; // Minimum decoded reserve to accumulate before handoff
        std::shared_ptr<std::atomic<bool>> cancelFlag;
        std::function<void(AudioDecoder*)> activeDecoderChanged;
    };

    /*!
     * Prepare decoder and optional prefilled stream for @p track.
     *
     * Ownership of returned resources is
     * transferred to caller via NextTrackPreparationState.
     */
    [[nodiscard]] static NextTrackPreparationState prepare(const Track& track, const Context& context);
};

/*!
 * Single-worker background queue for next-track preparation jobs.
 *
 * Queue semantics are replace-latest: at most one pending request is retained.
 * Completion callback is invoked from the worker thread.
 */
class PICAUDIO_ENGINE_EXPORT NextTrackPrepareWorker
{
public:
    enum class Purpose : uint8_t
    {
        NextTrack,
        ManualRemoteCrossfade,
    };

    struct Request
    {
        uint64_t jobToken{0};
        uint64_t requestId{0};
        Purpose purpose{Purpose::NextTrack};
        Engine::PlaybackItem item;
        NextTrackPreparer::Context context;
    };

    using CompletionHandler = std::function<void(uint64_t jobToken, uint64_t requestId, Purpose purpose,
                                                 const Engine::PlaybackItem& item, NextTrackPreparationState prepared)>;

    NextTrackPrepareWorker();
    ~NextTrackPrepareWorker();

    NextTrackPrepareWorker(const NextTrackPrepareWorker&)            = delete;
    NextTrackPrepareWorker& operator=(const NextTrackPrepareWorker&) = delete;

    //! Start worker thread and set completion callback.
    void start(CompletionHandler handler);
    //! Stop worker and clear pending request.
    void stop();
    //! Cancel active/pending preparation without stopping worker thread.
    void cancelPendingJobs();
    //! Single-slot queue semantics: replaces any previously pending request.
    void replacePending(Request request);

    [[nodiscard]] uint64_t activeJobToken() const;
    //! Request cancellation of the decoder owned by the active preparation job, if any.
    void requestActiveJobAbort() const;

private:
    void run(const std::stop_token& stopToken);
    void setActiveDecoder(AudioDecoder* decoder);

    mutable std::mutex m_mutex;
    mutable std::mutex m_activeDecoderMutex;
    std::condition_variable_any m_cv;
    std::optional<Request> m_pendingRequest;
    CompletionHandler m_completion;
    std::jthread m_worker;
    uint64_t m_nextJobToken;
    std::atomic<uint64_t> m_activeJobToken;
    std::shared_ptr<std::atomic<bool>> m_cancelFlag;
    AudioDecoder* m_activeDecoder;
};
} // namespace PicAudioEngine
