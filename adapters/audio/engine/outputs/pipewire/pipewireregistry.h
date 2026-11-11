/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <core/engine/audiooutput.h>

#include <pipewire/core.h>
#include <spa/utils/hook.h>

namespace PicAudioEngine::Pipewire {
class PipewireCore;

class PipewireRegistry
{
public:
    explicit PipewireRegistry(PipewireCore* core);
    ~PipewireRegistry();

    [[nodiscard]] OutputDevices devices() const;

private:
    static void onRegistryEvent(void* data, uint32_t id, uint32_t permissions, const char* type, uint32_t version,
                                const struct spa_dict* props);

    struct PwRegistryDeleter
    {
        void operator()(pw_registry* registry) const
        {
            if(registry) {
                pw_proxy_destroy(reinterpret_cast<pw_proxy*>(registry));
            }
        }
    };
    using PwRegistryUPtr = std::unique_ptr<pw_registry, PwRegistryDeleter>;

    PwRegistryUPtr m_registry;
    spa_hook m_registryListener;
    OutputDevices m_sinks;
};
} // namespace PicAudioEngine::Pipewire
