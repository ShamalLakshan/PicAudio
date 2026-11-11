/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <core/coresettings.h>
#include <core/engine/audiooutput.h>
#include <utils/lockfreeringbuffer.h>

#include "pipewirecontext.h"
#include "pipewirecore.h"
#include "pipewireregistry.h"
#include "pipewirestream.h"
#include "pipewirethreadloop.h"

#include <cstddef>
#include <memory>

namespace PicAudioEngine::Pipewire {
class PipeWireOutput : public AudioOutput
{
public:
    PipeWireOutput();

    bool init(const AudioFormat& format) override;
    void uninit() override;
    void reset() override;
    void start() override;
    void drain() override;

    [[nodiscard]] bool initialised() const override;
    [[nodiscard]] QString device() const override;
    [[nodiscard]] OutputDevices getAllDevices(bool isCurrentOutput) override;

    OutputState currentState() override;
    [[nodiscard]] int bufferSize() const override;
    int write(std::span<const std::byte> data, int frameCount) override;
    void setPaused(bool pause) override;

    void setVolume(double volume) override;
    [[nodiscard]] bool supportsVolumeControl() const override;
    void setDevice(const QString& device) override;
    [[nodiscard]] AudioFormat negotiateFormat(const AudioFormat& requested) const override;

    [[nodiscard]] QString error() const override;
    [[nodiscard]] AudioFormat format() const override;

private:
    bool initCore();
    bool initStream();
    void uninitCore();
    static void process(void* userData);
    static void handleStateChanged(void* userdata, pw_stream_state old, pw_stream_state state, const char* error);
    static void drained(void* userdata);

    QString m_device;
    float m_volume;
    AudioFormat m_format;
    PicAudioSettings m_settings;

    std::unique_ptr<LockFreeRingBuffer<std::byte>> m_buffer;
    int m_targetBufferFrames;
    bool m_invalidTimingLogActive;

    bool m_loopStarted;
    std::unique_ptr<PipewireThreadLoop> m_loop;
    std::unique_ptr<PipewireContext> m_context;
    std::unique_ptr<PipewireCore> m_core;
    std::unique_ptr<PipewireStream> m_stream;
    std::unique_ptr<PipewireRegistry> m_registry;
};
} // namespace PicAudioEngine::Pipewire
