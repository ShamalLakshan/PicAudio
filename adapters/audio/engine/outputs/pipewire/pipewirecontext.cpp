/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include "pipewirecontext.h"

#include "pipewireutils.h"

#include <QDebug>

namespace PicAudioEngine::Pipewire {
PipewireContext::PipewireContext(PipewireThreadLoop* loop)
    : m_loop{loop}
    , m_context{pw_context_new(m_loop->loop(), nullptr, 0)}
{
    if(!m_context) {
        qCWarning(PIPEWIRE) << "Failed to create context";
    }
}

pw_context* PipewireContext::context() const
{
    if(!m_context) {
        return nullptr;
    }

    return m_context.get();
}

PipewireThreadLoop* PipewireContext::threadLoop() const
{
    return m_loop;
}
} // namespace PicAudioEngine::Pipewire
