#pragma once

#include <di/meta/unwrap_ref_decay.h>
#include <di/util/forward.h>
#include <di/vocab/tuple/tuple.h>

namespace di::vocab {
template<typename... Args>
constexpr auto make_tuple(Args&&... args) {
    return Tuple<meta::UnwrapRefDecay<Args>...>(util::forward<Args>(args)...);
}
}
