/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "pipewirecontext.h"

#include <pipewire/core.h>

#include <memory>

namespace PicAudioEngine::Pipewire {
class PipewireCore
{
public:
    explicit PipewireCore(PipewireContext* context);

    [[nodiscard]] pw_core* core() const;

    [[nodiscard]] bool initialised() const;
    [[nodiscard]] int sync() const;

    void syncCore();

private:
    static void onCoreDone(void* userdata, uint32_t id, int seq);

    struct PwCoreDeleter
    {
        void operator()(pw_core* core) const
        {
            if(core) {
                pw_core_disconnect(core);
            }
        }
    };
    using PwCoreUPtr = std::unique_ptr<pw_core, PwCoreDeleter>;

    PipewireContext* m_context;
    bool m_initialised;
    PwCoreUPtr m_core;
    spa_hook m_coreListener;
    int m_sync;
};
} // namespace PicAudioEngine::Pipewire
