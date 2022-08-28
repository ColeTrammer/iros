#pragma once

#include <di/concepts/constructible_from.h>

namespace di::util {
template<typename T, bool should_store>
struct StoreIf {
    T value;
};

template<typename T>
struct StoreIf<T, false> {
    template<typename... Args>
    requires(concepts::ConstructibleFrom<StoreIf<T, true>, Args...>)
    constexpr StoreIf(Args&&...) {}
};
}
