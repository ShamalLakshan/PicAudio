/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include "pipewirethreadloop.h"

#include "pipewireutils.h"

#include <QDebug>

namespace PicAudioEngine::Pipewire {
ThreadLoopGuard::ThreadLoopGuard(PipewireThreadLoop* loop)
    : m_loop{loop}
{
    m_loop->lock();
}

ThreadLoopGuard::~ThreadLoopGuard()
{
    m_loop->unlock();
}

int PipewireThreadLoop::wait(int secs)
{
    return pw_thread_loop_timed_wait(m_loop.get(), secs);
}

PipewireThreadLoop::PipewireThreadLoop()
    : m_loop{pw_thread_loop_new("PicAudio/pipewire", nullptr)}
{
    if(!m_loop) {
        qCWarning(PIPEWIRE) << "Could not create thread loop";
    }
}

pw_loop* PipewireThreadLoop::loop() const
{
    if(!m_loop) {
        return nullptr;
    }

    return pw_thread_loop_get_loop(m_loop.get());
}

bool PipewireThreadLoop::start()
{
    return pw_thread_loop_start(m_loop.get()) == 0;
}

void PipewireThreadLoop::stop()
{
    pw_thread_loop_stop(m_loop.get());
}

void PipewireThreadLoop::signal(bool accept)
{
    pw_thread_loop_signal(m_loop.get(), accept);
}

void PipewireThreadLoop::lock() const
{
    pw_thread_loop_lock(m_loop.get());
}

void PipewireThreadLoop::unlock() const
{
    pw_thread_loop_unlock(m_loop.get());
}
} // namespace PicAudioEngine::Pipewire
