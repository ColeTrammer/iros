#pragma once

#include <di/util/forward.h>
#include <di/vocab/tuple/tuple.h>

namespace di::vocab {
template<typename... Args>
constexpr Tuple<Args&&...> forward_as_tuple(Args&&... args) {
    return Tuple<Args&&...>(util::forward<Args>(args)...);
}
}
