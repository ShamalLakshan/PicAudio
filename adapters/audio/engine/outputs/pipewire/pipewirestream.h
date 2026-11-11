/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "pipewirecore.h"

#include <pipewire/stream.h>

#include <memory>
#include <optional>
#include <vector>

#include <QString>

namespace PicAudioEngine {
class AudioFormat;

namespace Pipewire {
class PipewireStream
{
public:
    PipewireStream(PipewireCore* core, const AudioFormat& format, int latencyFrames = 0, const QString& device = {});
    ~PipewireStream();

    struct TimeInfo
    {
        int64_t now{0};
        spa_fraction rate{0, 1};
        uint64_t ticks{0};
        int64_t delay{0};
        uint64_t queued{0};
        uint64_t buffered{0};
        uint32_t queuedBuffers{0};
        uint32_t availBuffers{0};
        uint64_t size{0};
    };

    pw_stream_state state();
    [[nodiscard]] std::optional<TimeInfo> time() const;

    void setActive(bool active);
    void setVolume(float volume);

    pw_buffer* dequeueBuffer();
    void queueBuffer(pw_buffer* buffer);
    void flush(bool drain);

    bool connect(uint32_t id, const pw_direction& direction, std::vector<const spa_pod*>& params,
                 const pw_stream_flags& flags);
    void addListener(const pw_stream_events& streamEvents, void* data);

private:
    struct PwStreamDeleter
    {
        void operator()(pw_stream* stream) const
        {
            if(stream) {
                pw_stream_disconnect(stream);
                pw_stream_destroy(stream);
            }
        }
    };
    using PwStreamUPtr = std::unique_ptr<pw_stream, PwStreamDeleter>;

    spa_hook m_streamListener;
    PwStreamUPtr m_stream;
    uint32_t m_channelCount;
};
} // namespace Pipewire
} // namespace PicAudioEngine
