#pragma once

#include <di/util/forward.h>
#include <di/vocab/tuple/tuple.h>

namespace di::vocab {
template<typename... Args>
constexpr auto forward_as_tuple(Args&&... args) -> Tuple<Args&&...> {
    return Tuple<Args&&...>(util::forward<Args>(args)...);
}
}

namespace di {
using vocab::forward_as_tuple;
}
