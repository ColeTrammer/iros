#pragma once

#include "di/meta/core.h"
#include "di/meta/language.h"
#include "di/sync/memory_order.h"
#include "di/types/prelude.h"
#include "di/util/addressof.h"
#include "di/util/exchange.h"
#include "di/util/to_underlying.h"

namespace di::sync {
template<typename T>
requires((concepts::Enum<T> || concepts::Arithmetic<T> || concepts::Pointer<T>) && sizeof(T) <= sizeof(void*))
class AtomicRef {
private:
    using DeltaType = meta::Conditional<concepts::Pointer<T>, ptrdiff_t, T>;

    // NOTE: the builtin atomic operations treat pointer addition bytewise, so we
    //       must multiply by the sizeof(*T) if T is a pointer.
    constexpr auto adjust_delta(DeltaType value) -> DeltaType {
        if constexpr (concepts::Pointer<T>) {
            return value * sizeof(**m_pointer);
        } else {
            return value;
        }
    }

public:
    AtomicRef(AtomicRef const&) = default;

    constexpr explicit AtomicRef(T& value) : m_pointer(util::addressof(value)) {}

    auto operator=(AtomicRef const&) -> AtomicRef& = delete;

    constexpr void store(T value, MemoryOrder order = MemoryOrder::SequentialConsistency) {
        if consteval {
            *m_pointer = value;
        } else {
            __atomic_store_n(m_pointer, value, util::to_underlying(order));
        }
    }

    constexpr auto load(MemoryOrder order = MemoryOrder::SequentialConsistency) const -> T {
        if consteval {
            return *m_pointer;
        }
        return __atomic_load_n(m_pointer, util::to_underlying(order));
    }

    constexpr auto exchange(T value, MemoryOrder order = MemoryOrder::SequentialConsistency) -> T {
        if consteval {
            return di::exchange(*m_pointer, value);
        }
        return __atomic_exchange_n(m_pointer, value, util::to_underlying(order));
    }

    constexpr auto compare_exchange_weak(T& expected, T desired, MemoryOrder success, MemoryOrder failure) -> bool {
        if consteval {
            if (*m_pointer == expected) {
                *m_pointer = desired;
                return true;
            }
            expected = *m_pointer;
            return false;
        }
        return __atomic_compare_exchange_n(m_pointer, util::addressof(expected), desired, true,
                                           util::to_underlying(success), util::to_underlying(failure));
    }

    constexpr auto compare_exchange_weak(T& expected, T desired, MemoryOrder order = MemoryOrder::SequentialConsistency)
        -> bool {
        if (order == MemoryOrder::AcquireRelease || order == MemoryOrder::Release) {
            return compare_exchange_weak(expected, desired, MemoryOrder::Release, MemoryOrder::Acquire);
        }
        return compare_exchange_weak(expected, desired, order, order);
    }

    constexpr auto compare_exchange_strong(T& expected, T desired, MemoryOrder success, MemoryOrder failure) -> bool {
        if consteval {
            if (*m_pointer == expected) {
                *m_pointer = desired;
                return true;
            }
            expected = *m_pointer;
            return false;
        }
        return __atomic_compare_exchange_n(m_pointer, util::addressof(expected), desired, false,
                                           util::to_underlying(success), util::to_underlying(failure));
    }

    constexpr auto compare_exchange_strong(T& expected, T desired,
                                           MemoryOrder order = MemoryOrder::SequentialConsistency) -> bool {
        if (order == MemoryOrder::AcquireRelease || order == MemoryOrder::Release) {
            return compare_exchange_strong(expected, desired, MemoryOrder::Release, MemoryOrder::Acquire);
        }
        return compare_exchange_strong(expected, desired, order, order);
    }

    constexpr auto fetch_add(DeltaType delta, MemoryOrder order = MemoryOrder::SequentialConsistency) -> T
    requires(concepts::Integral<T> || concepts::Pointer<T>)
    {
        if consteval {
            auto result = *m_pointer;
            *m_pointer += adjust_delta(delta);
            return result;
        }
        return __atomic_fetch_add(m_pointer, adjust_delta(delta), util::to_underlying(order));
    }

    constexpr auto fetch_sub(DeltaType delta, MemoryOrder order = MemoryOrder::SequentialConsistency) -> T
    requires(concepts::Integral<T> || concepts::Pointer<T>)
    {
        if consteval {
            auto result = *m_pointer;
            *m_pointer -= adjust_delta(delta);
            return result;
        }
        return __atomic_fetch_sub(m_pointer, adjust_delta(delta), util::to_underlying(order));
    }

    constexpr auto fetch_and(T value, MemoryOrder order = MemoryOrder::SequentialConsistency) -> T
    requires(concepts::Integral<T>)
    {
        if consteval {
            auto result = *m_pointer;
            *m_pointer &= value;
            return result;
        }
        return __atomic_fetch_and(m_pointer, value, util::to_underlying(order));
    }

    constexpr auto fetch_or(T value, MemoryOrder order = MemoryOrder::SequentialConsistency) -> T
    requires(concepts::Integral<T>)
    {
        if consteval {
            auto result = *m_pointer;
            *m_pointer |= value;
            return result;
        }
        return __atomic_fetch_or(m_pointer, value, util::to_underlying(order));
    }

    constexpr auto fetch_xor(T value, MemoryOrder order = MemoryOrder::SequentialConsistency) -> T
    requires(concepts::Integral<T>)
    {
        if consteval {
            auto result = *m_pointer;
            *m_pointer ^= value;
            return result;
        }
        return __atomic_fetch_xor(m_pointer, value, util::to_underlying(order));
    }

private:
    T* m_pointer { nullptr };
};
}

namespace di {
using sync::AtomicRef;
}
