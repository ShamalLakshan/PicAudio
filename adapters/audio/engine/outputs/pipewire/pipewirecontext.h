/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "pipewirethreadloop.h"

#include <pipewire/context.h>

namespace PicAudioEngine::Pipewire {
class PipewireContext
{
public:
    explicit PipewireContext(PipewireThreadLoop* loop);

    [[nodiscard]] pw_context* context() const;
    [[nodiscard]] PipewireThreadLoop* threadLoop() const;

private:
    struct PwContextDeleter
    {
        void operator()(pw_context* context)
        {
            if(context) {
                pw_context_destroy(context);
            }
        }
    };
    using PwContextUPtr = std::unique_ptr<pw_context, PwContextDeleter>;

    PipewireThreadLoop* m_loop;
    PwContextUPtr m_context;
};
} // namespace PicAudioEngine::Pipewire
