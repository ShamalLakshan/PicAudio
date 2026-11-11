/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <utils/compatutils.h>

namespace PicAudioEngine {
template <typename T>
class Snapshot
{
public:
    struct Data
    {
        uint64_t epoch{0};
        T value{};
    };

    Snapshot()
        : m_data{std::make_shared<const Data>()}
    { }

    [[nodiscard]] std::shared_ptr<const Data> load() const noexcept
    {
        return m_data.load(std::memory_order_acquire);
    }

    template <typename Fn>
    void update(Fn&& updater)
    {
        auto current = load();
        auto next    = std::make_shared<Data>(*current);

        ++next->epoch;

        std::invoke(std::forward<decltype(updater)>(updater), next->value);
        m_data.store(std::shared_ptr<const Data>{std::move(next)}, std::memory_order_release);
    }

private:
    AtomicSharedPtr<const Data> m_data;
};
} // namespace PicAudioEngine
