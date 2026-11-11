/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

// std::move_only_function is C++23; libc++ on some platforms (e.g. FreeBSD)
// does not implement it yet.  Provide a minimal virtual-dispatch polyfill.

#include <functional>

#ifdef __cpp_lib_move_only_function

namespace PicAudioEngine {
template <typename Sig>
using MoveOnlyFunction = std::move_only_function<Sig>;
} // namespace PicAudioEngine

#else

#include <cassert>
#include <memory>
#include <type_traits>
#include <utility>

namespace PicAudioEngine {
template <typename Signature>
class MoveOnlyFunction;

template <typename R, typename... Args>
class MoveOnlyFunction<R(Args...)>
{
private:
    struct CallableBase
    {
        virtual ~CallableBase()        = default;
        virtual R invoke(Args... args) = 0;
    };

    template <typename F>
    struct CallableImpl final : CallableBase
    {
        mutable F func;

        template <typename G>
        explicit CallableImpl(G&& g)
            : func(std::forward<G>(g))
        { }

        R invoke(Args... args) override
        {
            if constexpr(std::is_void_v<R>) {
                std::invoke(func, std::forward<Args>(args)...);
            }
            else {
                return std::invoke(func, std::forward<Args>(args)...);
            }
        }
    };

    std::unique_ptr<CallableBase> m_callable;

public:
    MoveOnlyFunction() noexcept = default;

    MoveOnlyFunction(std::nullptr_t) noexcept
        : MoveOnlyFunction()
    { }

    MoveOnlyFunction(MoveOnlyFunction&&) noexcept            = default;
    MoveOnlyFunction& operator=(MoveOnlyFunction&&) noexcept = default;

    MoveOnlyFunction(const MoveOnlyFunction&)            = delete;
    MoveOnlyFunction& operator=(const MoveOnlyFunction&) = delete;

    template <typename F>
        requires(!std::is_same_v<std::decay_t<F>, MoveOnlyFunction>
                 && std::is_invocable_r_v<R, std::decay_t<F>&, Args...>)
    MoveOnlyFunction(F&& f)
        : m_callable(std::make_unique<CallableImpl<std::decay_t<F>>>(std::forward<F>(f)))
    { }

    MoveOnlyFunction& operator=(std::nullptr_t) noexcept
    {
        m_callable.reset();
        return *this;
    }

    template <typename F>
        requires(!std::is_same_v<std::decay_t<F>, MoveOnlyFunction>
                 && std::is_invocable_r_v<R, std::decay_t<F>&, Args...>)
    MoveOnlyFunction& operator=(F&& f)
    {
        MoveOnlyFunction tmp(std::forward<F>(f));
        m_callable = std::move(tmp.m_callable);
        return *this;
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return static_cast<bool>(m_callable);
    }

    void swap(MoveOnlyFunction& other) noexcept
    {
        m_callable.swap(other.m_callable);
    }

    R operator()(Args... args)
    {
        if(!m_callable) {
            assert(false && "MoveOnlyFunction: called while empty");
            throw std::bad_function_call{};
        }

        return m_callable->invoke(std::forward<Args>(args)...);
    }
};

template <typename Signature>
void swap(MoveOnlyFunction<Signature>& lhs, MoveOnlyFunction<Signature>& rhs) noexcept
{
    lhs.swap(rhs);
}
} // namespace PicAudioEngine

#endif // __cpp_lib_move_only_function

// std::atomic<std::shared_ptr<T>> requires a specialisation that
// libc++ on some platforms has not yet shipped.  Fall back to the C++11
// std::atomic_load/store free functions in those cases.

#include <memory>

#ifdef __cpp_lib_atomic_shared_ptr

#include <atomic>

namespace PicAudioEngine {
template <typename T>
using AtomicSharedPtr = std::atomic<std::shared_ptr<T>>;
} // namespace PicAudioEngine

#else

namespace PicAudioEngine {

template <typename T>
class AtomicSharedPtr
{
public:
    AtomicSharedPtr() = default;
    explicit AtomicSharedPtr(std::shared_ptr<T> ptr)
        : m_ptr{std::move(ptr)}
    { }

    AtomicSharedPtr(const AtomicSharedPtr&)            = delete;
    AtomicSharedPtr& operator=(const AtomicSharedPtr&) = delete;
    AtomicSharedPtr(AtomicSharedPtr&&)                 = delete;
    AtomicSharedPtr& operator=(AtomicSharedPtr&&)      = delete;

    [[nodiscard]] std::shared_ptr<T> load(std::memory_order order = std::memory_order_seq_cst) const
    {
        return std::atomic_load_explicit(&m_ptr, order);
    }

    void store(std::shared_ptr<T> desired, std::memory_order order = std::memory_order_seq_cst)
    {
        std::atomic_store_explicit(&m_ptr, std::move(desired), order);
    }

private:
    std::shared_ptr<T> m_ptr;
};
} // namespace PicAudioEngine

#endif // __cpp_lib_atomic_shared_ptr
