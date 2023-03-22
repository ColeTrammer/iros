#pragma once

#include <di/concepts/arithmetic.h>
#include <di/concepts/enum.h>
#include <di/concepts/pointer.h>
#include <di/meta/conditional.h>
#include <di/sync/memory_order.h>
#include <di/types/prelude.h>
#include <di/util/addressof.h>
#include <di/util/to_underlying.h>

namespace di::sync {
template<typename T>
requires((concepts::Enum<T> || concepts::Arithmetic<T> || concepts::Pointer<T>) && sizeof(T) <= sizeof(void*))
class Atomic {
private:
    using DeltaType = meta::Conditional<concepts::Pointer<T>, ptrdiff_t, T>;

    // NOTE: the builtin atomic operations treat pointer addition bytewise, so we
    //       must multiply by the sizeof(*T) if T is a pointer.
    constexpr DeltaType adjust_delta(DeltaType value) {
        if constexpr (concepts::Pointer<T>) {
            return value * sizeof(*m_value);
        } else {
            return value;
        }
    }

public:
    Atomic() = default;
    Atomic(Atomic const&) = delete;

    constexpr explicit Atomic(T value) : m_value(value) {}

    Atomic& operator=(Atomic const&) = delete;
    Atomic& operator=(Atomic const&) volatile = delete;

    void store(T value, MemoryOrder order = MemoryOrder::SequentialConsistency) {
        __atomic_store_n(util::addressof(m_value), value, util::to_underlying(order));
    }
    void store(T value, MemoryOrder order = MemoryOrder::SequentialConsistency) volatile {
        __atomic_store_n(util::addressof(m_value), value, util::to_underlying(order));
    }

    T load(MemoryOrder order = MemoryOrder::SequentialConsistency) const {
        return __atomic_load_n(util::addressof(m_value), util::to_underlying(order));
    }
    T load(MemoryOrder order = MemoryOrder::SequentialConsistency) const volatile {
        return __atomic_load_n(util::addressof(m_value), util::to_underlying(order));
    }

    T exchange(T value, MemoryOrder order = MemoryOrder::SequentialConsistency) {
        return __atomic_exchange_n(util::addressof(m_value), value, util::to_underlying(order));
    }
    T exchange(T value, MemoryOrder order = MemoryOrder::SequentialConsistency) volatile {
        return __atomic_exchange_n(util::addressof(m_value), value, util::to_underlying(order));
    }

    bool compare_exchange_weak(T& expected, T desired, MemoryOrder success, MemoryOrder failure) {
        return __atomic_compare_exchange_n(util::addressof(m_value), util::addressof(expected), desired, true,
                                           util::to_underlying(success), util::to_underlying(failure));
    }
    bool compare_exchange_weak(T& expected, T desired, MemoryOrder success, MemoryOrder failure) volatile {
        return __atomic_compare_exchange_n(util::addressof(m_value), util::addressof(expected), desired, true,
                                           util::to_underlying(success), util::to_underlying(failure));
    }

    bool compare_exchange_weak(T& expected, T desired, MemoryOrder order = MemoryOrder::SequentialConsistency) {
        if (order == MemoryOrder::AcquireRelease || order == MemoryOrder::Release) {
            return compare_exchange_weak(expected, desired, MemoryOrder::Release, MemoryOrder::Acquire);
        } else {
            return compare_exchange_weak(expected, desired, order, order);
        }
    }
    bool compare_exchange_weak(T& expected, T desired,
                               MemoryOrder order = MemoryOrder::SequentialConsistency) volatile {
        if (order == MemoryOrder::AcquireRelease || order == MemoryOrder::Release) {
            return compare_exchange_weak(expected, desired, MemoryOrder::Release, MemoryOrder::Acquire);
        } else {
            return compare_exchange_weak(expected, desired, order, order);
        }
    }

