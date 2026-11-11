/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include <cassert>
#include <type_traits>
#include <utility>

namespace PicAudioEngine {
template <typename Function>
class ScopeGuard
{
public:
    [[nodiscard]] explicit ScopeGuard(Function&& function) noexcept(std::is_nothrow_move_constructible_v<Function>)
        : m_function(std::move(function))
    { }

    [[nodiscard]] explicit ScopeGuard(const Function& function) noexcept(std::is_nothrow_copy_constructible_v<Function>)
        : m_function(function)
    { }

    [[nodiscard]] ScopeGuard(ScopeGuard&& other) noexcept(std::is_nothrow_move_constructible_v<Function>)
        : m_function(std::move(other.m_function))
        , m_invoke(std::exchange(other.m_invoke, false))
    { }

    ScopeGuard(const ScopeGuard&)            = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard& operator=(ScopeGuard&&)      = delete;

    ~ScopeGuard() noexcept
    {
        if(m_invoke) {
            std::invoke(m_function);
        }
    }

    void dismiss() noexcept
    {
        m_invoke = false;
    }

    void commit() noexcept(std::is_nothrow_invocable_v<Function&>)
    {
        assert(m_invoke);
        m_invoke = false;
        std::invoke(m_function);
    }

private:
    Function m_function;
    bool m_invoke{true};
};

template <typename Result>
ScopeGuard(Result (&)()) -> ScopeGuard<Result (*)()>;

template <typename Function>
[[nodiscard]] auto scopeGuard(Function&& function)
{
    return ScopeGuard<std::decay_t<Function>>{std::forward<Function>(function)};
}
} // namespace PicAudioEngine
