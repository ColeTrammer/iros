#pragma once

#include "di/meta/util.h"
#include "di/util/forward.h"
#include "di/vocab/optional/optional.h"

namespace di::vocab {
template<typename T>
constexpr auto make_optional(T&& value) {
    return Optional<meta::UnwrapRefDecay<T>> { util::forward<T>(value) };
}

template<typename T, typename... Args>
constexpr auto make_optional(Args&&... args) {
    return Optional<T>(types::in_place, util::forward<Args>(args)...);
}
}

namespace di {
using vocab::make_optional;
}
