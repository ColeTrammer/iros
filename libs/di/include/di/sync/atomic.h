#pragma once

#include <di/meta/core.h>
#include <di/meta/language.h>
#include <di/meta/util.h>
#include <di/sync/atomic_ref.h>
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

    template<concepts::RemoveCVRefSameAs<Atomic> Self>
    constexpr auto as_ref(this Self&& self) {
        return AtomicRef<meta::Like<meta::RemoveReference<Self>, T>>(self.m_value);
    }

public:
    Atomic() = default;
    Atomic(Atomic const&) = delete;

    constexpr explicit Atomic(T value) : m_value(value) {}

    Atomic& operator=(Atomic const&) = delete;
    Atomic& operator=(Atomic const&) volatile = delete;

    constexpr void store(T value, MemoryOrder order = MemoryOrder::SequentialConsistency) {
        return as_ref().store(value, order);
    }
    constexpr void store(T value, MemoryOrder order = MemoryOrder::SequentialConsistency) volatile {
        return as_ref().store(value, order);
    }

    constexpr T load(MemoryOrder order = MemoryOrder::SequentialConsistency) const { return as_ref().load(order); }
    constexpr T load(MemoryOrder order = MemoryOrder::SequentialConsistency) const volatile {
        return as_ref().load(order);
    }

    constexpr T exchange(T value, MemoryOrder order = MemoryOrder::SequentialConsistency) {
        return as_ref().exchange(value, order);
    }
    constexpr T exchange(T value, MemoryOrder order = MemoryOrder::SequentialConsistency) volatile {
        return as_ref().exchange(value, order);
    }

    constexpr bool compare_exchange_weak(T& expected, T desired, MemoryOrder success, MemoryOrder failure) {
        return as_ref().compare_exchange_weak(expected, desired, success, failure);
    }
    constexpr bool compare_exchange_weak(T& expected, T desired, MemoryOrder success, MemoryOrder failure) volatile {
        return as_ref().compare_exchange_weak(expected, desired, success, failure);
    }

    constexpr bool compare_exchange_weak(T& expected, T desired,
                                         MemoryOrder order = MemoryOrder::SequentialConsistency) {
        return as_ref().compare_exchange_weak(expected, desired, order);
    }
    constexpr bool compare_exchange_weak(T& expected, T desired,
                                         MemoryOrder order = MemoryOrder::SequentialConsistency) volatile {
        return as_ref().compare_exchange_weak(expected, desired, order);
    }

    constexpr bool compare_exchange_strong(T& expected, T desired, MemoryOrder success, MemoryOrder failure) {
        return as_ref().compare_exchange_strong(expected, desired, success, failure);
    }
    constexpr bool compare_exchange_strong(T& expected, T desired, MemoryOrder success, MemoryOrder failure) volatile {
        return as_ref().compare_exchange_strong(expected, desired, success, failure);
    }

    constexpr bool compare_exchange_strong(T& expected, T desired,
                                           MemoryOrder order = MemoryOrder::SequentialConsistency) {
        return as_ref().compare_exchange_strong(expected, desired, order);
    }
    constexpr bool compare_exchange_strong(T& expected, T desired,
                                           MemoryOrder order = MemoryOrder::SequentialConsistency) volatile {
        return as_ref().compare_exchange_strong(expected, desired, order);
    }

    constexpr T fetch_add(DeltaType delta, MemoryOrder order = MemoryOrder::SequentialConsistency)
    requires(concepts::Integral<T> || concepts::Pointer<T>)
    {
        return as_ref().fetch_add(delta, order);
    }
    constexpr T fetch_add(DeltaType delta, MemoryOrder order = MemoryOrder::SequentialConsistency) volatile requires(
        concepts::Integral<T> || concepts::Pointer<T>) { return as_ref().fetch_add(delta, order); }

    constexpr T fetch_sub(DeltaType delta, MemoryOrder order = MemoryOrder::SequentialConsistency)
    requires(concepts::Integral<T> || concepts::Pointer<T>)
    {
        return as_ref().fetch_sub(delta, order);
    }
    constexpr T fetch_sub(DeltaType delta, MemoryOrder order = MemoryOrder::SequentialConsistency) volatile requires(
        concepts::Integral<T> || concepts::Pointer<T>) { return as_ref().fetch_sub(delta, order); }

    constexpr T fetch_and(T value, MemoryOrder order = MemoryOrder::SequentialConsistency)
    requires(concepts::Integral<T>)
    {
        return as_ref().fetch_and(value, order);
    }
    constexpr T fetch_and(T value, MemoryOrder order = MemoryOrder::SequentialConsistency) volatile requires(
        concepts::Integral<T>) { return as_ref().fetch_and(value, order); }

    constexpr T fetch_or(T value, MemoryOrder order = MemoryOrder::SequentialConsistency)
    requires(concepts::Integral<T>)
    {
        return as_ref().fetch_or(value, order);
    }
    constexpr T
    fetch_or(T value, MemoryOrder order = MemoryOrder::SequentialConsistency) volatile requires(concepts::Integral<T>) {
        return as_ref().fetch_or(value, order);
    }

    constexpr T fetch_xor(T value, MemoryOrder order = MemoryOrder::SequentialConsistency)
    requires(concepts::Integral<T>)
    {
        return as_ref().fetch_xor(value, order);
    }
constexpr T fetch_xor(T value,
                      MemoryOrder order = MemoryOrder::SequentialConsistency) volatile requires(concepts::Integral<T>) {
    return as_ref().fetch_xor(value, order);
}

private : T m_value;
};
}

namespace di {
using sync::Atomic;
}
