#pragma once

#include <di/util/forward.h>
#include <di/util/move.h>

namespace di::util {
template<typename T, typename U = T>
constexpr T exchange(T& object, U&& new_value) {
    auto temp = util::move(object);
    object = util::forward<U>(new_value);
    return temp;
}
}

namespace di {
using util::exchange;
}
