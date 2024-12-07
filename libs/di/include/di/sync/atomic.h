#pragma once

#include "di/meta/core.h"
#include "di/meta/language.h"
#include "di/meta/util.h"
#include "di/sync/atomic_ref.h"
#include "di/sync/memory_order.h"
#include "di/types/prelude.h"
#include "di/util/addressof.h"
#include "di/util/to_underlying.h"

namespace di::sync {
template<typename T>
requires((concepts::Enum<T> || concepts::Arithmetic<T> || concepts::Pointer<T>) && sizeof(T) <= sizeof(void*))
class Atomic {
private:
    using DeltaType = meta::Conditional<concepts::Pointer<T>, ptrdiff_t, T>;

    template<concepts::RemoveCVRefSameAs<Atomic> Self>
    constexpr auto as_ref(this Self&& self) {
        return AtomicRef<meta::Like<meta::RemoveReference<Self>, T>>(self.m_value);
    }

public:
    Atomic() = default;
    Atomic(Atomic const&) = delete;

    constexpr explicit Atomic(T value) : m_value(value) {}

    auto operator=(Atomic const&) -> Atomic& = delete;
    auto operator=(Atomic const&) volatile -> Atomic& = delete;

    constexpr void store(T value, MemoryOrder order = MemoryOrder::SequentialConsistency) {
        return as_ref().store(value, order);
    }
    constexpr void store(T value, MemoryOrder order = MemoryOrder::SequentialConsistency) volatile {
        return as_ref().store(value, order);
    }

    constexpr auto load(MemoryOrder order = MemoryOrder::SequentialConsistency) const -> T {
        return as_ref().load(order);
    }
    constexpr auto load(MemoryOrder order = MemoryOrder::SequentialConsistency) const volatile -> T {
        return as_ref().load(order);
    }

    constexpr auto exchange(T value, MemoryOrder order = MemoryOrder::SequentialConsistency) -> T {
        return as_ref().exchange(value, order);
    }
    constexpr auto exchange(T value, MemoryOrder order = MemoryOrder::SequentialConsistency) volatile -> T {
        return as_ref().exchange(value, order);
    }

    constexpr auto compare_exchange_weak(T& expected, T desired, MemoryOrder success, MemoryOrder failure) -> bool {
        return as_ref().compare_exchange_weak(expected, desired, success, failure);
    }
    constexpr auto compare_exchange_weak(T& expected, T desired, MemoryOrder success, MemoryOrder failure) volatile
        -> bool {
        return as_ref().compare_exchange_weak(expected, desired, success, failure);
    }

    constexpr auto compare_exchange_weak(T& expected, T desired, MemoryOrder order = MemoryOrder::SequentialConsistency)
        -> bool {
        return as_ref().compare_exchange_weak(expected, desired, order);
    }
    constexpr auto compare_exchange_weak(T& expected, T desired,
                                         MemoryOrder order = MemoryOrder::SequentialConsistency) volatile -> bool {
        return as_ref().compare_exchange_weak(expected, desired, order);
    }

    constexpr auto compare_exchange_strong(T& expected, T desired, MemoryOrder success, MemoryOrder failure) -> bool {
        return as_ref().compare_exchange_strong(expected, desired, success, failure);
    }
    constexpr auto compare_exchange_strong(T& expected, T desired, MemoryOrder success, MemoryOrder failure) volatile
        -> bool {
        return as_ref().compare_exchange_strong(expected, desired, success, failure);
    }

    constexpr auto compare_exchange_strong(T& expected, T desired,
                                           MemoryOrder order = MemoryOrder::SequentialConsistency) -> bool {
        return as_ref().compare_exchange_strong(expected, desired, order);
    }
    constexpr auto compare_exchange_strong(T& expected, T desired,
                                           MemoryOrder order = MemoryOrder::SequentialConsistency) volatile -> bool {
        return as_ref().compare_exchange_strong(expected, desired, order);
    }

    constexpr auto fetch_add(DeltaType delta, MemoryOrder order = MemoryOrder::SequentialConsistency) -> T
    requires(concepts::Integral<T> || concepts::Pointer<T>)
    {
        return as_ref().fetch_add(delta, order);
    }
    constexpr auto fetch_add(DeltaType delta, MemoryOrder order = MemoryOrder::SequentialConsistency) volatile -> T
    requires(concepts::Integral<T> || concepts::Pointer<T>)
    {
        return as_ref().fetch_add(delta, order);
    }

    constexpr auto fetch_sub(DeltaType delta, MemoryOrder order = MemoryOrder::SequentialConsistency) -> T
    requires(concepts::Integral<T> || concepts::Pointer<T>)
    {
        return as_ref().fetch_sub(delta, order);
    }
    constexpr auto fetch_sub(DeltaType delta, MemoryOrder order = MemoryOrder::SequentialConsistency) volatile -> T
    requires(concepts::Integral<T> || concepts::Pointer<T>)
    {
        return as_ref().fetch_sub(delta, order);
    }

    constexpr auto fetch_and(T value, MemoryOrder order = MemoryOrder::SequentialConsistency) -> T
    requires(concepts::Integral<T>)
    {
        return as_ref().fetch_and(value, order);
    }
    constexpr auto fetch_and(T value, MemoryOrder order = MemoryOrder::SequentialConsistency) volatile -> T
    requires(concepts::Integral<T>)
    {
        return as_ref().fetch_and(value, order);
    }

    constexpr auto fetch_or(T value, MemoryOrder order = MemoryOrder::SequentialConsistency) -> T
    requires(concepts::Integral<T>)
    {
        return as_ref().fetch_or(value, order);
    }
    constexpr auto fetch_or(T value, MemoryOrder order = MemoryOrder::SequentialConsistency) volatile -> T
    requires(concepts::Integral<T>)
    {
        return as_ref().fetch_or(value, order);
    }

    constexpr auto fetch_xor(T value, MemoryOrder order = MemoryOrder::SequentialConsistency) -> T
    requires(concepts::Integral<T>)
    {
        return as_ref().fetch_xor(value, order);
    }
    constexpr auto fetch_xor(T value, MemoryOrder order = MemoryOrder::SequentialConsistency) volatile -> T
    requires(concepts::Integral<T>)
    {
        return as_ref().fetch_xor(value, order);
    }

private:
    T m_value;
};
}

namespace di {
using sync::Atomic;
}
