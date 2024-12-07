#pragma once

#include "di/meta/operations.h"

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

    StoreIf() = default;
    StoreIf(StoreIf const&) = default;
    StoreIf(StoreIf&&) = default;
    auto operator=(StoreIf const&) -> StoreIf& = default;
    auto operator=(StoreIf&&) -> StoreIf& = default;
};
}

namespace di {
using util::StoreIf;
}