    bool compare_exchange_strong(T& expected, T desired, MemoryOrder success, MemoryOrder failure) {
        return __atomic_compare_exchange_n(util::addressof(m_value), util::addressof(expected), desired, false,
                                           util::to_underlying(success), util::to_underlying(failure));
    }
    bool compare_exchange_strong(T& expected, T desired, MemoryOrder success, MemoryOrder failure) volatile {
        return __atomic_compare_exchange_n(util::addressof(m_value), util::addressof(expected), desired, false,
                                           util::to_underlying(success), util::to_underlying(failure));
    }

    bool compare_exchange_strong(T& expected, T desired, MemoryOrder order = MemoryOrder::SequentialConsistency) {
        if (order == MemoryOrder::AcquireRelease || order == MemoryOrder::Release) {
            return compare_exchange_strong(expected, desired, MemoryOrder::Release, MemoryOrder::Acquire);
        } else {
            return compare_exchange_strong(expected, desired, order, order);
        }
    }
    bool compare_exchange_strong(T& expected, T desired,
                                 MemoryOrder order = MemoryOrder::SequentialConsistency) volatile {
        if (order == MemoryOrder::AcquireRelease || order == MemoryOrder::Release) {
            return compare_exchange_strong(expected, desired, MemoryOrder::Release, MemoryOrder::Acquire);
        } else {
            return compare_exchange_strong(expected, desired, order, order);
        }
    }

#define DI_VOLATILE volatile

    constexpr T fetch_add(DeltaType delta, MemoryOrder order = MemoryOrder::SequentialConsistency)
    requires(concepts::Integral<T> || concepts::Pointer<T>)
    {
        return __atomic_fetch_add(util::addressof(m_value), adjust_delta(delta), util::to_underlying(order));
    }
    constexpr T fetch_add(DeltaType delta, MemoryOrder order = MemoryOrder::SequentialConsistency) DI_VOLATILE
    requires(concepts::Integral<T> || concepts::Pointer<T>)
    {
        return __atomic_fetch_add(util::addressof(m_value), adjust_delta(delta), util::to_underlying(order));
    }

    constexpr T fetch_sub(DeltaType delta, MemoryOrder order = MemoryOrder::SequentialConsistency)
    requires(concepts::Integral<T> || concepts::Pointer<T>)
    {
        return __atomic_fetch_sub(util::addressof(m_value), adjust_delta(delta), util::to_underlying(order));
    }
    constexpr T fetch_sub(DeltaType delta, MemoryOrder order = MemoryOrder::SequentialConsistency) DI_VOLATILE
    requires(concepts::Integral<T> || concepts::Pointer<T>)
    {
        return __atomic_fetch_sub(util::addressof(m_value), adjust_delta(delta), util::to_underlying(order));
    }

    T fetch_and(T value, MemoryOrder order = MemoryOrder::SequentialConsistency)
    requires(concepts::Integral<T>)
    {
        return __atomic_fetch_and(util::addressof(m_value), value, util::to_underlying(order));
    }
    T fetch_and(T value, MemoryOrder order = MemoryOrder::SequentialConsistency) DI_VOLATILE
    requires(concepts::Integral<T>)
    {
        return __atomic_fetch_and(util::addressof(m_value), value, util::to_underlying(order));
    }

    T fetch_or(T value, MemoryOrder order = MemoryOrder::SequentialConsistency)
    requires(concepts::Integral<T>)
    {
        return __atomic_fetch_or(util::addressof(m_value), value, util::to_underlying(order));
    }
    T fetch_or(T value, MemoryOrder order = MemoryOrder::SequentialConsistency) DI_VOLATILE
    requires(concepts::Integral<T>)
    {
        return __atomic_fetch_or(util::addressof(m_value), value, util::to_underlying(order));
    }

    T fetch_xor(T value, MemoryOrder order = MemoryOrder::SequentialConsistency)
    requires(concepts::Integral<T>)
    {
        return __atomic_fetch_xor(util::addressof(m_value), value, util::to_underlying(order));
    }
    T fetch_xor(T value, MemoryOrder order = MemoryOrder::SequentialConsistency) DI_VOLATILE
    requires(concepts::Integral<T>)
    {
        return __atomic_fetch_xor(util::addressof(m_value), value, util::to_underlying(order));
    }

#undef DI_VOLATILE

private:
    T m_value;
};
}
