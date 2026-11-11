/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#include "pipewirecore.h"

#include "pipewireutils.h"

#include <QDebug>

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

#ifdef __clang__
#pragma clang diagnostic ignored "-Wgnu-statement-expression-from-macro-expansion"
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

namespace PicAudioEngine::Pipewire {
PipewireCore::PipewireCore(PipewireContext* context)
    : m_context{context}
    , m_initialised{false}
    , m_core{pw_context_connect(m_context->context(), nullptr, 0)}
{
    if(!m_core) {
        qCWarning(PIPEWIRE) << "Failed to create core";
        return;
    }

    static constexpr pw_core_events coreEvents = {
        .version = PW_VERSION_CORE_EVENTS,
        .done    = onCoreDone,
    };

    pw_core_add_listener(m_core.get(), &m_coreListener, &coreEvents, this); // NOLINT
}

pw_core* PipewireCore::core() const
{
    return m_core.get();
}

bool PipewireCore::initialised() const
{
    return m_initialised;
}

int PipewireCore::sync() const
{
    return m_sync;
}

void PipewireCore::syncCore()
{
    m_sync = pw_core_sync(m_core.get(), PW_ID_CORE, m_sync); // NOLINT
}

void PipewireCore::onCoreDone(void* userdata, uint32_t id, int seq)
{
    auto* core = static_cast<PipewireCore*>(userdata);
    auto* loop = core->m_context->threadLoop();

    if(id != PW_ID_CORE || seq != core->sync()) {
        return;
    }

    spa_hook_remove(&core->m_coreListener);
    core->m_initialised = true;
    loop->signal(false);
}
} // namespace PicAudioEngine::Pipewire
