#pragma once

#include <di/vocab/tuple/tuple.h>

namespace di::vocab {
template<typename... Types>
constexpr auto tie(Types&... references) {
    return Tuple<Types&...>(references...);
}
}
