/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <pipewire/thread-loop.h>

#include <memory>

namespace PicAudioEngine::Pipewire {
class PipewireThreadLoop;

class ThreadLoopGuard
{
public:
    explicit ThreadLoopGuard(PipewireThreadLoop* loop);
    ~ThreadLoopGuard();

private:
    PipewireThreadLoop* m_loop;
};

class PipewireThreadLoop
{
public:
    PipewireThreadLoop();

    [[nodiscard]] pw_loop* loop() const;

    bool start();
    void stop();

    void signal(bool accept);
    int wait(int secs);

private:
    friend class ThreadLoopGuard;

    void lock() const;
    void unlock() const;

    struct PwThreadLoopDeleter
    {
        void operator()(pw_thread_loop* loop)
        {
            if(loop) {
                pw_thread_loop_destroy(loop);
            }
        }
    };
    using PwThreadLoopUPtr = std::unique_ptr<pw_thread_loop, PwThreadLoopDeleter>;

    PwThreadLoopUPtr m_loop;
};
} // namespace PicAudioEngine::Pipewire
