#pragma once

#include <di/meta/unwrap_ref_decay.h>
#include <di/util/forward.h>
#include <di/vocab/optional/optional.h>

namespace di::vocab::optional {
template<typename T>
constexpr auto make_optional(T&& value) {
    return Optional<meta::UnwrapRefDecay<T>> { util::forward<T>(value) };
}

template<typename T, typename... Args>
constexpr auto make_optional(Args&&... args) {
    return Optional<T>(types::in_place, util::forward<Args>(args)...);
}
}
