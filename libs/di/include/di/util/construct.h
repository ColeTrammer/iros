#pragma once

#include "di/function/curry_back.h"
#include "di/meta/operations.h"
#include "di/util/forward.h"

namespace di::util {
template<typename T>
struct Construct {
    template<typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr static auto operator()(Args&&... args) -> T {
        return T(di::forward<Args>(args)...);
    }
};

template<typename T>
constexpr inline auto construct = di::curry_back(Construct<T> {});
}

namespace di {
using util::construct;
}
