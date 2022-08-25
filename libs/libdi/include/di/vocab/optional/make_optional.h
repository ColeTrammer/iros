#pragma once

#include <di/util/forward.h>
#include <di/util/meta/unwrap_ref_decay.h>
#include <di/vocab/optional/optional.h>

namespace di::vocab::optional {
template<typename T>
constexpr auto make_optional(T&& value) {
    return Optional<util::meta::UnwrapRefDecay<T>> { util::forward<T>(value) };
}

template<typename T, typename... Args>
constexpr auto make_optional(Args&&... args) {
    return Optional<T>(util::in_place, util::forward<Args>(args)...);
}
}
